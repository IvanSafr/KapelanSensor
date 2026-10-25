//////////////////////////////////////////////////////////////////////////
// index.h — Полный веб-интерфейс дашборда ENV Station v15
// -----------------------------------------------------------------------
// ВАЖНО ДЛЯ РАЗРАБОТЧИКА:
// 1. Хранится во Flash-памяти (PROGMEM), чтобы не занимать оперативную память (Heap).
// 2. В качестве ограничителя Raw String Literal используется `R"=====( ... )====="`
//    вместо стандартного `R"rawliteral(...)rawliteral"`, так как внутри JS-кода
//    встречаются конструкции вида `join('')}</div>")`, которые могли бы конфликтовать
//    с простыми скобками.
// 3. Вся логика графиков (Canvas), конвертера величин, истории сессии и экспорта CSV
//    выполняется на стороне браузера клиента, не нагружая процессор ESP32.
//////////////////////////////////////////////////////////////////////////
#pragma once
#include <Arduino.h>

const char index_html[] PROGMEM = R"=====(
<!doctype html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0b0f14">
<title>ENV Station</title>
<style>
/* Цветовая палитра и дизайн-система дашборда (Dark Mode) */
:root{
  --bg:#0b0f14;--panel:#121821;--panel2:#18212d;--text:#f2f6fb;
  --muted:#8d9aae;--line:#263243;--ok:#38d996;--warn:#ffbf47;
  --bad:#ff6678;--accent:#72a7ff;--shadow:0 14px 36px rgba(0,0,0,.28)
}
*{box-sizing:border-box}
html{background:var(--bg);color-scheme:dark}
body{
  margin:0;min-height:100vh;
  background:
    radial-gradient(circle at 8% -10%,rgba(66,117,255,.18),transparent 34%),
    radial-gradient(circle at 100% 12%,rgba(56,217,150,.10),transparent 28%),
    var(--bg);
  color:var(--text);
  font:15px/1.45 system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Arial,sans-serif
}
button{font:inherit}
.app{width:min(980px,100%);margin:auto;padding:18px 14px 34px}
header{display:flex;align-items:center;justify-content:space-between;gap:14px;margin-bottom:16px}
.brand h1{font-size:20px;letter-spacing:.08em;margin:0}
.brand p{margin:3px 0 0;color:var(--muted);font-size:12px}
.connection{
  display:flex;align-items:center;gap:8px;padding:8px 11px;border:1px solid var(--line);
  border-radius:999px;background:#121821d6;white-space:nowrap;font-size:12px
}
.dot{width:9px;height:9px;border-radius:50%;background:var(--warn);box-shadow:0 0 0 4px rgba(255,191,71,.12)}
.dot.ok{background:var(--ok);box-shadow:0 0 0 4px rgba(56,217,150,.12)}
.dot.bad{background:var(--bad);box-shadow:0 0 0 4px rgba(255,102,120,.12)}
.tabs{
  display:grid;grid-template-columns:repeat(5,minmax(104px,1fr));gap:7px;background:#121821bf;
  padding:6px;border:1px solid var(--line);border-radius:17px;margin-bottom:16px;
  position:sticky;top:8px;z-index:5;backdrop-filter:blur(12px);
  overflow-x:auto;scrollbar-width:none
}
.tabs::-webkit-scrollbar{display:none}
.tab{border:0;border-radius:12px;padding:10px 5px;color:var(--muted);background:transparent;cursor:pointer}
.tab.active{background:var(--panel2);color:var(--text)}
.view{display:none}
.view.active{display:block}
.hero{
  border:1px solid var(--line);border-radius:28px;padding:24px;
  background:linear-gradient(145deg,#18212dfa,#0f151dfa);box-shadow:var(--shadow);
  margin-bottom:14px;text-align:center;position:relative;overflow:hidden
}
.hero:before{
  content:"";position:absolute;inset:-40% 20% auto -15%;height:220px;
  background:radial-gradient(circle,rgba(114,167,255,.18),transparent 65%)
}
.hero-label,.hero-value,.hero-status,.hero-time{position:relative}
.hero-label{color:var(--muted);text-transform:uppercase;letter-spacing:.14em;font-size:12px}
.hero-value{font-size:clamp(54px,12vw,92px);line-height:1;font-weight:750;margin:14px 0 8px}
.hero-value span{font-size:.26em;font-weight:600;color:var(--muted);margin-left:6px}
.hero-status{
  display:inline-flex;align-items:center;gap:7px;padding:8px 13px;border-radius:999px;
  background:rgba(56,217,150,.12);color:var(--ok)
}
.hero-status.warn{background:rgba(255,191,71,.12);color:var(--warn)}
.hero-status.bad{background:rgba(255,102,120,.12);color:var(--bad)}
.hero-time{color:var(--muted);font-size:12px;margin-top:12px}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:12px}
.card{
  border:1px solid var(--line);border-radius:22px;background:#121821ef;
  padding:16px;min-width:0
}
.info-card,.hero.info-card{cursor:pointer;transition:transform .12s ease,border-color .12s ease}
.info-card:active{transform:scale(.985)}
.info-card:focus-visible{outline:2px solid var(--accent);outline-offset:2px}
.metric-head{display:flex;justify-content:space-between;align-items:center;gap:8px;color:var(--muted);font-size:12px}
.metric-value{font-size:27px;font-weight:700;margin-top:12px;white-space:nowrap}
.unit{font-size:13px;color:var(--muted);font-weight:600}
.metric-note{font-size:11px;color:var(--muted);margin-top:5px;min-height:16px}
.tap-hint{font-size:10px;color:#627087;margin-top:8px}
.section-title{
  font-size:13px;color:var(--muted);letter-spacing:.1em;text-transform:uppercase;
  margin:20px 2px 10px
}
.chart-card{margin-bottom:12px}
.chart-top{display:flex;align-items:center;justify-content:space-between;gap:10px;margin-bottom:10px}
.chart-name{font-weight:700}.chart-range{color:var(--muted);font-size:11px}
canvas{width:100%;height:190px;display:block}
.sensor-row,.system-row{
  display:flex;align-items:center;justify-content:space-between;gap:12px;
  padding:13px 0;border-bottom:1px solid var(--line)
}
.sensor-row:last-child,.system-row:last-child{border-bottom:0}
.sensor-name strong,.system-row strong{display:block}
.sensor-name span,.system-row span{color:var(--muted);font-size:12px}
.pill{padding:5px 9px;border-radius:999px;font-size:11px;background:rgba(141,154,174,.12);color:var(--muted)}
.pill.ok{background:rgba(56,217,150,.12);color:var(--ok)}
.pill.bad{background:rgba(255,102,120,.12);color:var(--bad)}
.notice{
  border:1px dashed var(--line);border-radius:18px;padding:14px;
  color:var(--muted);font-size:12px;margin-top:12px
}
.capabilities{display:grid;gap:10px;margin-top:12px}
.capability{
  border:1px solid var(--line);border-radius:16px;padding:12px;background:#0f151d
}
.capability strong{display:block;margin-bottom:5px}
.capability span{font-size:12px;color:var(--muted)}
.learn-grid{display:grid;gap:12px}
.learn-card{
  border:1px solid var(--line);border-radius:22px;background:#121821ef;
  padding:16px
}
.learn-card h3{margin:0 0 7px;font-size:17px}
.learn-card p{margin:0;color:var(--muted);font-size:13px}
.fact-line{display:flex;gap:11px;align-items:flex-start;padding:12px 0;border-bottom:1px solid var(--line)}
.fact-line:last-child{border-bottom:0}
.fact-year{
  min-width:58px;padding:5px 7px;border-radius:10px;background:rgba(114,167,255,.14);
  color:#a9c7ff;text-align:center;font-weight:700;font-size:11px
}
.fact-text strong{display:block;margin-bottom:3px}
.fact-text span{font-size:12px;color:var(--muted)}
.planet-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px}
.planet-card{
  border:1px solid var(--line);border-radius:18px;padding:14px;background:#0f151d
}
.planet-title{display:flex;align-items:center;gap:10px;margin-bottom:9px}
.planet-title h4{margin:0}
.planet-icon{
  width:42px;height:42px;flex:0 0 42px;display:block;
  filter:drop-shadow(0 5px 8px rgba(0,0,0,.28))
}

.planet-card h4{margin:0;font-size:16px}
.planet-value{font-size:12px;color:var(--muted);margin:5px 0}
.planet-value b{color:var(--text)}
.planet-note{
  margin-top:12px;padding:12px;border-radius:15px;background:rgba(255,191,71,.09);
  color:#ffd889;font-size:12px
}
@media(min-width:720px){
  .learn-grid{grid-template-columns:repeat(2,minmax(0,1fr))}
  .planet-grid{grid-template-columns:repeat(4,minmax(0,1fr))}
}

