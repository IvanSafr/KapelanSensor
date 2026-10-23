//////////////////////////////////////////////////////////////////////////
// index.h
//////////////////////////////////////////////////////////////////////////
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Информация о ESP</title>
    <style>
        body { font-family: Arial, sans-serif; background-color: #f4f4f9; text-align: center; margin-top: 50px; }
        .card { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: inline-block; text-align: left; margin: 10px; vertical-align: top; }
        h1 { color: #333; }
        h3 { color: #555; margin-top: 16px; margin-bottom: 4px; }
        .data-row { margin: 10px 0; font-size: 18px; }
        .val { font-weight: bold; color: #007BFF; }
    </style>
</head>
<body>
    <div class="card">
        <h1>Данные сенсоров</h1>

        <h3>CO<sub>2</sub> (SCD4x)</h3>
        <div class="data-row">CO<sub>2</sub>: <span id="CO2" class="val">Загрузка...</span> ppm</div>
        <div class="data-row">Температура: <span id="temp" class="val">Загрузка...</span> °C</div>
        <div class="data-row">Влажность: <span id="hum" class="val">Загрузка...</span> %</div>

        <h3>BME680</h3>
        <div class="data-row">Температура: <span id="temp_b" class="val">Загрузка...</span> °C</div>
        <div class="data-row">Влажность: <span id="hum_b" class="val">Загрузка...</span> %</div>
        <div class="data-row">Давление: <span id="pressure_b" class="val">Загрузка...</span> гПа</div>
        <div class="data-row">Газ (сопротивление): <span id="gas_b" class="val">Загрузка...</span> кОм</div>

        <h3>LTR390 (УФ)</h3>
        <div class="data-row">УФ-индекс (UVS): <span id="uf" class="val">Загрузка...</span></div>
    </div>

    <div class="card">
        <h1>Статус чипа ESP</h1>
        <div class="data-row">Время работы: <span id="uptime" class="val">Загрузка...</span> сек.</div>
        <div class="data-row">Свободная ОЗУ: <span id="ram" class="val">Загрузка...</span> байт</div>
        <div class="data-row">Частота CPU: <span id="cpu" class="val">Загрузка...</span> МГц</div>
        <div class="data-row">MAC-адрес: <span id="mac" class="val">Загрузка...</span></div>
        <div class="data-row">Модель чипа: <span id="g" class="val">Загрузка...</span></div>
    </div>

    <script>
        function fetchChipData() {
            fetch('/api/chip-info')
                .then(response => response.json())
                .then(data => {
                    // Чип
                    document.getElementById('uptime').innerText = data.uptime;
                    document.getElementById('ram').innerText    = data.ram;
                    document.getElementById('cpu').innerText    = data.cpu;
                    document.getElementById('mac').innerText    = data.mac;
                    document.getElementById('g').innerText      = data.g;

                    // SCD4x
                    document.getElementById('CO2').innerText  = data.CO2;
                    document.getElementById('temp').innerText = data.temp;
                    document.getElementById('hum').innerText  = data.hum;

                    // BME680
                    document.getElementById('temp_b').innerText     = data.temp_b;
                    document.getElementById('hum_b').innerText      = data.hum_b;
                    document.getElementById('pressure_b').innerText = data.pressure_b;
                    document.getElementById('gas_b').innerText      = data.gas_b;

                    // LTR390
                    document.getElementById('uf').innerText = data.uf;
                })
                .catch(error => console.error('Ошибка:', error));
        }
        setInterval(fetchChipData, 1000);
        window.onload = fetchChipData;
    </script>
</body>
</html>
)rawliteral";