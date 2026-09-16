//////////////////////////////////////////////////////////////////////////
// main.cpp
//////////////////////////////////////////////////////////////////////////
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>

#include "SparkFun_SCD4x_Arduino_Library.h"

#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#include "index.h"
#include "config.h"
#include "Adafruit_LTR390.h"


SCD4x COSensor;

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme(&Wire); // I2C

Adafruit_LTR390 ltr = Adafruit_LTR390();

WebServer server(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;

const char* ssid     = "ESP_Network";
const char* password = ""; 

void handleNotFound();


void setup() {
  Serial.begin(115200);
  ////////////////////////////////////////////////////////////////////////
  // Sensors


  Wire.begin(SDA, SCL);
  // Co2
  COSensor.begin();
  //COSensor.startPeriodicMeasurement();
  // Bme
  bme.begin();

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  //bme.performReading();

  //Uf
  ltr.begin();
  ltr.setMode(LTR390_MODE_UVS);

  ltr.setGain(LTR390_GAIN_3);
  ltr.setResolution(LTR390_RESOLUTION_16BIT);

  ltr.setThresholds(100, 1000);


  ////////////////////////////////////////////////////////////////////////
  // Wifi
  
  WiFi.softAP(ssid, password);
  Serial.println("");
  Serial.print("IP адрес точки доступа: ");
  Serial.println(WiFi.softAPIP());

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  server.on("/", []() {
    server.send(200, "text/html", index_html);
  });



server.on("/api/chip-info", []() {
    String json = "{";
    json += "\"uptime\":" + String(millis() / 1000) + ","; 
    json += "\"ram\":" + String(ESP.getFreeHeap()) + ",";  
    json += "\"cpu\":" + String(ESP.getCpuFreqMHz()) + ",";      
    json += "\"mac\":\"" + WiFi.softAPmacAddress() + "\",";
    json += "\"g\":\"" + String(ESP.getChipModel()) + "\","; 

    // Данные с датчика CO2
    json += "\"CO2\":" + String(COSensor.getCO2()) + ","; 
    json += "\"temp\":" + String(COSensor.getTemperature()) + ","; 
    json += "\"hum\":" + String(COSensor.getHumidity()) + ","; // <-- Добавлена запятая!
  

    // Данные с датчика BME680 (предполагаем, что объект называется bme)
    json += "\"temp_b\":" + String(bme.temperature, 2) + ","; 
    json += "\"hum_b\":" + String(bme.humidity, 2) + ","; 
    json += "\"pressure_b\":" + String(bme.pressure / 100.0, 2) + ","; 
    
    // Делим на 1000, чтобы получить кОм, округляем до 2 знаков
    json += "\"gas_b\":" + String(bme.gas_resistance / 1000.0, 2) + ","; 

    json += "\"uf\":" + String(ltr.readUVS()); 

    json += "}";
    
    server.send(200, "application/json", json);
});

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("Веб-сервер запущен.");


  // Sensors

}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
}


//////////////////////////////////////////////////////////////////////////
// Функции
//////////////////////////////////////////////////////////////////////////

void handleNotFound() {
  server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString(), true);
  server.send(302, "text/plain", "");
}