.learn-subtabs{
  display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:8px;
  margin-bottom:14px;padding:6px;border:1px solid var(--line);
  border-radius:17px;background:#121821bf
}
.learn-subtab{
  border:0;border-radius:12px;padding:10px 8px;background:transparent;
  color:var(--muted);cursor:pointer
}
.learn-subtab.active{background:var(--panel2);color:var(--text)}
.learn-panel{display:none}
.learn-panel.active{display:block}
.why-grid,.history-grid,.chip-grid{display:grid;gap:12px}
.why-card,.history-card,.chip-card{
  border:1px solid var(--line);border-radius:21px;background:#121821ef;padding:16px
}
.why-card h3,.history-card h3,.chip-card h3{margin:0 0 6px;font-size:17px}
.why-card p,.history-card p,.chip-card p{margin:0;color:var(--muted);font-size:13px}
.why-card .simple{color:var(--text);font-size:14px;margin-top:9px}
.why-icon{font-size:22px;margin-bottom:8px}
.history-meta,.chip-meta{
  display:flex;flex-wrap:wrap;gap:7px;margin:10px 0
}
.meta-pill{
  padding:5px 8px;border-radius:999px;background:rgba(114,167,255,.12);
  color:#aac8ff;font-size:11px
}
.recognition{
  margin-top:12px;padding:11px 12px;border-radius:14px;
  background:rgba(56,217,150,.08);color:#a9f1d1;font-size:12px
}
.neutral-note{
  margin-bottom:14px;padding:13px;border:1px dashed var(--line);
  border-radius:17px;color:var(--muted);font-size:12px
}
.source-box{
  margin-top:16px;padding:13px;border-radius:17px;background:#0d131b;
  border:1px solid var(--line)
}
.source-box summary{cursor:pointer;font-weight:700}
.source-box a{display:block;color:#a9c7ff;font-size:12px;margin-top:8px;overflow-wrap:anywhere}
@media(min-width:720px){
  .learn-subtabs{grid-template-columns:repeat(4,minmax(0,1fr))}
  .why-grid{grid-template-columns:repeat(2,minmax(0,1fr))}
  .history-grid{grid-template-columns:repeat(2,minmax(0,1fr))}
  .chip-grid{grid-template-columns:repeat(2,minmax(0,1fr))}
}

.calc-grid{display:grid;gap:12px}
.calc-card{
  border:1px solid var(--line);border-radius:21px;background:#121821ef;padding:16px
}
.calc-card h3{margin:0 0 5px;font-size:17px}
.calc-card>p{margin:0 0 14px;color:var(--muted);font-size:12px}
.calc-row{
  display:grid;grid-template-columns:1fr auto 1fr;align-items:end;gap:9px
}
.calc-field label{
  display:block;color:var(--muted);font-size:11px;margin:0 0 6px
}
.calc-field input{
  width:100%;min-width:0;border:1px solid var(--line);border-radius:13px;
  background:#0c1219;color:var(--text);padding:11px 10px;font:inherit;
  outline:none
}
.calc-field input:focus{border-color:var(--accent);box-shadow:0 0 0 3px rgba(114,167,255,.12)}
.calc-arrow{color:var(--muted);padding-bottom:11px}
.calc-formula{
  margin-top:11px;color:var(--muted);font-size:11px
}
@media(min-width:720px){
  .calc-grid{grid-template-columns:repeat(2,minmax(0,1fr))}
}

.summary-card{
  display:flex;align-items:flex-start;gap:14px;margin-bottom:14px;padding:17px;
  border:1px solid var(--line);border-radius:22px;background:#121821ef;
  box-shadow:var(--shadow)
}
.summary-card.ok{border-color:rgba(56,217,150,.34)}
.summary-card.warn{border-color:rgba(255,191,71,.4)}
.summary-card.bad{border-color:rgba(255,102,120,.44)}
.summary-icon{
  width:46px;height:46px;display:grid;place-items:center;flex:0 0 46px;
  border-radius:15px;background:rgba(114,167,255,.12);font-size:23px
}
.summary-card.ok .summary-icon{background:rgba(56,217,150,.12)}
.summary-card.warn .summary-icon{background:rgba(255,191,71,.12)}
.summary-card.bad .summary-icon{background:rgba(255,102,120,.12)}
.summary-kicker{font-size:10px;letter-spacing:.12em;text-transform:uppercase;color:var(--muted)}
.summary-title{font-size:20px;font-weight:760;margin-top:2px}
.summary-detail{font-size:12px;color:var(--muted);margin-top:5px}
.trend{
  display:inline-flex;align-items:center;justify-content:center;
  min-width:26px;margin-left:5px;font-size:18px;vertical-align:middle;
  color:var(--muted)
}
.hero-value .trend{font-size:.28em;margin-left:8px}
.trend.up{color:#a9c7ff}
.trend.down{color:#c4b1ff}
.trend.steady{color:var(--muted)}
.metric-range{font-size:10px;color:#6f7d91;margin-top:5px;min-height:15px}
.export-bar{
  display:flex;align-items:center;justify-content:space-between;gap:12px;
  margin-bottom:12px;padding:13px 14px;border:1px solid var(--line);
  border-radius:18px;background:#121821ef
}
.export-copy strong{display:block}
.export-copy span{display:block;color:var(--muted);font-size:11px;margin-top:2px}
.export-btn{
  border:1px solid rgba(114,167,255,.34);border-radius:13px;
  padding:9px 12px;background:rgba(114,167,255,.12);color:#bdd3ff;
  cursor:pointer;white-space:nowrap
}
.export-btn:disabled{opacity:.45;cursor:not-allowed}
.pill.warm{background:rgba(255,191,71,.12);color:var(--warn)}
.pill.cal{background:rgba(114,167,255,.12);color:#a9c7ff}
.sensor-detail{display:block!important;margin-top:3px;color:#6f7d91!important;font-size:11px!important}
@media(max-width:620px){
  .tabs{
    grid-template-columns:repeat(3,minmax(0,1fr));
    overflow:visible
  }
  .tab{font-size:12px;white-space:nowrap}
  .summary-title{font-size:18px}
  .export-bar{align-items:flex-start;flex-direction:column}
  .export-btn{width:100%}
}

.metric-condition{
  display:inline-flex;align-items:center;gap:6px;
  margin-top:9px;padding:6px 10px;border-radius:999px;
  background:rgba(141,154,174,.12);color:var(--muted);
  font-size:11px;font-weight:700
}
.metric-condition.ok{background:rgba(56,217,150,.12);color:var(--ok)}
.metric-condition.warn{background:rgba(255,191,71,.12);color:var(--warn)}
.metric-condition.bad{background:rgba(255,102,120,.12);color:var(--bad)}
.metric-condition.info{background:rgba(114,167,255,.12);color:#a9c7ff}
.summary-scope{
  margin-top:9px;color:#758398;font-size:10px;line-height:1.4
}

.modal-status-box{
  margin-top:15px;padding:12px;border:1px solid var(--line);
  border-radius:16px;background:#0c1219
}
.modal-status-top{
  display:flex;align-items:center;justify-content:space-between;gap:10px
}
.modal-status-label{
  color:var(--muted);font-size:10px;letter-spacing:.1em;text-transform:uppercase
}
.modal-current-value{
  color:var(--muted);font-size:12px;text-align:right
}
.modal-live-status{
  display:inline-flex;align-items:center;margin-top:7px;padding:6px 10px;
  border-radius:999px;background:rgba(141,154,174,.12);
  color:var(--muted);font-size:12px;font-weight:750
}
.modal-live-status.ok{background:rgba(56,217,150,.12);color:var(--ok)}
.modal-live-status.warn{background:rgba(255,191,71,.12);color:var(--warn)}
.modal-live-status.bad{background:rgba(255,102,120,.12);color:var(--bad)}
.modal-live-status.info{background:rgba(114,167,255,.12);color:#a9c7ff}
.modal-status-help{margin-top:8px;color:#cbd5e1;font-size:12px}

.dashboard-tools{
  display:flex;align-items:center;justify-content:space-between;gap:12px;
  margin-bottom:12px;padding:12px 14px;border:1px solid var(--line);
  border-radius:18px;background:#121821ef
}
.dashboard-tools-copy strong{display:block;font-size:13px}
.dashboard-tools-copy span{display:block;margin-top:3px;color:var(--muted);font-size:11px}
.metric-settings-button{
  display:inline-flex;align-items:center;justify-content:center;gap:8px;
  flex:0 0 auto;border:1px solid rgba(114,167,255,.34);
  border-radius:13px;padding:9px 12px;background:rgba(114,167,255,.12);
  color:#bdd3ff;font:inherit;font-size:12px;font-weight:750;cursor:pointer
}
.metric-settings-count{
  min-width:38px;padding:2px 7px;border-radius:999px;
  background:rgba(255,255,255,.08);font-size:10px;text-align:center
}
.metric-hidden{display:none!important}
.metric-empty,.chart-empty{
  display:none;margin-top:12px;padding:22px;border:1px dashed var(--line);
  border-radius:20px;background:rgba(18,24,33,.72);text-align:center
}
.metric-empty.show,.chart-empty.show{display:block}
.metric-empty strong{display:block;font-size:16px}
.metric-empty span,.chart-empty{
  color:var(--muted);font-size:12px;line-height:1.5
}
.metric-empty span{display:block;margin-top:5px}
.metric-empty button{
  margin-top:12px;border:1px solid rgba(114,167,255,.34);
  border-radius:12px;padding:9px 12px;background:rgba(114,167,255,.12);
  color:#bdd3ff;font:inherit;font-size:12px;font-weight:700;cursor:pointer
}
.settings-sheet{max-width:560px}
.settings-intro{margin:0 0 14px;color:var(--muted);font-size:12px;line-height:1.55}
.preset-row{display:flex;flex-wrap:wrap;gap:8px;margin-bottom:14px}
.preset-button{
  border:1px solid var(--line);border-radius:999px;padding:8px 11px;
  background:#111821;color:var(--text);font:inherit;font-size:11px;cursor:pointer
}
.preset-button.active{
  border-color:rgba(114,167,255,.55);
  background:rgba(114,167,255,.14);color:#bdd3ff
}
.metric-choice-list{display:grid;grid-template-columns:1fr 1fr;gap:9px}
.metric-choice{
  display:flex;align-items:center;gap:10px;padding:12px;
  border:1px solid var(--line);border-radius:15px;background:#0d141d;
  cursor:pointer;user-select:none
}
.metric-choice input{width:18px;height:18px;accent-color:#72a7ff;flex:0 0 auto}
.metric-choice-copy strong{display:block;font-size:13px}
.metric-choice-copy span{display:block;margin-top:2px;color:var(--muted);font-size:10px}
.settings-note{
  margin-top:14px;padding:11px 12px;border:1px solid rgba(255,191,71,.23);
  border-radius:14px;background:rgba(255,191,71,.07);
  color:#d8c69c;font-size:11px;line-height:1.5
}
.settings-actions{
  display:flex;align-items:center;justify-content:space-between;gap:10px;margin-top:15px
}
.settings-reset,.settings-done{
  border-radius:12px;padding:10px 13px;font:inherit;font-size:12px;
  font-weight:750;cursor:pointer
}
.settings-reset{border:1px solid var(--line);background:#111821;color:var(--muted)}
.settings-done{
  border:1px solid rgba(56,217,150,.35);
  background:rgba(56,217,150,.12);color:#92efc1
}
@media(max-width:620px){
  .dashboard-tools{align-items:flex-start;flex-direction:column}
  .metric-settings-button{width:100%}
  .metric-choice-list{grid-template-columns:1fr}
  .settings-actions{align-items:stretch;flex-direction:column-reverse}
  .settings-reset,.settings-done{width:100%}
}

.footer{color:var(--muted);font-size:11px;text-align:center;margin-top:20px}

/* Окно с пояснением */
.modal{
  position:fixed;inset:0;z-index:50;display:none;align-items:flex-end;
  background:rgba(3,6,10,.76);padding:14px
}
.modal.open{display:flex}
.modal-sheet{
  width:min(620px,100%);max-height:min(82vh,760px);overflow:auto;margin:0 auto;
  border:1px solid var(--line);border-radius:26px 26px 20px 20px;
  background:#101720;box-shadow:0 30px 90px rgba(0,0,0,.55);padding:18px
}
.modal-top{display:flex;align-items:flex-start;justify-content:space-between;gap:14px}
.modal-title{font-size:21px;font-weight:760;margin:0}
.modal-subtitle{color:var(--muted);font-size:12px;margin-top:4px}
.modal-close{
  width:38px;height:38px;border:1px solid var(--line);border-radius:50%;
  background:var(--panel2);color:var(--text);cursor:pointer;flex:0 0 auto
}
.info-block{margin-top:16px}
.info-block h3{font-size:12px;letter-spacing:.1em;text-transform:uppercase;color:var(--muted);margin:0 0 7px}
.info-block p{margin:0;color:#dbe4ef}
.unit-box{
  display:grid;grid-template-columns:auto 1fr;gap:9px 12px;align-items:start;
  border:1px solid var(--line);background:#0c1219;border-radius:16px;padding:12px
}
.unit-badge{
  padding:5px 8px;border-radius:10px;background:rgba(114,167,255,.14);
  color:#a9c7ff;font-weight:700;font-size:12px
}
.range-list{display:grid;gap:7px}
.range-item{
  display:grid;grid-template-columns:92px 1fr;gap:10px;padding:9px 10px;
  border-radius:13px;background:#0c1219
}
.range-item b{font-size:12px}.range-item span{font-size:12px;color:var(--muted)}
.warning{
  margin-top:14px;padding:11px 12px;border-radius:14px;
  background:rgba(255,191,71,.09);color:#ffd889;font-size:12px
}

@media(min-width:720px){
  .grid{grid-template-columns:repeat(3,minmax(0,1fr))}
  .app{padding-top:26px}
  .modal{align-items:center}
  .modal-sheet{border-radius:26px}
}
</style>
</head>
<body>

<!-- Встроенный SVG-спрайт для иконок планет Солнечной системы (работает полностью офлайн без интернета) -->
<svg width="0" height="0" style="position:absolute;overflow:hidden" aria-hidden="true">
  <defs>
    <radialGradient id="g-mercury" cx="35%" cy="28%">
      <stop offset="0" stop-color="#d7d0c3"/>
      <stop offset=".65" stop-color="#999083"/>
      <stop offset="1" stop-color="#665f57"/>
    </radialGradient>
    <radialGradient id="g-venus" cx="32%" cy="25%">
      <stop offset="0" stop-color="#fff0ad"/>
      <stop offset=".62" stop-color="#d8a849"/>
      <stop offset="1" stop-color="#8f5f22"/>
    </radialGradient>
    <radialGradient id="g-earth" cx="32%" cy="25%">
      <stop offset="0" stop-color="#74d7ff"/>
      <stop offset=".7" stop-color="#1774bd"/>
      <stop offset="1" stop-color="#0a386f"/>
    </radialGradient>
    <radialGradient id="g-moon" cx="32%" cy="25%">
      <stop offset="0" stop-color="#f1f1ec"/>
      <stop offset=".7" stop-color="#b8bab9"/>
      <stop offset="1" stop-color="#777b7f"/>
    </radialGradient>
    <radialGradient id="g-mars" cx="32%" cy="25%">
      <stop offset="0" stop-color="#ff9b68"/>
      <stop offset=".7" stop-color="#c84f2e"/>
      <stop offset="1" stop-color="#76261c"/>
    </radialGradient>
    <radialGradient id="g-jupiter" cx="32%" cy="25%">
      <stop offset="0" stop-color="#f5d7a6"/>
      <stop offset=".75" stop-color="#b77442"/>
      <stop offset="1" stop-color="#754426"/>
    </radialGradient>
    <radialGradient id="g-saturn" cx="32%" cy="25%">
      <stop offset="0" stop-color="#ffe6a3"/>
      <stop offset=".72" stop-color="#d2aa59"/>
      <stop offset="1" stop-color="#8e6b35"/>
    </radialGradient>
    <radialGradient id="g-uranus" cx="32%" cy="25%">
      <stop offset="0" stop-color="#d9ffff"/>
      <stop offset=".72" stop-color="#79d8df"/>
      <stop offset="1" stop-color="#3d8794"/>
    </radialGradient>
    <radialGradient id="g-neptune" cx="32%" cy="25%">
      <stop offset="0" stop-color="#7db4ff"/>
      <stop offset=".72" stop-color="#2557cc"/>
      <stop offset="1" stop-color="#15296f"/>
    </radialGradient>
    <radialGradient id="g-pluto" cx="32%" cy="25%">
      <stop offset="0" stop-color="#ead6c2"/>
      <stop offset=".72" stop-color="#9c7c65"/>
      <stop offset="1" stop-color="#59453a"/>
    </radialGradient>

    <clipPath id="clip-jupiter"><circle cx="32" cy="32" r="23"/></clipPath>
    <clipPath id="clip-neptune"><circle cx="32" cy="32" r="23"/></clipPath>

    <symbol id="planet-mercury" viewBox="0 0 64 64">
      <circle cx="32" cy="32" r="23" fill="url(#g-mercury)"/>
      <circle cx="22" cy="23" r="4.5" fill="#706960" opacity=".65"/>
      <circle cx="42" cy="20" r="3" fill="#847b70" opacity=".7"/>
      <circle cx="40" cy="40" r="6" fill="#746c62" opacity=".58"/>
      <circle cx="25" cy="43" r="2.5" fill="#d3ccc0" opacity=".5"/>
      <circle cx="32" cy="32" r="23" fill="none" stroke="#e1d9cb" stroke-opacity=".22"/>
    </symbol>

    <symbol id="planet-venus" viewBox="0 0 64 64">
      <circle cx="32" cy="32" r="23" fill="url(#g-venus)"/>
      <path d="M12 25c10-6 29-9 40-2M10 34c13 5 29 7 44 1M15 43c12-5 25-6 37-2" fill="none" stroke="#fff1b8" stroke-width="4" stroke-linecap="round" opacity=".55"/>
      <circle cx="32" cy="32" r="23" fill="none" stroke="#ffd77b" stroke-opacity=".3"/>
    </symbol>

    <symbol id="planet-earth" viewBox="0 0 64 64">
      <circle cx="32" cy="32" r="23" fill="url(#g-earth)"/>
      <path d="M17 20l8-5 8 3 3 6-4 4-1 7-7 2-5-5-6-2zM38 33l8-3 7 5-2 10-7 5-5-6-5-2z" fill="#70c86f"/>
      <path d="M12 28c11 3 14 1 21-2M30 14c7 4 13 4 22 2M18 47c8-3 16-3 25 1" fill="none" stroke="#f6fbff" stroke-width="2.6" stroke-linecap="round" opacity=".72"/>
      <circle cx="32" cy="32" r="23" fill="none" stroke="#9be5ff" stroke-opacity=".35"/>
    </symbol>

    <symbol id="planet-moon" viewBox="0 0 64 64">
      <circle cx="32" cy="32" r="23" fill="url(#g-moon)"/>
      <circle cx="21" cy="23" r="5" fill="#85898a" opacity=".62"/>
      <circle cx="42" cy="20" r="3.5" fill="#a1a3a2" opacity=".65"/>
      <circle cx="39" cy="42" r="7" fill="#8e9191" opacity=".5"/>
      <circle cx="23" cy="43" r="3" fill="#d9d9d4" opacity=".65"/>
      <circle cx="32" cy="32" r="23" fill="none" stroke="#fff" stroke-opacity=".24"/>
    </symbol>

    <symbol id="planet-mars" viewBox="0 0 64 64">
      <circle cx="32" cy="32" r="23" fill="url(#g-mars)"/>
      <path d="M16 23c7-5 14-3 18 1M34 42c7-6 13-4 17-1M21 48c4-2 7-2 10 0" fill="none" stroke="#7d271c" stroke-width="4" stroke-linecap="round" opacity=".55"/>
      <path d="M18 17c5-2 10-2 14 0" stroke="#f8d3ba" stroke-width="2.5" stroke-linecap="round" opacity=".7"/>
      <circle cx="32" cy="32" r="23" fill="none" stroke="#ffb18b" stroke-opacity=".25"/>
    </symbol>

    <symbol id="planet-jupiter" viewBox="0 0 64 64">
      <circle cx="32" cy="32" r="23" fill="url(#g-jupiter)"/>
      <g clip-path="url(#clip-jupiter)">
        <path d="M7 18h50M5 25h54M5 33h54M7 41h50M10 48h44" stroke="#f3d5a4" stroke-width="4" opacity=".72"/>
        <path d="M7 22h50M7 37h50M10 45h44" stroke="#8f4e32" stroke-width="2.5" opacity=".65"/>
        <ellipse cx="43" cy="37" rx="6.8" ry="3.5" fill="#a93f2d" opacity=".9"/>
      </g>
      <circle cx="32" cy="32" r="23" fill="none" stroke="#ffe8bf" stroke-opacity=".25"/>
    </symbol>

    <symbol id="planet-saturn" viewBox="0 0 64 64">
      <ellipse cx="32" cy="35" rx="29" ry="10" fill="none" stroke="#d9c59a" stroke-width="6" transform="rotate(-13 32 35)" opacity=".78"/>
      <ellipse cx="32" cy="35" rx="29" ry="10" fill="none" stroke="#7d6a4d" stroke-width="1.8" transform="rotate(-13 32 35)" opacity=".85"/>
      <circle cx="32" cy="31" r="18" fill="url(#g-saturn)"/>
      <path d="M16 27h33M15 34h34" stroke="#f5d487" stroke-width="2.2" opacity=".5"/>
      <path d="M5 37c10 7 42 11 55-4" fill="none" stroke="#ead9b2" stroke-width="4" transform="rotate(-13 32 35)" opacity=".92"/>
      <circle cx="32" cy="31" r="18" fill="none" stroke="#fff0b6" stroke-opacity=".3"/>
    </symbol>

    <symbol id="planet-uranus" viewBox="0 0 64 64">
      <ellipse cx="32" cy="32" rx="28" ry="8" fill="none" stroke="#b9f4f4" stroke-width="2.2" transform="rotate(70 32 32)" opacity=".55"/>
      <circle cx="32" cy="32" r="23" fill="url(#g-uranus)"/>
      <path d="M13 31c13-3 25-3 38 0" fill="none" stroke="#dcffff" stroke-width="2" opacity=".4"/>
      <circle cx="32" cy="32" r="23" fill="none" stroke="#eaffff" stroke-opacity=".35"/>
    </symbol>

    <symbol id="planet-neptune" viewBox="0 0 64 64">
      <circle cx="32" cy="32" r="23" fill="url(#g-neptune)"/>
      <g clip-path="url(#clip-neptune)">
        <path d="M7 25c12 3 34 3 50 0M7 39c15-4 34-3 50 1" fill="none" stroke="#80a8ff" stroke-width="3" opacity=".5"/>
        <ellipse cx="43" cy="37" rx="5" ry="2.6" fill="#142b83" opacity=".8"/>
      </g>
      <circle cx="32" cy="32" r="23" fill="none" stroke="#9cc2ff" stroke-opacity=".3"/>
    </symbol>

    <symbol id="planet-pluto" viewBox="0 0 64 64">
      <circle cx="32" cy="32" r="23" fill="url(#g-pluto)"/>
      <path d="M30 19c-5-5-13-1-11 6 2 7 10 11 13 15 4-5 12-8 14-15 2-7-7-11-12-5-1 1-2 2-2 3-1-2-1-3-2-4z" fill="#d8c1aa" opacity=".9"/>
      <path d="M15 37c8 4 22 8 35 3" fill="none" stroke="#6d5342" stroke-width="3" opacity=".45"/>
      <circle cx="32" cy="32" r="23" fill="none" stroke="#f2dfcc" stroke-opacity=".25"/>
    </symbol>
  </defs>
</svg>

<main class="app">
<!-- Шапка приложения без плашки "Концепт для обсуждения" -->
<header>
  <div class="brand">
    <h1>ENV STATION</h1>
    <p>XIAO ESP32-S3 · локальный монитор среды</p>
  </div>
  <div class="connection">
    <span id="connectionDot" class="dot"></span>
    <span id="connectionText">Подключение…</span>
  </div>
</header>

<!-- Навигация по 5 основным разделам дашборда -->
<nav class="tabs">
  <button class="tab active" data-view="dashboard">Показания</button>
  <button class="tab" data-view="charts">Графики</button>
  <button class="tab" data-view="calculator">Калькулятор</button>
  <button class="tab" data-view="learn">Узнать больше</button>
  <button class="tab" data-view="system">Система</button>
</nav>

<!-- Вкладка 1: Главный экран показаний -->
<section id="dashboard" class="view active">

  <div class="dashboard-tools">
    <div class="dashboard-tools-copy">
      <strong>Нужные показатели</strong>
      <span>Скрой лишнее и оставь только то, чем пользуешься.</span>
    </div>
    <button id="openMetricSettings" class="metric-settings-button" type="button">
      Настроить
      <span id="visibleMetricCount" class="metric-settings-count">7 из 7</span>
    </button>
  </div>


  <article id="environmentSummary" class="summary-card">
    <div id="summaryIcon" class="summary-icon">◌</div>
    <div>
      <div class="summary-kicker">Состояние среды</div>
      <div id="summaryTitle" class="summary-title">Ожидание данных</div>
      <div id="summaryDetail" class="summary-detail">Станция ещё не получила показания.</div>
      <div class="summary-scope">Оценка только по установленным датчикам. CO, дым и частицы PM2.5 не измеряются.</div>
    </div>
  </article>

  <article class="hero info-card" tabindex="0" role="button" data-info="co2" data-metric-card="co2">
    <div class="hero-label">Углекислый газ</div>
    <div class="hero-value"><b id="co2">—</b><span>ppm</span></div>
    <div id="co2Status" class="hero-status">Ожидание данных</div>
    <div id="co2Range" class="metric-range">За текущую сессию: —</div>
    <div id="lastUpdate" class="hero-time">Последнее обновление: —</div>
    <div class="tap-hint">Нажми, чтобы открыть пояснение</div>
  </article>

  <div class="grid">
    <article class="card info-card" tabindex="0" role="button" data-info="temperature" data-metric-card="temperature">
      <div class="metric-head"><span>Температура</span><span>🌡</span></div>
      <div class="metric-value"><span id="temperature">—</span> <span class="unit">°C</span></div>
      <div id="temperatureStatus" class="metric-condition">Ожидание данных</div>
      <div id="temperatureRange" class="metric-range">За текущую сессию: —</div>
      <div class="metric-note">Основное значение: SCD41</div>
      <div class="tap-hint">Зачем измерять и что значит °C</div>
    </article>

    <article class="card info-card" tabindex="0" role="button" data-info="humidity" data-metric-card="humidity">
      <div class="metric-head"><span>Влажность</span><span>💧</span></div>
      <div class="metric-value"><span id="humidity">—</span> <span class="unit">%</span></div>
      <div id="humidityStatus" class="metric-condition">Ожидание данных</div>
      <div id="humidityRange" class="metric-range">За текущую сессию: —</div>
      <div class="metric-note">Относительная влажность</div>
      <div class="tap-hint">Зачем измерять и что значит %RH</div>
    </article>

    <article class="card info-card" tabindex="0" role="button" data-info="pressure" data-metric-card="pressure">
      <div class="metric-head"><span>Давление</span><span>◉</span></div>
      <div class="metric-value"><span id="pressure">—</span> <span class="unit">гПа</span></div>
      <div id="pressureStatus" class="metric-condition">Ожидание данных</div>
      <div id="pressureRange" class="metric-range">За текущую сессию: —</div>
      <div class="metric-note"><span id="pressureMmhg">—</span> мм рт. ст. · BME680</div>
      <div class="tap-hint">Зачем измерять и как связаны гПа и мм рт. ст.</div>
    </article>

    <article class="card info-card" tabindex="0" role="button" data-info="gas" data-metric-card="gas">
      <div class="metric-head"><span>Газовый сенсор</span><span>≈</span></div>
      <div class="metric-value"><span id="gas">—</span> <span class="unit">кОм</span></div>
      <div id="gasStatus" class="metric-condition">Собираем фоновый уровень</div>
      <div id="gasRange" class="metric-range">За текущую сессию: —</div>
      <div class="metric-note">Сопротивление BME680</div>
      <div class="tap-hint">Почему это не «VOC ppm»</div>
    </article>

    <article class="card info-card" tabindex="0" role="button" data-info="lux" data-metric-card="lux">
      <div class="metric-head"><span>Освещённость</span><span>☀</span></div>
      <div class="metric-value"><span id="lux">—</span> <span class="unit">лк</span></div>
      <div id="luxStatus" class="metric-condition">Ожидание данных</div>
      <div id="luxRange" class="metric-range">За текущую сессию: —</div>
      <div class="metric-note">LTR390, канал ALS</div>
      <div class="tap-hint">Зачем измерять и что значит люкс</div>
    </article>

    <article class="card info-card" tabindex="0" role="button" data-info="uv" data-metric-card="uv">
      <div class="metric-head"><span>Ультрафиолет</span><span>UV</span></div>
      <div class="metric-value"><span id="uv">—</span> <span id="uvUnit" class="unit">raw</span></div>
      <div id="uvStatus" class="metric-condition">Ожидание данных</div>
      <div id="uvRange" class="metric-range">За текущую сессию: —</div>
      <div class="metric-note">LTR390, канал UVS</div>
      <div class="tap-hint">Разница между raw и UVI</div>
    </article>
  </div>

  <div id="metricEmptyState" class="metric-empty">
    <strong>Все карточки скрыты</strong>
    <span>Датчики продолжают измерять, а общая оценка состояния среды остаётся активной.</span>
    <button id="showAllMetricsFromEmpty" type="button">Показать все показатели</button>
  </div>

</section>

<!-- Вкладка 2: Графики в реальном времени и выгрузка CSV -->
<section id="charts" class="view">

  <div class="export-bar">
    <div class="export-copy">
      <strong>История измерений</strong>
      <span id="sessionCount">Пока нет сохранённых точек</span>
    </div>
    <button id="exportCsv" class="export-btn" type="button" disabled>Скачать CSV</button>
  </div>
  <article class="card chart-card" data-metric-chart="co2">
    <div class="chart-top"><div class="chart-name">CO₂</div><div class="chart-range">последние 60 точек</div></div>
    <canvas id="co2Chart"></canvas>
  </article>
  <article class="card chart-card" data-metric-chart="temperature">
    <div class="chart-top"><div class="chart-name">Температура</div><div class="chart-range">последние 60 точек</div></div>
    <canvas id="tempChart"></canvas>
  </article>
  <article class="card chart-card" data-metric-chart="humidity">
    <div class="chart-top"><div class="chart-name">Влажность</div><div class="chart-range">последние 60 точек</div></div>
    <canvas id="humidityChart"></canvas>
  </article>
  <div id="chartEmptyState" class="chart-empty">
    Для выбранных показателей в этой версии пока нет отдельных графиков.
    Измерения всё равно продолжают сохраняться в истории и CSV.
  </div>
</section>

<!-- Вкладка 3: Интерактивный калькулятор перевода физических единиц -->
<section id="calculator" class="view">
  <article class="card" style="margin-bottom:12px">
    <div class="section-title" style="margin-top:0">Зачем он вообще нужен</div>
    <p style="margin:0;color:var(--text);font-size:14px">
      Датчики, погодные приложения и зарубежные сайты часто показывают одну и ту же величину
      в разных единицах. Калькулятор просто переводит число в привычный формат, чтобы не считать вручную.
    </p>
    <div class="notice" style="margin-top:12px">
      Перевод единиц не создаёт новое измерение и не делает его точнее.
      Это та же самая величина, только записанная другим способом.
    </div>
  </article>

  <div class="calc-grid">
    <article class="calc-card">
      <h3>Давление</h3>
      <p><b>Зачем:</b> BME680 и метеосайты используют гПа, а многие привыкли к мм рт. ст.</p>
      <div class="calc-row">
        <div class="calc-field">
          <label for="calcHpa">гПа</label>
          <input id="calcHpa" inputmode="decimal" type="number" step="0.1" value="1013.25">
        </div>
        <div class="calc-arrow">⇄</div>
        <div class="calc-field">
          <label for="calcMmhg">мм рт. ст.</label>
          <input id="calcMmhg" inputmode="decimal" type="number" step="0.1" value="760">
        </div>
      </div>
      <div class="calc-formula">1013 гПа ≈ 760 мм рт. ст.</div>
    </article>

    <article class="calc-card">
      <h3>CO₂</h3>
      <p><b>Зачем:</b> датчик показывает ppm, а в статьях состав атмосферы часто указан в процентах.</p>
      <div class="calc-row">
        <div class="calc-field">
          <label for="calcPpm">ppm</label>
          <input id="calcPpm" inputmode="decimal" type="number" step="1" value="1000">
        </div>
        <div class="calc-arrow">⇄</div>
        <div class="calc-field">
          <label for="calcPercent">%</label>
          <input id="calcPercent" inputmode="decimal" type="number" step="0.0001" value="0.1">
        </div>
      </div>
      <div class="calc-formula">10 000 ppm = 1%</div>
    </article>

    <article class="calc-card">
      <h3>Газовое сопротивление</h3>
      <p><b>Зачем:</b> BME680 может отдавать большое число в омах, а в килоомах его проще читать.</p>
      <div class="calc-row">
        <div class="calc-field">
          <label for="calcOhm">Ом</label>
          <input id="calcOhm" inputmode="decimal" type="number" step="1" value="126000">
        </div>
        <div class="calc-arrow">⇄</div>
        <div class="calc-field">
          <label for="calcKohm">кОм</label>
          <input id="calcKohm" inputmode="decimal" type="number" step="0.1" value="126">
        </div>
      </div>
      <div class="calc-formula">1000 Ом = 1 кОм</div>
    </article>

    <article class="calc-card">
      <h3>Температура</h3>
      <p><b>Зачем:</b> российские источники используют °C, а американские сайты часто показывают °F.</p>
      <div class="calc-row">
        <div class="calc-field">
          <label for="calcCelsius">°C</label>
          <input id="calcCelsius" inputmode="decimal" type="number" step="0.1" value="23">
        </div>
        <div class="calc-arrow">⇄</div>
        <div class="calc-field">
          <label for="calcFahrenheit">°F</label>
          <input id="calcFahrenheit" inputmode="decimal" type="number" step="0.1" value="73.4">
        </div>
      </div>
      <div class="calc-formula">°F = °C × 1,8 + 32</div>
    </article>
  </div>

  <div class="notice">
    Все расчёты выполняет браузер телефона. ESP32 только отдаёт страницу,
    поэтому отдельная вкладка почти не добавляет нагрузки микроконтроллеру.
  </div>
</section>

<!-- Вкладка 4: Энциклопедия (Зачем измерять, Ученые, Чипы, Планеты) -->
<section id="learn" class="view">
  <nav class="learn-subtabs" aria-label="Материалы">
    <button class="learn-subtab active" data-learn="why">Зачем измерять</button>
    <button class="learn-subtab" data-learn="history">Учёные</button>
    <button class="learn-subtab" data-learn="chips">Наши чипы</button>
    <button class="learn-subtab" data-learn="planets">Солнечная система</button>
  </nav>

  <div id="learn-why" class="learn-panel active">
    <div class="why-grid">
      <article class="why-card">
        <div class="why-icon">🌬️</div>
        <h3>CO₂</h3>
        <p class="simple">Показывает, хватает ли комнате свежего воздуха.</p>
        <p>Когда людей много и окна закрыты, CO₂ растёт. Значит, пора проветрить.</p>
      </article>

      <article class="why-card">
        <div class="why-icon">🌡️</div>
        <h3>Температура</h3>
        <p class="simple">Показывает, холодно, комфортно или жарко.</p>
        <p>Ещё температура влияет на влажность и на работу остальных датчиков.</p>
      </article>

      <article class="why-card">
        <div class="why-icon">💧</div>
        <h3>Влажность</h3>
        <p class="simple">Показывает, воздух сухой или сырой.</p>
        <p>Сухой воздух неприятен для кожи и слизистых, а сырой помогает плесени.</p>
      </article>

      <article class="why-card">
        <div class="why-icon">◉</div>
        <h3>Давление</h3>
        <p class="simple">Помогает замечать смену погоды.</p>
        <p>Падение часто бывает перед ухудшением погоды, рост — перед более спокойной.</p>
      </article>

      <article class="why-card">
        <div class="why-icon">≈</div>
        <h3>Газовый канал</h3>
        <p class="simple">Замечает, что состав воздуха изменился.</p>
        <p>Может отреагировать на готовку, спирт, аэрозоль или сильный запах, но не умеет назвать конкретный газ.</p>
      </article>

      <article class="why-card">
        <div class="why-icon">☀️</div>
        <h3>Освещённость</h3>
        <p class="simple">Показывает, достаточно ли света.</p>
        <p>Полезно для рабочего места, автоматической подсветки и сравнения дня и ночи.</p>
      </article>

      <article class="why-card">
        <div class="why-icon">UV</div>
        <h3>Ультрафиолет</h3>
        <p class="simple">Показывает силу невидимой части солнечного света.</p>
        <p>На улице помогает следить за UV-индексом. За стеклом и в помещении показания обычно ниже.</p>
      </article>
    </div>
  </div>

  <div id="learn-history" class="learn-panel">
    <div class="neutral-note">
      У многих учёных ниже не было Нобелевских премий: они жили раньше появления современных научных наград.
      Поэтому здесь указаны и награды, и другое признание — членство в академиях, названные в их честь единицы и место в истории науки.
    </div>

    <div class="history-grid">
      <article class="history-card">
        <h3>Джозеф Блэк</h3>
        <div class="history-meta"><span class="meta-pill">CO₂</span><span class="meta-pill">1750-е</span></div>
        <p>Нагревая известняк и изучая выделившийся газ, Блэк понял, что это отдельное вещество. Он называл его «связанным воздухом» — сегодня мы знаем его как углекислый газ.</p>
        <div class="recognition"><b>Признание:</b> один из основателей Королевского общества Эдинбурга; также известен работами о скрытой и удельной теплоте.</div>
      </article>

      <article class="history-card">
        <h3>Андерс Цельсий</h3>
        <div class="history-meta"><span class="meta-pill">Температура</span><span class="meta-pill">1742</span></div>
        <p>Цельсий предложил шкалу со ста делениями между замерзанием и кипением воды. В его первом варианте 0 означал кипение, а 100 — замерзание; позже шкалу перевернули.</p>
        <div class="recognition"><b>Признание:</b> шкала и единица °C названы в его честь; он был профессором Уппсальского университета и участвовал в создании его обсерватории.</div>
      </article>

      <article class="history-card">
        <h3>Эванджелиста Торричелли</h3>
        <div class="history-meta"><span class="meta-pill">Давление</span><span class="meta-pill">1643</span></div>
        <p>Торричелли наполнил трубку ртутью и увидел, что столб удерживает не «сила пустоты», а давление воздуха. Так появился ртутный барометр.</p>
        <div class="recognition"><b>Признание:</b> единица давления «торр» названа в его честь; его опыт стал основой барометрии.</div>
      </article>

      <article class="history-card">
        <h3>Орас Бенедикт де Соссюр</h3>
        <div class="history-meta"><span class="meta-pill">Влажность</span><span class="meta-pill">1783</span></div>
        <p>Он заметил, что человеческий волос становится длиннее во влажном воздухе и короче в сухом. На этом эффекте Соссюр построил волосной гигрометр.</p>
        <div class="recognition"><b>Признание:</b> его гигрометр стал классическим научным прибором и сохранился в музейных коллекциях.</div>
      </article>

      <article class="history-card">
        <h3>Пьер Бугер</h3>
        <div class="history-meta"><span class="meta-pill">Свет</span><span class="meta-pill">1729</span></div>
        <p>Бугер одним из первых начал не просто смотреть на свет, а сравнивать его яркость числами. Его книга заложила основы фотометрии — науки об измерении света.</p>
        <div class="recognition"><b>Признание:</b> его часто называют «отцом фотометрии»; он был членом Французской академии наук.</div>
      </article>

      <article class="history-card">
        <h3>Иоганн Вильгельм Риттер</h3>
        <div class="history-meta"><span class="meta-pill">Ультрафиолет</span><span class="meta-pill">1801</span></div>
        <p>Риттер проверил область сразу за фиолетовым краем солнечного спектра. Невидимые лучи там особенно быстро затемняли хлорид серебра — так обнаружили ультрафиолет.</p>
        <div class="recognition"><b>Признание:</b> в 1804 году его избрали членом Баварской академии наук.</div>
      </article>
    </div>

    <details class="source-box">
      <summary>Источники раздела</summary>
      <a href="https://www.rcpe.ac.uk/heritage/joseph-black">Royal College of Physicians of Edinburgh — Joseph Black</a>
      <a href="https://www.uu.se/en/about-uu/history/prominent-people/anders-celsius">Uppsala University — Anders Celsius</a>
      <a href="https://www.si.edu/object/hygrometer%3Anmah_1865188">Smithsonian — hygrometer of de Saussure</a>
      <a href="https://www.aps.org/publications/apsnews/201108/physicshistory.cfm">American Physical Society — Pierre Bouguer and photometry</a>
      <a href="https://www.sciencedirect.com/science/article/pii/S003936810900020X">Historical Studies in the Natural Sciences — Ritter and ultraviolet</a>
    </details>
  </div>

  <div id="learn-chips" class="learn-panel">
    <div class="chip-grid">
      <article class="chip-card">
        <h3>Sensirion · SCD41</h3>
        <div class="chip-meta"><span class="meta-pill">Швейцария</span><span class="meta-pill">CO₂</span></div>
        <p>Sensirion появилась в 1998 году как проект, выросший из ETH Zurich. Компанию основали Мориц Лехнер и Феликс Майер. Семейство миниатюрных SCD4x вышло на мировой рынок в 2021 году.</p>
        <div class="recognition"><b>В нашей станции:</b> SCD41 измеряет настоящий CO₂, а также температуру и влажность.</div>
      </article>

      <article class="chip-card">
        <h3>Bosch Sensortec · BME680</h3>
        <div class="chip-meta"><span class="meta-pill">Германия</span><span class="meta-pill">4 в 1</span></div>
        <p>Bosch Sensortec основали в 2005 году небольшой командой, которая занималась миниатюрными MEMS-датчиками для электроники. Первая версия документации BME680 датирована июлем 2017 года.</p>
        <div class="recognition"><b>В нашей станции:</b> давление, дополнительная температура и влажность, а также газовое сопротивление.</div>
      </article>

      <article class="chip-card">
        <h3>LITEON · LTR390</h3>
        <div class="chip-meta"><span class="meta-pill">Тайвань</span><span class="meta-pill">Свет и UV</span></div>
        <p>LITEON работает с оптоэлектроникой с 1975 года. LTR390 объединяет цифровой канал окружающего света и отдельный канал ультрафиолета с подключением по I²C.</p>
        <div class="recognition"><b>В нашей станции:</b> освещённость, сырые ALS/UVS-данные и расчёт UV-индекса после калибровки.</div>
      </article>

      <article class="chip-card">
        <h3>Espressif · ESP32-S3</h3>
        <div class="chip-meta"><span class="meta-pill">Микроконтроллер</span><span class="meta-pill">Wi-Fi и BLE</span></div>
        <p>Espressif начала работу в 2008 году. ESP32-S3 компания представила в конце 2020 года как чип для подключённых устройств и обработки данных на самом устройстве.</p>
        <div class="recognition"><b>В нашей станции:</b> читает датчики, создаёт Wi-Fi, отдаёт HTML и общается с Flipper.</div>
      </article>

      <article class="chip-card">
        <h3>Seeed Studio · XIAO ESP32-S3</h3>
        <div class="chip-meta"><span class="meta-pill">Плата XIAO</span><span class="meta-pill">Компактный формат</span></div>
        <p>Seeed Studio появилась в 2008 году и развивает компактные платы для прототипов. Она не создавала сам ESP32-S3: Seeed разместила чип Espressif на маленькой плате XIAO с USB-C, питанием и удобными контактами.</p>
        <div class="recognition"><b>В нашей станции:</b> это готовая основа, на которой работает микроконтроллер ESP32-S3.</div>
      </article>
    </div>

    <details class="source-box">
      <summary>Источники раздела</summary>
      <a href="https://sensirion.com/media/media/file/99DEEC73/6613E0D3/Annual_Report_2023_web.pdf">Sensirion Annual Report — происхождение компании</a>
      <a href="https://sensirion.com/company/news/press-releases-and-news/article/first-miniaturized-co2-sensors-are-now-available-globally">Sensirion — мировой выпуск SCD40/SCD41 в 2021 году</a>
      <a href="https://www.bosch-sensortec.com/en/news/20-years-of-sensing-the-future-bosch-sensortec-celebrates-two-decades-of-innovation.html">Bosch Sensortec — история с 2005 года</a>
      <a href="https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme680-ds001.pdf">Bosch — BME680 datasheet</a>
      <a href="https://www.liteon.com/en/staticpage/about-liteon">LITEON — история компании</a>
      <a href="https://optoelectronics.liteon.com/upload/download/DS86-2015-0004/LTR-390UV_Final_%20DS_V1%201.pdf">LITEON — LTR390 datasheet</a>
      <a href="https://www.espressif.com/en/company/about-us/ceo-letter">Espressif — история компании</a>
      <a href="https://www.espressif.com/en/node/4978">Espressif — анонс ESP32-S3</a>
      <a href="https://www.seeedstudio.com/blog/2018/08/09/10-years-of-service-history-of-seeed-fusion/">Seeed Studio — история компании</a>
    </details>
  </div>

  <div id="learn-planets" class="learn-panel">
    <div class="section-title">Приблизительные условия в Солнечной системе</div>

    <div class="planet-note">
      Значения округлены. В разных источниках цифры могут немного отличаться:
      температура зависит от места, времени суток и способа усреднения, а у газовых планет
      вообще нет твёрдой поверхности. Основные справочные значения взяты из материалов NASA;
      российский контекст и история исследований дополнены материалами Роскосмоса.
    </div>

    <div class="planet-grid">
      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-mercury"></use></svg><h4>Меркурий</h4></div>
        <div class="planet-value"><b>Средняя температура:</b> примерно +167 °C</div>
        <div class="planet-value"><b>Диапазон:</b> примерно от −179 до +427 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> почти отсутствует, есть крайне разреженная экзосфера</div>
        <div class="planet-value"><b>Давление:</b> практически нулевое</div>
      </article>

      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-venus"></use></svg><h4>Венера</h4></div>
        <div class="planet-value"><b>Средняя температура:</b> примерно +464 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> около 96,5% CO₂</div>
        <div class="planet-value"><b>Давление:</b> примерно в 90–93 раза выше земного</div>
        <div class="planet-value"><b>Особенность:</b> самая горячая планета Солнечной системы</div>
      </article>

      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-earth"></use></svg><h4>Земля</h4></div>
        <div class="planet-value"><b>Средняя температура:</b> примерно +15 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> в основном азот и кислород</div>
        <div class="planet-value"><b>Давление:</b> около 1013 гПа у уровня моря</div>
        <div class="planet-value"><b>CO₂:</b> примерно 0,04%</div>
      </article>

      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-moon"></use></svg><h4>Луна</h4></div>
        <div class="planet-value"><b>Температура:</b> примерно от −173 до +127 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> крайне разреженная экзосфера</div>
        <div class="planet-value"><b>Давление:</b> практически нулевое</div>
        <div class="planet-value"><b>Особенность:</b> воздух почти не переносит тепло между днём и ночью</div>
      </article>

      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-mars"></use></svg><h4>Марс</h4></div>
        <div class="planet-value"><b>Средняя температура:</b> примерно −65 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> около 95% CO₂</div>
        <div class="planet-value"><b>Давление:</b> примерно 6–7 гПа</div>
        <div class="planet-value"><b>Особенность:</b> атмосфера примерно в 150 раз разреженнее земной</div>
      </article>

      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-jupiter"></use></svg><h4>Юпитер</h4></div>
        <div class="planet-value"><b>Средняя температура:</b> примерно −110 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> в основном водород и гелий</div>
        <div class="planet-value"><b>Поверхность:</b> твёрдой поверхности нет</div>
        <div class="planet-value"><b>Особенность:</b> температура указана для слоя с земным давлением</div>
      </article>

      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-saturn"></use></svg><h4>Сатурн</h4></div>
        <div class="planet-value"><b>Средняя температура:</b> примерно −140 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> в основном водород и гелий</div>
        <div class="planet-value"><b>Поверхность:</b> твёрдой поверхности нет</div>
        <div class="planet-value"><b>Особенность:</b> средняя плотность ниже плотности воды</div>
      </article>

      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-uranus"></use></svg><h4>Уран</h4></div>
        <div class="planet-value"><b>Средняя температура:</b> примерно −195 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> водород, гелий и метан</div>
        <div class="planet-value"><b>Поверхность:</b> твёрдой поверхности нет</div>
        <div class="planet-value"><b>Особенность:</b> вращается почти «лёжа на боку»</div>
      </article>

      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-neptune"></use></svg><h4>Нептун</h4></div>
        <div class="planet-value"><b>Средняя температура:</b> примерно −200 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> водород, гелий и метан</div>
        <div class="planet-value"><b>Поверхность:</b> твёрдой поверхности нет</div>
        <div class="planet-value"><b>Особенность:</b> здесь наблюдаются самые быстрые ветры среди планет</div>
      </article>

      <article class="planet-card">
        <div class="planet-title"><svg class="planet-icon"><use href="#planet-pluto"></use></svg><h4>Плутон</h4></div>
        <div class="planet-value"><b>Средняя температура:</b> примерно −225 °C</div>
        <div class="planet-value"><b>Атмосфера:</b> очень тонкая, в основном азот</div>
        <div class="planet-value"><b>Давление:</b> во много тысяч раз ниже земного</div>
        <div class="planet-value"><b>Статус:</b> карликовая планета — но у нас он свой</div>
      </article>
    </div>

    <div class="planet-note">
      Наша станция рассчитана на земные условия. Эти карточки нужны для сравнения масштаба:
      на Луне, Венере, Плутоне и планетах-гигантах наши датчики работать не смогут.
    </div>

    <details class="source-box">
      <summary>Источники и почему цифры могут различаться</summary>
      <a href="https://science.nasa.gov/solar-system/temperatures-across-our-solar-system/">NASA Science — температуры в Солнечной системе</a>
      <a href="https://science.nasa.gov/resource/solar-system-temperatures/">NASA Science — средние температуры планет и Плутона</a>
      <a href="https://solarsystem.nasa.gov/planet-compare/">NASA Solar System Exploration — сравнение планет и Луны</a>
      <a href="https://www.roscosmos.ru/13727/">Роскосмос — материалы о Солнечной системе</a>
      <a href="https://www.roscosmos.ru/37996/">Роскосмос — результаты измерений станций «Венера»</a>
      <a href="https://www.roscosmos.ru/29591/">Роскосмос — условия на Луне</a>
      <p style="margin:10px 0 0;color:var(--muted);font-size:12px">
        Разница между источниками обычно появляется потому, что одни указывают среднюю температуру,
        другие — дневной и ночной диапазон, а для газовых планет выбирают определённый уровень атмосферы.
      </p>
    </details>
  </div>

</section>

<!-- Вкладка 5: Системная диагностика ESP32 и статусы сенсоров -->
<!-- Блок "Технические каналы" удален, т.к. дублировал основные метрики и ломал верстку за счет display:block на span -->
<section id="system" class="view">
  <article class="card">
    <div class="system-row"><div><strong>Режим данных</strong><span>Запрос /api/data</span></div><span id="dataMode" class="pill">—</span></div>
    <div class="system-row"><div><strong>Версия прошивки</strong><span>Передаётся ESP32</span></div><span id="firmware">—</span></div>
    <div class="system-row"><div><strong>Время работы</strong><span>После последнего запуска</span></div><span id="uptime">—</span></div>
    <div class="system-row"><div><strong>Модель чипа и CPU</strong><span>Аппаратная платформа</span></div><span id="chipInfo">—</span></div>
    <div class="system-row"><div><strong>Свободная ОЗУ (Heap)</strong><span>Оперативная память</span></div><span id="ramFree">—</span></div>
    <div class="system-row"><div><strong>MAC-адрес точки доступа</strong><span>SoftAP MAC</span></div><span id="macAddr">—</span></div>
    <div class="system-row"><div><strong>Сигнал Wi-Fi</strong><span>RSSI</span></div><span id="rssi">—</span></div>
    <div class="system-row"><div><strong>Аккумулятор</strong><span>Вторая версия</span></div><span id="battery">Не подключён</span></div>
  </article>

  <div class="section-title">Датчики</div>
  <article class="card">
    <div class="sensor-row">
      <div class="sensor-name"><strong>SCD41</strong><span>CO₂ · температура · влажность</span><span id="scd41Detail" class="sensor-detail">Ожидание данных</span></div>
      <span id="scd41State" class="pill">—</span>
    </div>
    <div class="sensor-row">
      <div class="sensor-name"><strong>BME680</strong><span>Температура · влажность · давление · газовое сопротивление</span><span id="bme680Detail" class="sensor-detail">Ожидание данных</span></div>
      <span id="bme680State" class="pill">—</span>
    </div>
    <div class="sensor-row">
      <div class="sensor-name"><strong>LTR390</strong><span>Окружающий свет ALS · ультрафиолет UVS</span><span id="ltr390Detail" class="sensor-detail">Ожидание данных</span></div>
      <span id="ltr390State" class="pill">—</span>
    </div>
  </article>

  <div class="section-title">Что датчики умеют, но это не отдельные прямые измерения</div>
  <div class="capabilities">
    <div class="capability">
      <strong>SCD41: компенсации и калибровка</strong>
      <span>Поддерживает поправку температуры, высоты и атмосферного давления, автоматическую самокалибровку и принудительную перекалибровку. Это настройки точности, а не новые измеряемые параметры.</span>
    </div>
    <div class="capability">
      <strong>BME680: IAQ, VOC и CO₂-эквивалент</strong>
      <span>Такие показатели можно вычислять алгоритмом BSEC, но сам датчик напрямую отдаёт температуру, влажность, давление и сопротивление газового элемента. Конкретный газ он не определяет.</span>
    </div>
    <div class="capability">
      <strong>LTR390: люксы и UV Index</strong>
      <span>Чип напрямую отдаёт цифровые отсчёты ALS и UVS. Люксы и UVI вычисляет прошивка с учётом усиления, разрешения и калибровки.</span>
    </div>
    <div class="capability">
      <strong>Высота над уровнем моря</strong>
      <span>Её можно приблизительно вычислить из давления, но BME680 напрямую высоту не измеряет. Результат зависит от текущего давления на уровне моря.</span>
    </div>
  </div>
</section>

<div class="footer">ENV Station · protocol v15</div>
</main>

<!-- Модальное окно настройки видимости карточек (сохраняется в localStorage браузера) -->
<div id="metricSettingsModal" class="modal" aria-hidden="true">
  <section class="modal-sheet settings-sheet" role="dialog" aria-modal="true" aria-labelledby="metricSettingsTitle">
    <div class="modal-top">
      <div>
        <h2 id="metricSettingsTitle" class="modal-title">Настроить показатели</h2>
        <div class="modal-subtitle">Главный экран и доступные графики</div>
      </div>
      <button id="metricSettingsClose" class="modal-close" aria-label="Закрыть">✕</button>
    </div>

    <p class="settings-intro">
      Выбери только нужные карточки. Настройка сохранится в браузере этого устройства.
    </p>

    <div class="preset-row" aria-label="Готовые наборы">
      <button class="preset-button" type="button" data-preset="main">Основное</button>
      <button class="preset-button" type="button" data-preset="climate">Микроклимат</button>
      <button class="preset-button" type="button" data-preset="light">Свет и UV</button>
      <button class="preset-button" type="button" data-preset="all">Все</button>
    </div>

    <div class="metric-choice-list">
      <label class="metric-choice">
        <input type="checkbox" value="co2" data-metric-toggle>
        <span class="metric-choice-copy"><strong>CO₂</strong><span>SCD41 · ppm</span></span>
      </label>
      <label class="metric-choice">
        <input type="checkbox" value="temperature" data-metric-toggle>
        <span class="metric-choice-copy"><strong>Температура</strong><span>SCD41 · °C</span></span>
      </label>
      <label class="metric-choice">
        <input type="checkbox" value="humidity" data-metric-toggle>
        <span class="metric-choice-copy"><strong>Влажность</strong><span>SCD41 · %RH</span></span>
      </label>
      <label class="metric-choice">
        <input type="checkbox" value="pressure" data-metric-toggle>
        <span class="metric-choice-copy"><strong>Давление</strong><span>BME680 · гПа и мм рт. ст.</span></span>
      </label>
      <label class="metric-choice">
        <input type="checkbox" value="gas" data-metric-toggle>
        <span class="metric-choice-copy"><strong>Газовый канал</strong><span>BME680 · изменение фона</span></span>
      </label>
      <label class="metric-choice">
        <input type="checkbox" value="lux" data-metric-toggle>
        <span class="metric-choice-copy"><strong>Освещённость</strong><span>LTR390 · люксы</span></span>
      </label>
      <label class="metric-choice">
        <input type="checkbox" value="uv" data-metric-toggle>
        <span class="metric-choice-copy"><strong>Ультрафиолет</strong><span>LTR390 · raw или UVI</span></span>
      </label>
    </div>

    <div class="settings-note">
      Скрытие карточки не выключает датчик и не останавливает сбор данных.
      Общая оценка продолжает учитывать все установленные датчики.
    </div>

    <div class="settings-actions">
      <button id="metricSettingsReset" class="settings-reset" type="button">Вернуть все</button>
      <button id="metricSettingsDone" class="settings-done" type="button">Готово</button>
    </div>
  </section>
</div>

<!-- Модальное окно подробной справки при клике на любую карточку показателя -->
<div id="infoModal" class="modal" aria-hidden="true">
  <section class="modal-sheet" role="dialog" aria-modal="true" aria-labelledby="modalTitle">
    <div class="modal-top">
      <div>
        <h2 id="modalTitle" class="modal-title">Показатель</h2>
        <div id="modalSubtitle" class="modal-subtitle"></div>
      </div>
      <button id="modalClose" class="modal-close" aria-label="Закрыть">✕</button>
    </div>
    <div id="modalMetricStatus" class="modal-status-box">
      <div class="modal-status-top">
        <div class="modal-status-label">Текущее состояние</div>
        <div id="modalCurrentValue" class="modal-current-value">—</div>
      </div>
      <div id="modalLiveStatus" class="modal-live-status">Ожидание данных</div>
      <div id="modalStatusHelp" class="modal-status-help">Статус появится после получения показаний.</div>
    </div>
    <div id="modalContent"></div>
  </section>
</div>

<script>
(()=>{
// Конфигурация опроса: запрашиваем /api/data каждые 2000 мс, храним 60 точек для графиков
const API='/api/data',POLL=2000,MAX=60,$=id=>document.getElementById(id);

// Управление отображением карточек и сохранением пресетов в localStorage
const METRIC_KEYS=['co2','temperature','humidity','pressure','gas','lux','uv'];
const METRIC_STORAGE_KEY='envStationVisibleMetricsV15';
const METRIC_PRESETS={
  main:['co2','temperature','humidity','pressure'],
  climate:['temperature','humidity','pressure'],
  light:['lux','uv'],
  all:[...METRIC_KEYS]
};
let visibleMetrics=loadVisibleMetrics();

function normalizeMetricList(value){
  if(!Array.isArray(value))return [...METRIC_KEYS];
  return [...new Set(value.filter(key=>METRIC_KEYS.includes(key)))];
}
function loadVisibleMetrics(){
  try{
    const saved=localStorage.getItem(METRIC_STORAGE_KEY);
    return saved===null?[...METRIC_KEYS]:normalizeMetricList(JSON.parse(saved));
  }catch(_){
    return [...METRIC_KEYS];
  }
}
function saveVisibleMetrics(){
  try{localStorage.setItem(METRIC_STORAGE_KEY,JSON.stringify(visibleMetrics))}catch(_){}
}
function sameMetricSet(a,b){
  return a.length===b.length&&a.every(key=>b.includes(key));
}
function currentPresetName(){
  for(const [name,list] of Object.entries(METRIC_PRESETS)){
    if(sameMetricSet(visibleMetrics,list))return name;
  }
  return 'custom';
}
function syncMetricControls(){
  document.querySelectorAll('[data-metric-toggle]').forEach(input=>{
    input.checked=visibleMetrics.includes(input.value);
  });
  const preset=currentPresetName();
  document.querySelectorAll('[data-preset]').forEach(button=>{
    button.classList.toggle('active',button.dataset.preset===preset);
  });
}
function applyMetricVisibility(){
  document.querySelectorAll('[data-metric-card]').forEach(card=>{
    card.classList.toggle('metric-hidden',!visibleMetrics.includes(card.dataset.metricCard));
  });

  document.querySelectorAll('[data-metric-chart]').forEach(card=>{
    card.classList.toggle('metric-hidden',!visibleMetrics.includes(card.dataset.metricChart));
  });

  const visibleCharts=[...document.querySelectorAll('[data-metric-chart]')]
    .filter(card=>!card.classList.contains('metric-hidden')).length;

  $('visibleMetricCount').textContent=`${visibleMetrics.length} из ${METRIC_KEYS.length}`;
  $('metricEmptyState').classList.toggle('show',visibleMetrics.length===0);
  $('chartEmptyState').classList.toggle('show',visibleCharts===0);
  syncMetricControls();
  requestAnimationFrame(drawAll);
}
function setVisibleMetrics(list){
  visibleMetrics=normalizeMetricList(list);
  saveVisibleMetrics();
  applyMetricVisibility();
}
function openMetricSettings(){
  syncMetricControls();
  metricSettingsModal.classList.add('open');
  metricSettingsModal.setAttribute('aria-hidden','false');
  document.body.style.overflow='hidden';
}
function closeMetricSettings(){
  metricSettingsModal.classList.remove('open');
  metricSettingsModal.setAttribute('aria-hidden','true');
  document.body.style.overflow='';
}

// Демо-режим включается ТОЛЬКО при открытии локального файла с диска (file://) или с параметром ?demo=1
let demo=location.protocol==='file:'||new URLSearchParams(location.search).get('demo')==='1',fails=0;
const hist={co2:[],temp:[],hum:[],pressure:[],gas:[],lux:[],uv:[]};
const sessionLog=[];
const stats={};
let previousData=null;
const sensorMeta={
  scd41:{state:'waiting',ok:null,age:0,updatedAt:0},
  bme680:{state:'waiting',ok:null,age:0,updatedAt:0},
  ltr390:{state:'waiting',ok:null,age:0,updatedAt:0}
};


let currentInfoKey=null;

const STATUS_SOURCE={
  co2:'co2Status',
  temperature:'temperatureStatus',
  humidity:'humidityStatus',
  pressure:'pressureStatus',
  gas:'gasStatus',
  lux:'luxStatus',
  uv:'uvStatus'
};

function metricCurrentValue(key){
  if(key==='co2')return `${$('co2').textContent} ppm`;
  if(key==='temperature')return `${$('temperature').textContent} °C`;
  if(key==='humidity')return `${$('humidity').textContent} %`;
  if(key==='pressure')return `${$('pressure').textContent} гПа · ${$('pressureMmhg').textContent} мм рт. ст.`;
  if(key==='gas')return `${$('gas').textContent} кОм`;
  if(key==='lux')return `${$('lux').textContent} лк`;
  if(key==='uv')return `${$('uv').textContent} ${$('uvUnit').textContent}`;
  return '—';
}

// Пояснения к текущему статусу внутри модального окна карточки
function statusHelp(key,label){
  const text=String(label||'').toLowerCase();

  if(key==='co2'){
    if(text.includes('отлично'))return 'CO₂ низкий: вентиляция работает хорошо.';
    if(text.includes('нормально'))return 'CO₂ находится в обычном диапазоне для помещения.';
    if(text.includes('проветрить'))return 'Свежего воздуха становится мало — стоит открыть окно или включить вентиляцию.';
    if(text.includes('высок'))return 'CO₂ заметно повышен. Помещение желательно проветрить.';
  }

  if(key==='temperature'){
    if(text.includes('комфорт'))return 'Температура подходит большинству людей для обычного помещения.';
    if(text.includes('прохлад'))return 'Ниже выбранного комфортного диапазона.';
    if(text.includes('тепло'))return 'Выше комфортного диапазона, но ещё не экстремально.';
    if(text.includes('жар'))return 'Температура заметно выше комфортной.';
    if(text.includes('холод'))return 'Температура заметно ниже комфортной.';
  }

  if(key==='humidity'){
    if(text.includes('комфорт'))return 'Влажность находится в удобном для помещения диапазоне.';
    if(text.includes('сух'))return 'Воздух суше комфортного диапазона.';
    if(text.includes('влаж'))return 'Влажность выше комфортного диапазона.';
  }

  if(key==='pressure'){
    if(text.includes('обыч'))return 'Давление находится около типичного диапазона у уровня моря.';
    if(text.includes('пониж'))return 'Давление ниже условного диапазона у уровня моря. На высоте это может быть нормой.';
    if(text.includes('повыш'))return 'Давление выше условного диапазона у уровня моря.';
  }

  if(key==='gas'){
    if(text.includes('прогрев')||text.includes('собираем'))return 'Датчику нужно 90 секунд прогрева пластины до 320 °C и время для оценки фона помещения.';
    if(text.includes('фонов')||text.includes('норм'))return 'Газовое сопротивление близко к привычному уровню чистого помещения.';
    if(text.includes('есть изменение')||text.includes('средн'))return 'Состав летучих веществ изменился относительно фонового уровня.';
    if(text.includes('сильное')||text.includes('загрязн'))return 'Газовый канал заметил резкое изменение, но не может определить конкретное вещество.';
  }

  if(key==='lux'){
    if(text.includes('темно'))return 'Света почти нет (ночь или выключено освещение).';
    if(text.includes('приглуш'))return 'Мягкий вечерний или фоновый свет.';
    if(text.includes('комфорт'))return 'Обычное комфортное освещение в комнате.';
    if(text.includes('ярко'))return 'Яркий дневной или направленный свет.';
  }

  if(key==='uv'){
    if(text.includes('калибров'))return 'Сейчас показаны сырые UV-данные. Для понятного UVI нужна настройка пересчёта.';
    if(text.includes('низк'))return 'Ультрафиолетовый индекс низкий.';
    if(text.includes('умерен'))return 'Уровень UV уже заметный.';
    if(text.includes('высок'))return 'Уровень UV высокий.';
  }

  return 'Статус показывает, как текущее значение соотносится с выбранным ориентировочным диапазоном.';
}

function updateModalMetricStatus(){
  if(!currentInfoKey)return;

  const sourceId=STATUS_SOURCE[currentInfoKey];
  const source=sourceId?$(sourceId):null;
  let label='Справочный канал';
  let cls='info';

  if(source){
    label=source.textContent||'Ожидание данных';
    if(source.classList.contains('ok'))cls='ok';
    else if(source.classList.contains('warn'))cls='warn';
    else if(source.classList.contains('bad'))cls='bad';
    else if(source.classList.contains('info'))cls='info';
    else cls='';
  }

  $('modalCurrentValue').textContent=metricCurrentValue(currentInfoKey);
  $('modalLiveStatus').textContent=label;
  $('modalLiveStatus').className='modal-live-status '+cls;
  $('modalStatusHelp').textContent=statusHelp(currentInfoKey,label);
}

// Справочная база для модальных окон при нажатии на карточку
const INFO={
co2:{
 title:'Углекислый газ',
 subtitle:'SCD41',
 unit:'ppm',
 unitText:'Сколько частиц CO₂ приходится на миллион частиц воздуха. 1000 ppm — примерно 0,1%.',
 why:'Показывает, хватает ли комнате свежего воздуха. Чем больше людей и хуже проветривание, тем выше CO₂.',
 ranges:[
  ['до 800','воздух обычно свежий'],
  ['800–1000','всё нормально'],
  ['1000–1400','пора проветрить'],
  ['выше 1400','воздух заметно застоялся']
 ],
 note:'SCD41 измеряет настоящий CO₂. Это не догадка по запахам или другим газам.'
},
temperature:{
 title:'Температура',
 subtitle:'Основное значение — SCD41',
 unit:'°C',
 unitText:'Привычные градусы Цельсия: чем больше число, тем теплее.',
 why:'Показывает, будет ли в комнате холодно, комфортно или жарко.',
 ranges:[
  ['18–20 °C','прохладно'],
  ['20–24 °C','обычно комфортно'],
  ['24–27 °C','тепло'],
  ['выше 27 °C','жарко']
 ],
 note:'BME680 тоже измеряет температуру, но рядом с электроникой может показывать чуть больше.'
},
humidity:{
 title:'Влажность',
 subtitle:'Относительная влажность воздуха',
 unit:'%',
 unitText:'0% — очень сухо, 100% — воздух почти не может принять ещё влагу.',
 why:'Показывает, воздух сухой или сырой. Это важно для комфорта, кожи, дыхания и риска плесени.',
 ranges:[
  ['ниже 30%','очень сухо'],
  ['30–40%','суховато'],
  ['40–60%','обычно комфортно'],
  ['выше 60%','сыровато']
 ],
 note:'При нагреве воздуха процент влажности может упасть, даже если воды в комнате не стало меньше.'
},
pressure:{
 title:'Атмосферное давление',
 subtitle:'BME680',
 unit:'гПа и мм рт. ст.',
 unitText:'гПа — гектопаскали, современная единица, которую используют метеорологи и сам датчик. мм рт. ст. — миллиметры ртутного столба, привычная единица старых барометров и прогнозов. 1013 гПа ≈ 760 мм рт. ст.',
 why:'Помогает замечать смену погоды. Падение давления часто бывает перед ухудшением погоды, а рост — перед более спокойной.',
 ranges:[
  ['1013 гПа / 760 мм','обычное давление у уровня моря'],
  ['1027 гПа / 770 мм','пример повышенного давления'],
  ['987 гПа / 740 мм','пример пониженного давления']
 ],
 note:'На высоте обычное давление ниже. Важны не только сами цифры, но и то, растут они или падают.'
},
gas:{
 title:'Газовый сенсор',
 subtitle:'BME680',
 unit:'кОм',
 unitText:'Это сопротивление чувствительного элемента внутри датчика.',
 why:'Реагирует на запахи, спирт, аэрозоли, готовку и другие летучие вещества.',
 ranges:[
  ['стабильно','фон комнаты почти не меняется'],
  ['резко упало','рядом появился новый запах или испарение'],
  ['постепенно вернулось','воздух очищается']
 ],
 note:'Он не умеет сказать: «это именно спирт» или «это именно дым». Он замечает общее изменение смеси газов.'
},
lux:{
 title:'Освещённость',
 subtitle:'LTR390',
 unit:'лк',
 unitText:'Люксы показывают, насколько ярким свет кажется человеческому глазу.',
 why:'Помогает понять, темно ли в комнате и хватает ли света для отдыха или работы.',
 ranges:[
  ['до 10 лк','темно'],
  ['10–40 лк','приглушённый вечерний свет'],
  ['40–500 лк','комфортный комнатный свет'],
  ['500+ лк','очень ярко / солнечный свет']
 ],
 note:'Положение датчика важно: если закрыть его рукой или отвернуть от люстры/окна, значение сразу изменится.'
},
uv:{
 title:'Ультрафиолет',
 subtitle:'LTR390',
 unit:'raw / UVI',
 unitText:'raw — сырые отсчёты датчика. UVI — привычный ультрафиолетовый индекс после пересчёта.',
 why:'Показывает, насколько сильное ультрафиолетовое излучение попадает на датчик.',
 ranges:[
  ['UVI 0–2','низкий'],
  ['UVI 3–5','умеренный'],
  ['UVI 6–7','высокий'],
  ['UVI 8+','очень высокий']
 ],
 note:'В помещении без прямого солнечного луча индекс UVI равен 0.'
}
};

document.querySelectorAll('.tab').forEach(b=>b.onclick=()=>{
  document.querySelectorAll('.tab').forEach(x=>x.classList.toggle('active',x===b));
  document.querySelectorAll('.view').forEach(v=>v.classList.toggle('active',v.id===b.dataset.view));
  requestAnimationFrame(drawAll)
});

document.querySelectorAll('.learn-subtab').forEach(b=>b.onclick=()=>{
  document.querySelectorAll('.learn-subtab').forEach(x=>x.classList.toggle('active',x===b));
  document.querySelectorAll('.learn-panel').forEach(p=>p.classList.toggle('active',p.id==='learn-'+b.dataset.learn));
});


const metricSettingsModal=$('metricSettingsModal');
$('openMetricSettings').onclick=openMetricSettings;
$('metricSettingsClose').onclick=closeMetricSettings;
$('metricSettingsDone').onclick=closeMetricSettings;
$('metricSettingsReset').onclick=()=>setVisibleMetrics(METRIC_PRESETS.all);
$('showAllMetricsFromEmpty').onclick=()=>setVisibleMetrics(METRIC_PRESETS.all);

document.querySelectorAll('[data-metric-toggle]').forEach(input=>{
  input.addEventListener('change',()=>{
    const selected=[...document.querySelectorAll('[data-metric-toggle]:checked')].map(x=>x.value);
    setVisibleMetrics(selected);
  });
});
document.querySelectorAll('[data-preset]').forEach(button=>{
  button.addEventListener('click',()=>setVisibleMetrics(METRIC_PRESETS[button.dataset.preset]||METRIC_PRESETS.all));
});
metricSettingsModal.addEventListener('click',event=>{
  if(event.target===metricSettingsModal)closeMetricSettings();
});

const modal=$('infoModal');
function openInfo(key){
  const x=INFO[key]; if(!x)return;
  currentInfoKey=key;
  $('modalTitle').textContent=x.title;
  $('modalSubtitle').textContent=x.subtitle;
  const ranges=x.ranges.length
    ? `<div class="info-block"><h3>Ориентиры</h3><div class="range-list">${x.ranges.map(r=>`<div class="range-item"><b>${r[0]}</b><span>${r[1]}</span></div>`).join('')}</div></div>`
    : '';
  $('modalContent').innerHTML=`
    <div class="info-block"><h3>Единица измерения</h3><div class="unit-box"><span class="unit-badge">${x.unit}</span><p>${x.unitText}</p></div></div>
    <div class="info-block"><h3>Зачем измерять</h3><p>${x.why}</p></div>
    ${ranges}
    <div class="warning">${x.note}</div>`;
  updateModalMetricStatus();
  modal.classList.add('open');
  modal.setAttribute('aria-hidden','false');
  document.body.style.overflow='hidden';
}
function closeInfo(){
  currentInfoKey=null;
  modal.classList.remove('open');
  modal.setAttribute('aria-hidden','true');
  document.body.style.overflow='';
}
document.querySelectorAll('[data-info]').forEach(el=>{
  el.addEventListener('click',()=>openInfo(el.dataset.info));
  el.addEventListener('keydown',e=>{
    if(e.key==='Enter'||e.key===' '){e.preventDefault();openInfo(el.dataset.info)}
  });
});
$('modalClose').onclick=closeInfo;
modal.addEventListener('click',e=>{if(e.target===modal)closeInfo()});
addEventListener('keydown',e=>{
  if(e.key!=='Escape')return;
  if(metricSettingsModal.classList.contains('open'))closeMetricSettings();
  else if(modal.classList.contains('open'))closeInfo();
});

const pick=(o,...k)=>{for(const x of k)if(o[x]!==undefined&&o[x]!==null)return o[x];return null};
const num=(v,d=0)=>v!==null&&v!==undefined&&Number.isFinite(Number(v))?Number(v).toFixed(d).replace('.',','):'—';

// Нормализация входящего JSON-ответа от ESP32 (/api/data)
function norm(r){
  const pa=pick(r,'pressure_pa'),ph=pick(r,'pressure_hpa','pressure');
  const uvi=pick(r,'uv_index'),uvr=pick(r,'uv_raw','uv');
  return{
    co2:pick(r,'co2_ppm','co2'),
    t:pick(r,'temperature_c','temperature'),
    h:pick(r,'humidity_pct','humidity'),
    p:pa!==null?Number(pa)/100:ph,
    g:pick(r,'gas_ohm','gas_resistance'),
    lux:pick(r,'lux'),
    uv:uvi!==null?uvi:uvr,
    uvu:uvi!==null?'UVI':'raw',
    scd:pick(r,'scd41_ok')!==false,
    bme:pick(r,'bme680_ok')!==false,
    ltr:pick(r,'ltr390_ok')!==false,
    scdState:pick(r,'scd41_state','scd_state'),
    bmeState:pick(r,'bme680_state','bme_state'),
    ltrState:pick(r,'ltr390_state','ltr_state'),
    scdAge:pick(r,'scd41_age_s','scd_age_s','sensor_age_s'),
    bmeAge:pick(r,'bme680_age_s','bme_age_s','sensor_age_s'),
    ltrAge:pick(r,'ltr390_age_s','ltr_age_s','sensor_age_s'),
    bat:pick(r,'battery_mv'),
    up:pick(r,'uptime_s'),
    rssi:pick(r,'rssi_dbm'),
    ram:pick(r,'ram_free'),
    cpu:pick(r,'cpu_mhz'),
    mac:pick(r,'mac'),
    chip:pick(r,'chip_model'),
    fw:pick(r,'firmware','fw_version')||'—'
  }
}

// Оценка качества воздуха по CO2
function co2Label(v){
  if(v===null||v===undefined||!Number.isFinite(Number(v)))return['Ожидание данных',''];
  v=Number(v);
  if(v<800)return['Отлично',''];
  if(v<1000)return['Нормально',''];
  if(v<1400)return['Стоит проветрить','warn'];
  return['Высокий CO₂','bad']
}

function setCondition(id,label,cls=''){
  const e=$(id);if(!e)return;
  e.textContent=label;
  e.className='metric-condition '+cls;
  return{label,cls};
}
function temperatureCondition(value){
  if(value===null||value===undefined||!Number.isFinite(Number(value)))return{label:'Нет данных',cls:''};
  const v=Number(value);
  if(v<10)return{label:'Очень холодно',cls:'bad'};
  if(v<18)return{label:'Прохладно',cls:'warn'};
  if(v<=26)return{label:'Комфортно',cls:'ok'};
  if(v<=30)return{label:'Тепло',cls:'warn'};
  return{label:'Жарко',cls:'bad'};
}
function humidityCondition(value){
  if(value===null||value===undefined||!Number.isFinite(Number(value)))return{label:'Нет данных',cls:''};
  const v=Number(value);
  if(v<30)return{label:'Очень сухо',cls:'bad'};
  if(v<40)return{label:'Суховато',cls:'warn'};
  if(v<=60)return{label:'Комфортно',cls:'ok'};
  if(v<=70)return{label:'Влажно',cls:'warn'};
  return{label:'Очень влажно',cls:'bad'};
}
function pressureCondition(value){
  if(value===null||value===undefined||!Number.isFinite(Number(value)))return{label:'Нет данных',cls:''};
  const v=Number(value);
  const mm=v*0.750061683;
  if(mm<740)return{label:'Пониженное',cls:'info'};
  if(mm>770)return{label:'Повышенное',cls:'info'};
  return{label:'Обычный диапазон',cls:'ok'};
}
// Вычисление скользящего базового уровня (Baseline) газового сенсора за последние 30 точек
function gasBaseline(){
  const values=sessionLog
    .map(row=>Number(row.gas_ohm))
    .filter(Number.isFinite)
    .slice(-30);
  if(values.length<6)return null;
  return values.reduce((a,b)=>a+b,0)/values.length;
}
// Оценка состояния газа: первые 90 секунд выводим обратный отсчет прогрева пластины
function gasCondition(value,up){
  if(up!==null&&Number(up)<90){
    const left=Math.max(0,90-Math.floor(Number(up)));
    return{label:`Прогрев... ${left} с`,cls:'info'};
  }
  if(value===null||value===undefined||!Number.isFinite(Number(value)))return{label:'Собираем фоновый уровень',cls:'info'};
  const v=Number(value),base=gasBaseline();
  if(!Number.isFinite(base)){
    const kOhm=v/1000;
    if(kOhm>=50)return{label:'Фоновый уровень',cls:'ok'};
    if(kOhm>=20)return{label:'Есть изменение',cls:'warn'};
    return{label:'Сильное изменение',cls:'bad'};
  }
  const ratio=v/base;
  if(ratio<0.70)return{label:'Сильное изменение',cls:'bad'};
  if(ratio<0.88)return{label:'Есть изменение',cls:'warn'};
  return{label:'Фоновый уровень',cls:'ok'};
}
// Адаптированные пороги освещенности для жилого помещения (40-500 лк = Комфортно)
function luxCondition(value){
  if(value===null||value===undefined||!Number.isFinite(Number(value)))return{label:'Нет данных',cls:''};
  const v=Number(value);
  if(v<10)return{label:'Темно',cls:'warn'};
  if(v<40)return{label:'Приглушённо',cls:'info'};
  if(v<=500)return{label:'Комфортно',cls:'ok'};
  return{label:'Ярко',cls:'info'};
}
function uvCondition(value,unit){
  if(value===null||value===undefined||!Number.isFinite(Number(value)))return{label:'Нет данных',cls:''};
  const v=Number(value);
  if(unit!=='UVI')return{label:'Нужна калибровка UVI',cls:'info'};
  if(v<3)return{label:'Низкий UV',cls:'ok'};
  if(v<6)return{label:'Умеренный UV',cls:'warn'};
  if(v<8)return{label:'Высокий UV',cls:'bad'};
  return{label:'Очень высокий UV',cls:'bad'};
}

function updateStat(key,value){
  if(value===null||value===undefined||!Number.isFinite(Number(value)))return;
  value=Number(value);
  if(!stats[key])stats[key]={min:value,max:value};
  stats[key].min=Math.min(stats[key].min,value);
  stats[key].max=Math.max(stats[key].max,value);
}
function setRange(id,key,digits,unit,transform=v=>v){
  const e=$(id),s=stats[key];if(!e||!s)return;
  const a=transform(s.min),b=transform(s.max);
  e.textContent=`За текущую сессию: ${num(a,digits)}–${num(b,digits)} ${unit}`;
}
function normalizeSensorState(value,ok){
  const s=String(value||'').toLowerCase();
  if(/warm|heat|прогрев/.test(s))return{label:'Прогрев',cls:'warm',state:'warming'};
  if(/calib|calibr|калибр/.test(s))return{label:'Калибровка',cls:'cal',state:'calibrating'};
  if(/wait|starting|boot|ожидан/.test(s))return{label:'Ожидание',cls:'',state:'waiting'};
  if(ok===false||/error|fail|offline|missing|нет/.test(s))return{label:'Ошибка',cls:'bad',state:'error'};
  return{label:'Готов',cls:'ok',state:'ready'};
}
function setSensorMeta(key,stateValue,ok,age){
  const meta=sensorMeta[key];
  const parsed=normalizeSensorState(stateValue,ok);
  meta.state=parsed.state;meta.label=parsed.label;meta.cls=parsed.cls;meta.ok=ok;
  meta.age=Number.isFinite(Number(age))?Math.max(0,Number(age)):0;
  meta.updatedAt=Date.now();
}
function renderSensor(key,pillId,detailId){
  const meta=sensorMeta[key],pill=$(pillId),detail=$(detailId);
  if(!pill||!detail)return;
  pill.textContent=meta.label||'Ожидание';
  pill.className='pill '+(meta.cls||'');
  if(!meta.updatedAt){detail.textContent='Ожидание данных';return}
  const age=Math.floor(meta.age+(Date.now()-meta.updatedAt)/1000);
  if(meta.state==='error')detail.textContent=`Нет корректных данных · ${age} с назад`;
  else if(meta.state==='warming')detail.textContent=`Датчик стабилизируется · данные ${age} с назад`;
  else if(meta.state==='calibrating')detail.textContent=`Идёт настройка точности · данные ${age} с назад`;
  else detail.textContent=`Последнее измерение: ${age} с назад`;
}
function refreshSensorDetails(){
  renderSensor('scd41','scd41State','scd41Detail');
  renderSensor('bme680','bme680State','bme680Detail');
  renderSensor('ltr390','ltr390State','ltr390Detail');
}
function overallSummary(d,conditions){
  let severity=0;
  const notes=[];

  const co2=Number(d.co2);
  if(d.co2!==null&&Number.isFinite(co2)){
    if(co2>=1400){severity=Math.max(severity,2);notes.push('CO₂ высокий — лучше проветрить')}
    else if(co2>=1000){severity=Math.max(severity,1);notes.push('CO₂ требует внимания')}
    else notes.push('CO₂ в норме');
  }

  if(conditions.temperature.cls==='bad')severity=Math.max(severity,2);
  else if(conditions.temperature.cls==='warn')severity=Math.max(severity,1);
  if(conditions.temperature.label!=='Нет данных')notes.push(`температура: ${conditions.temperature.label.toLowerCase()}`);

  if(conditions.humidity.cls==='bad')severity=Math.max(severity,2);
  else if(conditions.humidity.cls==='warn')severity=Math.max(severity,1);
  if(conditions.humidity.label!=='Нет данных')notes.push(`влажность: ${conditions.humidity.label.toLowerCase()}`);

  if(conditions.gas.cls==='bad')severity=Math.max(severity,2);
  else if(conditions.gas.cls==='warn')severity=Math.max(severity,1);
  if(conditions.gas.cls==='bad'||conditions.gas.cls==='warn')notes.push(`газовый канал: ${conditions.gas.label.toLowerCase()}`);

  if(conditions.uv.cls==='bad')severity=Math.max(severity,2);
  else if(conditions.uv.cls==='warn')severity=Math.max(severity,1);
  if(conditions.uv.cls==='bad'||conditions.uv.cls==='warn')notes.push(conditions.uv.label);

  if(!notes.length)return{
    title:'Ожидание данных',
    detail:'Станция ещё не получила показания.',
    icon:'◌',cls:''
  };
  if(severity===2)return{
    title:'Некоторые параметры требуют внимания',
    detail:notes.join(' · '),
    icon:'!',cls:'bad'
  };
  if(severity===1)return{
    title:'Есть небольшие отклонения',
    detail:notes.join(' · '),
    icon:'i',cls:'warn'
  };
  return{
    title:'Измеряемые параметры в норме',
    detail:notes.join(' · '),
    icon:'✓',cls:'ok'
  };
}
function showSummary(summary){
  const box=$('environmentSummary');
  box.className='summary-card '+summary.cls;
  $('summaryIcon').textContent=summary.icon;
  $('summaryTitle').textContent=summary.title;
  $('summaryDetail').textContent=summary.detail;
}
function appendSession(d){
  const now=new Date();
  sessionLog.push({
    timestamp:now.toISOString(),
    local_time:now.toLocaleString('ru-RU'),
    co2_ppm:d.co2,
    temperature_c:d.t,
    humidity_pct:d.h,
    pressure_hpa:d.p,
    pressure_mmhg:d.p!==null&&Number.isFinite(Number(d.p))?Number(d.p)*0.750061683:null,
    gas_ohm:d.g,
    lux:d.lux,
    uv_value:d.uv,
    uv_unit:d.uvu,
    scd41_state:sensorMeta.scd41.state,
    bme680_state:sensorMeta.bme680.state,
    ltr390_state:sensorMeta.ltr390.state
  });
  if(sessionLog.length>10000)sessionLog.shift();
  $('sessionCount').textContent=`Сохранено точек: ${sessionLog.length} · текущая сессия браузера`;
  $('exportCsv').disabled=sessionLog.length===0;
}
function csvValue(value){
  if(value===null||value===undefined)return'';
  const text=String(value).replace(/"/g,'""');
  return `"${text}"`;
}
function exportCsv(){
  if(!sessionLog.length)return;
  const columns=[
    'timestamp','local_time','co2_ppm','temperature_c','humidity_pct',
    'pressure_hpa','pressure_mmhg','gas_ohm','lux','uv_value','uv_unit',
    'scd41_state','bme680_state','ltr390_state'
  ];
  const rows=[columns.join(';')];
  sessionLog.forEach(row=>rows.push(columns.map(key=>csvValue(row[key])).join(';')));
  const blob=new Blob(['\ufeff'+rows.join('\r\n')],{type:'text/csv;charset=utf-8'});
  const url=URL.createObjectURL(blob),a=document.createElement('a');
  const stamp=new Date().toISOString().replace(/[:T]/g,'-').slice(0,19);
  a.href=url;a.download=`env-station-${stamp}.csv`;
  document.body.appendChild(a);a.click();a.remove();
  setTimeout(()=>URL.revokeObjectURL(url),1000);
}
function uptime(s){
  s=Number(s);if(!Number.isFinite(s))return'—';
  const d=Math.floor(s/86400),h=Math.floor((s%86400)/3600),m=Math.floor((s%3600)/60),sec=Math.floor(s%60);
  return d?`${d} д ${h} ч`:`${h} ч ${m} мин ${sec} с`;
}
function conn(ok,text){
  $('connectionDot').className='dot '+(ok?'ok':'bad');
  $('connectionText').textContent=text
}
function push(a,v){
  if(v===null||v===undefined||!Number.isFinite(Number(v)))return;
  a.push(Number(v));if(a.length>MAX)a.shift()
}
function update(raw,mode){
  const d=norm(raw);

  $('co2').textContent=num(d.co2);
  $('temperature').textContent=num(d.t,1);
  $('humidity').textContent=num(d.h,1);
  $('pressure').textContent=num(d.p,1);
  $('pressureMmhg').textContent=d.p==null?'—':num(Number(d.p)*0.750061683,0);
  $('gas').textContent=d.g==null?'—':num(Number(d.g)/1000,1);
  $('lux').textContent=num(d.lux);
  $('uv').textContent=num(d.uv,d.uvu==='UVI'?2:0);
  $('uvUnit').textContent=d.uvu;

  const conditions={
    temperature:temperatureCondition(d.t),
    humidity:humidityCondition(d.h),
    pressure:pressureCondition(d.p),
    gas:gasCondition(d.g,d.up),
    lux:luxCondition(d.lux),
    uv:uvCondition(d.uv,d.uvu)
  };

  setCondition('temperatureStatus',conditions.temperature.label,conditions.temperature.cls);
  setCondition('humidityStatus',conditions.humidity.label,conditions.humidity.cls);
  setCondition('pressureStatus',conditions.pressure.label,conditions.pressure.cls);
  setCondition('gasStatus',conditions.gas.label,conditions.gas.cls);
  setCondition('luxStatus',conditions.lux.label,conditions.lux.cls);
  setCondition('uvStatus',conditions.uv.label,conditions.uv.cls);

  updateStat('co2',d.co2);
  updateStat('temperature',d.t);
  updateStat('humidity',d.h);
  updateStat('pressure',d.p);
  updateStat('gas',d.g);
  updateStat('lux',d.lux);
  updateStat('uv',d.uv);

  setRange('co2Range','co2',0,'ppm');
  setRange('temperatureRange','temperature',1,'°C');
  setRange('humidityRange','humidity',1,'%');
  setRange('pressureRange','pressure',0,'мм рт. ст.',v=>v*0.750061683);
  setRange('gasRange','gas',1,'кОм',v=>v/1000);
  setRange('luxRange','lux',0,'лк');
  setRange('uvRange','uv',d.uvu==='UVI'?2:0,d.uvu);

  const [lab,cls]=co2Label(d.co2);
  $('co2Status').textContent=lab;
  $('co2Status').className='hero-status '+cls;
  $('lastUpdate').textContent='Последнее обновление: '+new Date().toLocaleTimeString('ru-RU');

  setSensorMeta('scd41',d.scdState,d.scd,d.scdAge);
  setSensorMeta('bme680',d.bmeState,d.bme,d.bmeAge);
  setSensorMeta('ltr390',d.ltrState,d.ltr,d.ltrAge);
  refreshSensorDetails();

  $('firmware').textContent=d.fw;
  $('uptime').textContent=uptime(d.up);
  if($('chipInfo'))$('chipInfo').textContent=d.chip?`${d.chip} · ${d.cpu||240} МГц`:'ESP32';
  if($('ramFree'))$('ramFree').textContent=d.ram?`${d.ram} байт`:'—';
  if($('macAddr'))$('macAddr').textContent=d.mac||'—';
  $('rssi').textContent=d.rssi==null?'Точка доступа (AP)':`${Math.round(d.rssi)} dBm`;
  $('battery').textContent=d.bat==null?'Не подключён':`${Math.round(d.bat)} мВ`;
  $('dataMode').textContent=mode;
  $('dataMode').className='pill '+(mode==='ESP32'?'ok':'');

  showSummary(overallSummary(d,conditions));
  updateModalMetricStatus();
  appendSession(d);

  push(hist.co2,d.co2);
  push(hist.temp,d.t);
  push(hist.hum,d.h);
  push(hist.pressure,d.p);
  push(hist.gas,d.g);
  push(hist.lux,d.lux);
  push(hist.uv,d.uv);

  previousData=d;
  drawAll()
}
function demoData(){
  const t=Date.now()/1000;
  return{
    protocol:2,firmware:'demo-2.0',uptime_s:Math.floor(performance.now()/1000),rssi_dbm:-48,
    co2_ppm:720+Math.sin(t/13)*95+Math.sin(t/4)*16,
    temperature_c:23.4+Math.sin(t/27)*.8,
    humidity_pct:44.2+Math.sin(t/31)*2.5,
    pressure_pa:100820+Math.sin(t/42)*170,
    gas_ohm:126000+Math.sin(t/17)*9000,
    lux:Math.max(0,360+Math.sin(t/11)*110),
    als_raw:18500+Math.sin(t/11)*4600,
    uv_raw:Math.max(0,690+Math.sin(t/9)*85),
    bme_temperature_c:24.1+Math.sin(t/24)*.7,
    bme_humidity_pct:43.6+Math.sin(t/29)*2.2,
    battery_mv:null,scd41_ok:true,bme680_ok:true,ltr390_ok:true,
    scd41_state:performance.now()<9000?'warming':'ready',
    bme680_state:performance.now()<14000?'warming':'ready',
    ltr390_state:'ready',
    scd41_age_s:0,bme680_age_s:0,ltr390_age_s:0
  }
}
// Опрос ESP32 каждые 2 секунды. При временной потере связи НЕ переключается навсегда в демо-режим,
// а ждет восстановления соединения с контроллером.
async function poll(){
  if(demo){update(demoData(),'Демо');conn(true,'Демо-режим');return}
  try{
    const c=new AbortController(),tm=setTimeout(()=>c.abort(),1600);
    const r=await fetch(API,{cache:'no-store',signal:c.signal});
    clearTimeout(tm);if(!r.ok)throw Error(r.status);
    update(await r.json(),'ESP32');fails=0;conn(true,'ESP32 подключена')
  }catch(e){
    fails++;conn(false,'Нет данных');
  }
}
// Отрисовка графиков на HTML5 Canvas с поддержкой экранов высокой четкости (Retina / devicePixelRatio)
function chart(canvas,vals,label,unit){
  const ctx=canvas.getContext('2d'),ratio=Math.max(1,devicePixelRatio||1);
  const rect=canvas.getBoundingClientRect(),w=Math.max(280,rect.width),h=190;
  canvas.width=w*ratio;canvas.height=h*ratio;ctx.setTransform(ratio,0,0,ratio,0,0);
  ctx.clearRect(0,0,w,h);
  const css=getComputedStyle(document.documentElement);
  const line=css.getPropertyValue('--line').trim(),muted=css.getPropertyValue('--muted').trim(),accent=css.getPropertyValue('--accent').trim();
  ctx.strokeStyle=line;ctx.lineWidth=1;
  for(let i=1;i<4;i++){const y=16+(h-38)*i/4;ctx.beginPath();ctx.moveTo(6,y);ctx.lineTo(w-6,y);ctx.stroke()}
  if(vals.length<2){ctx.fillStyle=muted;ctx.font='12px system-ui';ctx.fillText('Накапливаем данные…',12,28);return}
  let min=Math.min(...vals),max=Math.max(...vals);if(max===min){max++;min--}
  const pad=(max-min)*.16;min-=pad;max+=pad;
  ctx.strokeStyle=accent;ctx.lineWidth=2.5;ctx.lineJoin='round';ctx.lineCap='round';ctx.beginPath();
  vals.forEach((v,i)=>{const x=8+i*(w-16)/(MAX-1),y=12+(max-v)*(h-34)/(max-min);i?ctx.lineTo(x,y):ctx.moveTo(x,y)});
  ctx.stroke();ctx.fillStyle=muted;ctx.font='11px system-ui';
  ctx.fillText(`${label}: ${vals[vals.length-1].toFixed(label==='CO₂'?0:1)} ${unit}`,10,h-7)
}
function drawAll(){
  chart($('co2Chart'),hist.co2,'CO₂','ppm');
  chart($('tempChart'),hist.temp,'Температура','°C');
  chart($('humidityChart'),hist.hum,'Влажность','%')
}

function readCalc(id){
  const text=$(id).value.replace(',','.');
  const value=Number(text);
  return Number.isFinite(value)?value:null;
}
function setCalc(id,value,digits){
  if(value===null||!Number.isFinite(value))return;
  $(id).value=value.toFixed(digits).replace(/\.?0+$/,'');
}
function bindConverters(){
  const bind=(id,event)=>$(id)&&$(id).addEventListener('input',event);

  bind('calcHpa',()=>{
    const v=readCalc('calcHpa');
    if(v!==null)setCalc('calcMmhg',v*0.750061683,1);
  });
  bind('calcMmhg',()=>{
    const v=readCalc('calcMmhg');
    if(v!==null)setCalc('calcHpa',v/0.750061683,1);
  });

  bind('calcPpm',()=>{
    const v=readCalc('calcPpm');
    if(v!==null)setCalc('calcPercent',v/10000,4);
  });
  bind('calcPercent',()=>{
    const v=readCalc('calcPercent');
    if(v!==null)setCalc('calcPpm',v*10000,0);
  });

  bind('calcOhm',()=>{
    const v=readCalc('calcOhm');
    if(v!==null)setCalc('calcKohm',v/1000,2);
  });
  bind('calcKohm',()=>{
    const v=readCalc('calcKohm');
    if(v!==null)setCalc('calcOhm',v*1000,0);
  });

  bind('calcCelsius',()=>{
    const v=readCalc('calcCelsius');
    if(v!==null)setCalc('calcFahrenheit',v*1.8+32,1);
  });
  bind('calcFahrenheit',()=>{
    const v=readCalc('calcFahrenheit');
    if(v!==null)setCalc('calcCelsius',(v-32)/1.8,1);
  });
}
bindConverters();
applyMetricVisibility();
$('exportCsv').addEventListener('click',exportCsv);

addEventListener('resize',drawAll);
poll();
setInterval(poll,POLL);
setInterval(refreshSensorDetails,1000)
})();
</script>
</body>
</html>
)=====";