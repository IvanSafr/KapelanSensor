//////////////////////////////////////////////////////////////////////////
// main.cpp — Прошивка метеостанции ENV Station (протокол v15)
// Платформа: ESP32 / Seeed Studio XIAO ESP32-S3
// Датчики (шина I2C):
//   1. Sensirion SCD41 — фотоакустический датчик CO2 + основная температура и влажность
//   2. Bosch BME680    — атмосферное давление, газовое сопротивление (MOX) + резерв Т/В
//   3. Lite-On LTR390  — оптический датчик освещенности (ALS) и ультрафиолета (UVS)
//////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>

#include "SparkFun_SCD4x_Arduino_Library.h"
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
#include "Adafruit_LTR390.h"

#include "index.h"
#include "config.h"

SCD4x COSensor;
Adafruit_BME680 bme(&Wire);
Adafruit_LTR390 ltr = Adafruit_LTR390();

WebServer server(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;

const char* ssid     = "ESP_Network";
const char* password = ""; 

// ============================================================================
// АРХИТЕКТУРА ХРАНЕНИЯ ДАННЫХ (Глобальный стейт)
// ----------------------------------------------------------------------------
// ПОЧЕМУ ТАК СДЕЛАНО:
// Раньше опрос датчиков (включая bme.performReading()) вызывался прямо внутри
// HTTP-обработчика. Это приводило к двум проблемам:
//   1) Пока веб-страница закрыта, BME680 не опрашивался, и его нагреватель (320°C)
//      остывал. При открытии страницы первые замеры давали ложную тревогу по газу.
//   2) Каждый HTTP-запрос блокировал контроллер на ~200 мс (время нагрева BME680).
//
// РЕШЕНИЕ:
// Все датчики опрашиваются в фоновом цикле loop() раз в 1 секунду (updateSensors),
// данные проходят фильтрацию и складываются в структуру `sensors`.
// Веб-сервер не трогает шину I2C, а мгновенно (за ~1 мс) отдает готовый стейт из ОЗУ.
// ============================================================================
struct SensorState {
  // --- SCD41 (Основной климат + CO2) ---
  uint16_t co2 = 0;                // Концентрация CO2 в ppm
  float scd_temp = 0.0;            // Температура SCD41 (°C) — основная, т.к. нет нагревателя MOX
  float scd_hum = 0.0;             // Относительная влажность SCD41 (%)
  bool scd_ok = false;             // Флаг получения валидных данных
  unsigned long scd_updated = 0;   // Timestamp (millis) последнего успешного чтения

  // --- BME680 (Давление, газ + доп. температура/влажность) ---
  float bme_temp = 0.0;            // Температура BME680 (может быть чуть выше из-за нагревателя газа)
  float bme_hum = 0.0;             // Влажность BME680 (%)
  float press_pa = 0.0;            // Давление в Паскалях (основной формат для протокола v15)
  float press_mm = 0.0;            // Давление в мм рт. ст.
  float gas_ohm = 0.0;             // Сопротивление газового сенсора в Омах (для /api/data)
  bool bme_ok = false;             // Флаг работоспособности BME680
  bool gas_stable = false;         // Флаг завершения 90-секундного прогрева пластины MOX
  unsigned long bme_updated = 0;   // Timestamp последнего успешного чтения BME680

  // --- LTR390 (Свет ALS + Ультрафиолет UVS) ---
  float uvi = 0.0;                 // Рассчитанный УФ-индекс (UVI)
  uint32_t uv_raw = 0;             // Сырые отсчеты АЦП в режиме UVS
  float lux = 0.0;                 // Освещенность в люксах (лк)
  uint32_t als_raw = 0;            // Сырые отсчеты АЦП в режиме ALS
  bool ltr_ok = false;             // Флаг активности датчика света
  unsigned long ltr_updated = 0;   // Timestamp последнего успешного чтения LTR390
} sensors;

unsigned long lastSensorRead = 0;  // Таймер для неблокирующего опроса в loop()
uint16_t gasWarmupSamples = 0;     // Счетчик успешных замеров газа с момента старта (нужно 90 сек)

// ============================================================================
// УПРАВЛЕНИЕ СОСТОЯНИЯМИ LTR390 (Защита от переходных процессов АЦП)
// ----------------------------------------------------------------------------
// Внутри LTR390 две оптические матрицы (ALS и UVS), но один общий АЦП.
// Чтобы измерять и люксы, и ультрафиолет, мы чередуем режимы.
// ВАЖНО: После смены режима (UVS <-> ALS) и смены усиления (GAIN_18 <-> GAIN_3)
// первый замер в АЦП всегда содержит переходный "мусор" (остаточный заряд), из-за чего
// показания люксов раньше плавно "сползали".
// Флаг `ltrSkipNextSample` заставляет принудительно сбросить первый замер после
// переключения и прочитать только второй — полностью чистый и устоявшийся.
// ============================================================================
bool ltrMeasuringUV = true;    // Текущий режим: true = UVS (УФ), false = ALS (Люксы)
bool ltrSkipNextSample = true; // true = следующий готовый замер нужно прочитать вхолостую и отбросить

void handleNotFound();
void updateSensors();
void configureLtrMode(bool uvMode);
void sendApiData();

void setup() {
  Serial.begin(115200);
  
  ////////////////////////////////////////////////////////////////////////
  // Инициализация шины I2C и датчиков
  Wire.begin(SDA, SCL);

  // 1. CO2 (SCD4x) — запускаем внутренние периодические измерения (раз в 5 секунд)
  COSensor.begin();
  COSensor.startPeriodicMeasurement();

  // 2. BME680 — настраиваем передискретизацию, IIR-фильтр и профиль нагревателя
  bme.begin();
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // Нагрев чувствительного слоя до 320°C в течение 150 мс

  // 3. LTR390 — инициализация без программного сброса калибровочных OTP-регистров
  // (регистры 0x02, 0x03, 0x16, 0x1C хранят заводские поправки кристалла, их нельзя затирать)
  if (ltr.begin()) {
    configureLtrMode(false); // Начинаем с более быстрого замера освещенности (ALS)
  }

  ////////////////////////////////////////////////////////////////////////
  // Wi-Fi (Точка доступа + Captive Portal DNS) и Веб-сервер
  WiFi.softAP(ssid, password);
  Serial.println("");
  Serial.print("IP адрес точки доступа: ");
  Serial.println(WiFi.softAPIP());

  // Перенаправляем все DNS-запросы на IP нашей ESP32 (для работы Captive Portal)
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Главная страница: используем send_P для отдачи большого HTML напрямую из Flash (PROGMEM),
  // не копируя 50 КБ в оперативную память (Heap)
  server.on("/", []() {
    server.send_P(200, "text/html", index_html);
  });

  // Основной JSON-эндпоинт для дашборда v15 (/api/data) + алиас (/api/chip-info) для обратной совместимости
  server.on("/api/data", sendApiData);
  server.on("/api/chip-info", sendApiData);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Веб-сервер запущен.");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  // Неблокирующий фоновый опрос всех датчиков строго раз в 1000 мс (1 сек).
  // Работает 24/7 независимо от того, открыт ли браузер у клиента.
  if (millis() - lastSensorRead >= 1000) {
    lastSensorRead = millis();
    updateSensors();
  }
}

//////////////////////////////////////////////////////////////////////////
// Формирование JSON-ответа по спецификации протокола ENV Station v15
//////////////////////////////////////////////////////////////////////////
void sendApiData() {
  unsigned long now = millis();
  uint32_t up_s = now / 1000;

  // Вычисляем "возраст" последнего успешного замера каждого датчика (в секундах)
  // для отображения во вкладке "Система" -> "Датчики"
  uint32_t scd_age = sensors.scd_updated ? (now - sensors.scd_updated) / 1000 : 0;
  uint32_t bme_age = sensors.bme_updated ? (now - sensors.bme_updated) / 1000 : 0;
  uint32_t ltr_age = sensors.ltr_updated ? (now - sensors.ltr_updated) / 1000 : 0;

  // Определяем текстовые статусы датчиков ("warming" / "waiting" / "ready" / "error"):
  // - SCD41 дает первый замер через ~5-10 сек после старта (до этого статус "warming")
  // - BME680 первые 90 сек прогревает газовую пластину (статус "warming")
  String scd_state = !sensors.scd_ok ? (up_s < 15 ? "warming" : "error") : "ready";
  String bme_state = !sensors.bme_ok ? "error" : (!sensors.gas_stable ? "warming" : "ready");
  String ltr_state = !sensors.ltr_ok ? (up_s < 10 ? "waiting" : "error") : "ready";

  // Основная температура и влажность для главного экрана:
  // Приоритет у SCD41 (он точнее, т.к. не греет воздух вокруг себя до 320°C как BME680).
  // Если SCD41 еще не выдал первый пакет, временно берем данные с BME680.
  float main_temp = sensors.scd_ok ? sensors.scd_temp : sensors.bme_temp;
  float main_hum  = sensors.scd_ok ? sensors.scd_hum  : sensors.bme_hum;

  String json = "{";
  json += "\"protocol\":15,";
  json += "\"firmware\":\"v15-esp32\",";
  json += "\"uptime_s\":" + String(up_s) + ",";
  json += "\"ram_free\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"cpu_mhz\":"  + String(ESP.getCpuFreqMHz()) + ",";
  json += "\"mac\":\""    + WiFi.softAPmacAddress() + "\",";
  json += "\"chip_model\":\"" + String(ESP.getChipModel()) + "\",";

  // Показания SCD41 (если еще не готов — передаем null, чтобы фронтенд показал "Ожидание данных")
  if (sensors.scd_ok) {
    json += "\"co2_ppm\":" + String(sensors.co2) + ",";
  } else {
    json += "\"co2_ppm\":null,";
  }

  // Основные температура и влажность
  if (sensors.scd_ok || sensors.bme_ok) {
    json += "\"temperature_c\":" + String(main_temp, 1) + ",";
    json += "\"humidity_pct\":"  + String(main_hum, 1) + ",";
  } else {
    json += "\"temperature_c\":null,\"humidity_pct\":null,";
  }

  // Показания BME680:
  // ПОЧЕМУ gas_ohm отдает null первые 90 секунд:
  // У непрогретого датчика сопротивление слоя минимальное (единицы кОм), что выглядит как
  // "Сильное загрязнение воздуха". Передача null + uptime_s позволяет фронтенду показать
  // честный обратный отсчет "Прогрев... X с" вместо ложной тревоги.
  if (sensors.bme_ok) {
    json += "\"pressure_pa\":"       + String(sensors.press_pa, 0) + ",";
    json += "\"bme_temperature_c\":" + String(sensors.bme_temp, 1) + ",";
    json += "\"bme_humidity_pct\":"  + String(sensors.bme_hum, 1) + ",";
    if (sensors.gas_stable && up_s >= 90) {
      json += "\"gas_ohm\":" + String(sensors.gas_ohm, 0) + ",";
    } else {
      json += "\"gas_ohm\":null,";
    }
  } else {
    json += "\"pressure_pa\":null,\"bme_temperature_c\":null,\"bme_humidity_pct\":null,\"gas_ohm\":null,";
  }

  // Показания LTR390 (Передаем и рассчитанный UVI, и освещенность в люксах, и сырые счетчики)
  if (sensors.ltr_ok) {
    json += "\"lux\":"      + String(sensors.lux, 1) + ",";
    json += "\"uv_index\":" + String(sensors.uvi, 2) + ",";
    json += "\"uv_raw\":"   + String(sensors.uv_raw) + ",";
    json += "\"als_raw\":"  + String(sensors.als_raw) + ",";
  } else {
    json += "\"lux\":null,\"uv_index\":null,\"uv_raw\":null,\"als_raw\":null,";
  }

  // Диагностические флаги и состояния для вкладки "Система"
  json += "\"scd41_ok\":"     + String(sensors.scd_ok ? "true" : "false") + ",";
  json += "\"bme680_ok\":"    + String(sensors.bme_ok ? "true" : "false") + ",";
  json += "\"ltr390_ok\":"    + String(sensors.ltr_ok ? "true" : "false") + ",";
  json += "\"scd41_state\":\""  + scd_state + "\",";
  json += "\"bme680_state\":\"" + bme_state + "\",";
  json += "\"ltr390_state\":\"" + ltr_state + "\",";
  json += "\"scd41_age_s\":"  + String(scd_age) + ",";
  json += "\"bme680_age_s\":" + String(bme_age) + ",";
  json += "\"ltr390_age_s\":" + String(ltr_age);
  json += "}";

  server.send(200, "application/json", json);
}

//////////////////////////////////////////////////////////////////////////
// Переключение режимов LTR390 (UVS <-> ALS) с защитой от переходного замера
//////////////////////////////////////////////////////////////////////////
void configureLtrMode(bool uvMode) {
  ltrMeasuringUV = uvMode;
  ltrSkipNextSample = true; // Сбрасываем первый замер после реконфигурации АЦП!

  if (uvMode) {
    // Режим УФ (UVS): ставим максимальное усиление GAIN_18 и максимальное разрешение 20 бит (выдержка 400 мс).
    // Это дает максимальную физическую чувствительность, для которой по даташиту Lite-On
    // действует точный калибровочный делитель 2300 имп. = 1 UVI.
    ltr.setMode(LTR390_MODE_UVS);
    ltr.setGain(LTR390_GAIN_18);
    ltr.setResolution(LTR390_RESOLUTION_20BIT);
  } else {
    // Режим видимого света (ALS): ставим умеренное усиление GAIN_3 и 18 бит (выдержка 100 мс),
    // чтобы АЦП не уходил в переполнение (Saturation) при ярком комнатном или дневном свете.
    ltr.setMode(LTR390_MODE_ALS);
    ltr.setGain(LTR390_GAIN_3);
    ltr.setResolution(LTR390_RESOLUTION_18BIT);
  }
}

//////////////////////////////////////////////////////////////////////////
// Фоновый опрос датчиков, валидация физических границ и EMA-фильтрация
//////////////////////////////////////////////////////////////////////////
void updateSensors() {
  // ----------------------------------------------------------------------
  // 1. Опрос SCD41 (CO2 + Температура + Влажность)
  // ----------------------------------------------------------------------
  if (COSensor.readMeasurement()) {
    uint16_t raw_co2 = COSensor.getCO2();
    float raw_t = COSensor.getTemperature();
    float raw_h = COSensor.getHumidity();

    // Отсекаем нули при инициализации и ошибки чтения I2C (65535)
    if (raw_co2 >= 350 && raw_co2 <= 10000) {
      if (!sensors.scd_ok) {
        // Первое валидное чтение записываем напрямую без сглаживания
        sensors.co2 = raw_co2;
        sensors.scd_temp = raw_t;
        sensors.scd_hum = raw_h;
        sensors.scd_ok = true;
      } else {
        // Экспоненциальное скользящее среднее (EMA 70% старое / 30% новое)
        // убирает случайный шум и резкие скачки от дыхания рядом с датчиком
        sensors.co2      = (uint16_t)(sensors.co2 * 0.7 + raw_co2 * 0.3);
        sensors.scd_temp = sensors.scd_temp * 0.7 + raw_t * 0.3;
        sensors.scd_hum  = sensors.scd_hum * 0.7 + raw_h * 0.3;
      }
      sensors.scd_updated = millis();
    }
  }

  // ----------------------------------------------------------------------
  // 2. Опрос BME680 (Температура, Влажность, Давление, Газ MOX)
  // ----------------------------------------------------------------------
  if (bme.performReading()) {
    float t = bme.temperature;
    float h = bme.humidity;
    float p_pa = bme.pressure;        // Давление в Паскалях
    float g_ohm = bme.gas_resistance; // Сопротивление в Омах

    // Проверка на физическую адекватность (защита от NaN и сбоев на шине I2C)
    bool valid_env = (!isnan(t) && t > -40.0 && t < 85.0) &&
                     (!isnan(h) && h >= 0.0 && h <= 100.0) &&
                     (!isnan(p_pa) && p_pa > 65000.0 && p_pa < 120000.0);

    if (valid_env) {
      if (!sensors.bme_ok) {
        sensors.bme_temp = t;
        sensors.bme_hum  = h;
        sensors.press_pa = p_pa;
        sensors.bme_ok   = true;
      } else {
        sensors.bme_temp = sensors.bme_temp * 0.7 + t * 0.3;
        sensors.bme_hum  = sensors.bme_hum * 0.7 + h * 0.3;
        sensors.press_pa = sensors.press_pa * 0.8 + p_pa * 0.2;
      }
      sensors.press_mm = sensors.press_pa / 133.322;
      sensors.bme_updated = millis();

      // Логика прогрева (90 циклов = 90 сек) и защита от одиночных провалов сопротивления газа
      if (g_ohm > 1000.0 && g_ohm < 2000000.0) {
        if (gasWarmupSamples < 90) {
          gasWarmupSamples++;
          sensors.gas_ohm = g_ohm;
          sensors.gas_stable = false;
        } else {
          // Если за 1 секунду сопротивление рухнуло более чем на 70% — скорее всего,
          // это одиночный артефакт питания/конвекции, демпфируем его жестким коэффициентом 0.9/0.1
          if (g_ohm < sensors.gas_ohm * 0.3) {
            sensors.gas_ohm = sensors.gas_ohm * 0.9 + g_ohm * 0.1;
          } else {
            sensors.gas_ohm = sensors.gas_ohm * 0.75 + g_ohm * 0.25;
          }
          sensors.gas_stable = true;
        }
      }
    } else {
      sensors.bme_ok = false;
    }
  } else {
    sensors.bme_ok = false;
  }

  // ----------------------------------------------------------------------
  // 3. Опрос LTR390 (Двухтактное чередование УФ и Люксов)
  // ----------------------------------------------------------------------
  if (ltr.newDataAvailable()) {
    if (ltrSkipNextSample) {
      // Такт 1 (Холостой): вычитываем регистр данных, чтобы сбросить флаг готовности
      // и выбросить переходный замер после переключения режима/усиления.
      if (ltrMeasuringUV) ltr.readUVS();
      else ltr.readALS();
      ltrSkipNextSample = false;
    } else {
      // Такт 2 (Чистое чтение): АЦП полностью устоялся в текущем режиме.
      // Применяем битовую маску `& 0x0FFFFF` (20 бит), чтобы отсечь старшие 4 резервных бита.
      if (ltrMeasuringUV) {
        uint32_t raw_uv = ltr.readUVS() & 0x0FFFFF;
        if (raw_uv < 0x0FFFFF) {
          sensors.uv_raw = raw_uv;
          // Формула UVI для GAIN_18 + 20BIT по документации Lite-On: raw / 2300.0
          sensors.uvi = (float)raw_uv / 2300.0;
          sensors.ltr_updated = millis();
        }
        configureLtrMode(false); // Переключаем на измерение видимого света (ALS)
      } else {
        uint32_t raw_als = ltr.readALS() & 0x0FFFFF;
        if (raw_als < 0x0FFFFF) {
          sensors.als_raw = raw_als;
          // Формула освещенности (Lux) из даташита: Lux = (0.6 * ALS_DATA) / (Gain * Int_Time)
          // При GAIN_3 (множитель 3) и 18BIT (Int_Time = 1.0): Lux = (0.6 / 3.0) * raw = raw * 0.2
          sensors.lux = (float)raw_als * 0.2;
          sensors.ltr_updated = millis();
        }
        configureLtrMode(true); // Переключаем обратно на измерение УФ (UVS)
      }
    }
  }
  // Если датчик не обновлялся более 10 секунд — помечаем его как неактивный
  sensors.ltr_ok = (sensors.ltr_updated > 0) && (millis() - sensors.ltr_updated < 10000);
}

//////////////////////////////////////////////////////////////////////////
// Редирект всех неизвестных URL на главную страницу (Captive Portal)
//////////////////////////////////////////////////////////////////////////
void handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "");
}