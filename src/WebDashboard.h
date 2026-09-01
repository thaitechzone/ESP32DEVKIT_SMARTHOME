#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "WeatherMonitor.h"
#include "DhtSensor.h"
#include "RelayController.h"
#include "MqttClient.h"

AsyncWebServer webDashboardServer(80);

static const char WEB_DASHBOARD_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>ESP32 Smart Home</title>
<style>
  :root { color-scheme: light dark; }
  body { font-family: -apple-system, Segoe UI, Roboto, sans-serif; margin: 0; padding: 16px; background: #f2f4f7; color: #1a1a1a; }
  h1 { font-size: 1.3rem; margin: 0 0 16px; }
  .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(260px, 1fr)); gap: 12px; }
  .card { background: #fff; border-radius: 10px; padding: 14px 16px; box-shadow: 0 1px 3px rgba(0,0,0,0.08); }
  .card h2 { font-size: 0.95rem; margin: 0 0 10px; color: #555; text-transform: uppercase; letter-spacing: .04em; }
  .row { display: flex; justify-content: space-between; padding: 3px 0; font-size: 0.92rem; }
  .row span:first-child { color: #666; }
  .relay-card h3 { margin: 0 0 8px; font-size: 1rem; }
  .badge { display: inline-block; padding: 2px 8px; border-radius: 12px; font-size: 0.75rem; font-weight: 600; }
  .badge.on { background: #d1f5d3; color: #1a7a1e; }
  .badge.off { background: #eee; color: #777; }
  .btn { border: none; border-radius: 6px; padding: 6px 12px; font-size: 0.85rem; cursor: pointer; margin: 2px 4px 2px 0; }
  .btn.on { background: #2e7d32; color: #fff; }
  .btn.off { background: #c62828; color: #fff; }
  .btn.mode { background: #e0e0e0; color: #333; }
  .btn.mode.active { background: #1565c0; color: #fff; }
  .btn.save { background: #1565c0; color: #fff; margin-top: 8px; }
  .field { margin: 6px 0; }
  .field label { display: block; font-size: 0.78rem; color: #666; margin-bottom: 2px; }
  .field input, .field select { width: 100%; box-sizing: border-box; padding: 5px 6px; border-radius: 5px; border: 1px solid #ccc; font-size: 0.85rem; }
  .mode-panel { display: none; margin-top: 8px; padding-top: 8px; border-top: 1px dashed #ddd; }
  .mode-panel.active { display: block; }
  #toast { position: fixed; bottom: 16px; left: 50%; transform: translateX(-50%); background: #333; color: #fff; padding: 8px 16px; border-radius: 6px; font-size: 0.85rem; opacity: 0; transition: opacity .25s; pointer-events: none; }
  #toast.show { opacity: 1; }
  .topics-card { grid-column: 1 / -1; }
  .topic-list { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 4px 16px; }
  .topic-row { display: flex; justify-content: space-between; gap: 8px; font-family: ui-monospace, Consolas, monospace; font-size: 0.78rem; padding: 3px 0; border-bottom: 1px solid #eee; }
  .topic-row span:first-child { color: #333; word-break: break-all; }
  .topic-row .dir { flex-shrink: 0; font-size: 0.68rem; padding: 1px 6px; border-radius: 8px; font-family: -apple-system, sans-serif; }
  .dir.pub { background: #e3f2fd; color: #1565c0; }
  .dir.sub { background: #fff3e0; color: #e65100; }
  @media (prefers-color-scheme: dark) {
    .topic-row { border-bottom-color: #2a303a; }
    .topic-row span:first-child { color: #ddd; }
    .dir.pub { background: #123049; color: #7fb8f0; }
    .dir.sub { background: #3a2a10; color: #f0b96a; }
  }
  @media (prefers-color-scheme: dark) {
    body { background: #15181d; color: #e8e8e8; }
    .card { background: #1f242b; box-shadow: none; }
    .card h2 { color: #9aa4b2; }
    .row span:first-child { color: #8b93a0; }
    .field label { color: #8b93a0; }
    .field input, .field select { background: #2a303a; border-color: #3a4149; color: #e8e8e8; }
    .btn.mode { background: #333a44; color: #ddd; }
    .badge.off { background: #333a44; color: #aaa; }
  }
</style>
</head>
<body>
<h1>ESP32 Smart Home Dashboard</h1>
<div class="grid" id="grid"></div>
<div id="toast"></div>
<script>
const grid = document.getElementById('grid');
const toastEl = document.getElementById('toast');
let toastTimer = null;

function toast(msg) {
  toastEl.textContent = msg;
  toastEl.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toastEl.classList.remove('show'), 2000);
}

function pad2(n) { return String(n).padStart(2, '0'); }

function renderStatusCards(data) {
  let html = '';

  html += `<div class="card"><h2>Environment (OpenWeather)</h2>`;
  if (data.weather.valid) {
    html += `<div class="row"><span>Temp</span><span>${data.weather.temp.toFixed(1)} C</span></div>`;
    html += `<div class="row"><span>Humidity</span><span>${data.weather.hum.toFixed(0)} %</span></div>`;
    html += `<div class="row"><span>Wind</span><span>${data.weather.wind.toFixed(1)} m/s</span></div>`;
    html += `<div class="row"><span>Sky</span><span>${data.weather.desc}</span></div>`;
    html += `<div class="row"><span>Rain</span><span>${data.weather.rain ? 'Expected' : 'None'}</span></div>`;
  } else {
    html += `<div class="row"><span>Status</span><span>N/A</span></div>`;
  }
  if (data.air.valid) {
    html += `<div class="row"><span>AQI</span><span>${data.air.aqi}/5</span></div>`;
    html += `<div class="row"><span>PM2.5</span><span>${data.air.pm25.toFixed(1)} ug/m3</span></div>`;
  }
  html += `</div>`;

  html += `<div class="card"><h2>DHT11 Sensor ${data.dht.sim ? '(SIM)' : ''}</h2>`;
  if (data.dht.valid) {
    html += `<div class="row"><span>Temp</span><span>${data.dht.temp.toFixed(1)} C</span></div>`;
    html += `<div class="row"><span>Humidity</span><span>${data.dht.hum.toFixed(0)} %</span></div>`;
  } else {
    html += `<div class="row"><span>Status</span><span>N/A</span></div>`;
  }
  html += `</div>`;

  html += `<div class="card"><h2>System</h2>`;
  html += `<div class="row"><span>WiFi</span><span>${data.wifi.connected ? 'Connected' : 'Disconnected'}</span></div>`;
  html += `<div class="row"><span>IP Address</span><span>${data.wifi.ip}</span></div>`;
  html += `<div class="row"><span>Time</span><span>${data.time}</span></div>`;
  html += `</div>`;

  data.relays.forEach((r, i) => {
    html += `<div class="card relay-card">`;
    html += `<h3>${r.name} <span class="badge ${r.state ? 'on' : 'off'}">${r.state ? 'ON' : 'OFF'}</span></h3>`;
    html += `<button class="btn on" onclick="setManual(${i}, true)">Turn ON</button>`;
    html += `<button class="btn off" onclick="setManual(${i}, false)">Turn OFF</button>`;

    html += `<div style="margin-top:10px;">`;
    ['manual', 'threshold', 'schedule'].forEach((m, mi) => {
      const active = r.mode === mi ? 'active' : '';
      html += `<button class="btn mode ${active}" onclick="showModePanel(${i}, ${mi})">${m}</button>`;
    });
    html += `</div>`;

    html += `<div class="mode-panel ${r.mode === 1 ? 'active' : ''}" id="panel-${i}-1">`;
    html += `<div class="field"><label>Metric</label><select id="metric-${i}">`;
    html += `<option value="0" ${r.thresholdMetric===0?'selected':''}>Temperature</option>`;
    html += `<option value="1" ${r.thresholdMetric===1?'selected':''}>Humidity</option>`;
    html += `</select></div>`;
    html += `<div class="field"><label>Turn ON at (>=)</label><input type="number" step="0.1" id="turnOn-${i}" value="${r.thresholdTurnOnAt}"></div>`;
    html += `<div class="field"><label>Turn OFF at (<=)</label><input type="number" step="0.1" id="turnOff-${i}" value="${r.thresholdTurnOffAt}"></div>`;
    html += `<button class="btn save" onclick="saveThreshold(${i})">Save Threshold</button>`;
    html += `</div>`;

    html += `<div class="mode-panel ${r.mode === 2 ? 'active' : ''}" id="panel-${i}-2">`;
    html += `<div class="field"><label>Turn ON time</label><input type="time" id="onTime-${i}" value="${pad2(r.scheduleOnHour)}:${pad2(r.scheduleOnMinute)}"></div>`;
    html += `<div class="field"><label>Turn OFF time</label><input type="time" id="offTime-${i}" value="${pad2(r.scheduleOffHour)}:${pad2(r.scheduleOffMinute)}"></div>`;
    html += `<button class="btn save" onclick="saveSchedule(${i})">Save Schedule</button>`;
    html += `</div>`;

    html += `</div>`;
  });

  html += `<div class="card topics-card"><h2>MQTT Topics (${data.mqtt.broker})</h2>`;
  html += `<div class="row"><span>Broker Status</span><span>${data.mqtt.connected ? 'Connected' : 'Disconnected'}</span></div>`;
  html += `<div class="topic-list" style="margin-top:8px;">`;
  data.mqtt.topics.forEach(t => {
    html += `<div class="topic-row"><span>${t.topic}</span><span class="dir ${t.dir}">${t.dir === 'pub' ? 'PUBLISH' : 'SUBSCRIBE'}</span></div>`;
  });
  html += `</div></div>`;

  grid.innerHTML = html;
}

function showModePanel(relayIndex, modeIndex) {
  document.querySelectorAll(`[id^="panel-${relayIndex}-"]`).forEach(el => el.classList.remove('active'));
  const panel = document.getElementById(`panel-${relayIndex}-${modeIndex}`);
  if (panel) panel.classList.add('active');
  if (modeIndex === 0) {
    setManualModeOnly(relayIndex);
  }
}

async function api(path, body) {
  const res = await fetch(path, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body)
  });
  if (!res.ok) throw new Error('Request failed');
  return res.json();
}

async function setManual(relayIndex, state) {
  try {
    await api('/api/relay', { relay: relayIndex, mode: 0, state });
    toast(`Relay${relayIndex + 1} set to ${state ? 'ON' : 'OFF'}`);
    refresh();
  } catch (e) { toast('Error: ' + e.message); }
}

async function setManualModeOnly(relayIndex) {
  try {
    await api('/api/relay', { relay: relayIndex, mode: 0 });
    refresh();
  } catch (e) { toast('Error: ' + e.message); }
}

async function saveThreshold(relayIndex) {
  const metric = parseInt(document.getElementById(`metric-${relayIndex}`).value, 10);
  const turnOnAt = parseFloat(document.getElementById(`turnOn-${relayIndex}`).value);
  const turnOffAt = parseFloat(document.getElementById(`turnOff-${relayIndex}`).value);
  try {
    await api('/api/relay', { relay: relayIndex, mode: 1, thresholdMetric: metric, turnOnAt, turnOffAt });
    toast(`Relay${relayIndex + 1} threshold saved`);
    refresh();
  } catch (e) { toast('Error: ' + e.message); }
}

async function saveSchedule(relayIndex) {
  const onTime = document.getElementById(`onTime-${relayIndex}`).value.split(':');
  const offTime = document.getElementById(`offTime-${relayIndex}`).value.split(':');
  try {
    await api('/api/relay', {
      relay: relayIndex, mode: 2,
      onHour: parseInt(onTime[0], 10), onMinute: parseInt(onTime[1], 10),
      offHour: parseInt(offTime[0], 10), offMinute: parseInt(offTime[1], 10)
    });
    toast(`Relay${relayIndex + 1} schedule saved`);
    refresh();
  } catch (e) { toast('Error: ' + e.message); }
}

async function refresh() {
  try {
    const res = await fetch('/api/status');
    const data = await res.json();
    renderStatusCards(data);
  } catch (e) {
    console.error(e);
  }
}

refresh();
setInterval(refresh, 5000);
</script>
</body>
</html>
)HTML";

inline String webDashboard_currentTimeString() {
  struct tm timeInfo;
  if (!getLocalTime(&timeInfo, 100)) return "N/A";
  char buf[20];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
           timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
           timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
  return String(buf);
}

inline void webDashboard_handleStatus(AsyncWebServerRequest *request) {
  JsonDocument doc;

  doc["weather"]["valid"] = latestWeatherData.weatherValid;
  doc["weather"]["temp"] = latestWeatherData.temperature;
  doc["weather"]["hum"] = latestWeatherData.humidity;
  doc["weather"]["wind"] = latestWeatherData.windSpeed;
  doc["weather"]["desc"] = latestWeatherData.weatherDesc;
  doc["weather"]["rain"] = latestWeatherData.rainExpected;

  doc["air"]["valid"] = latestWeatherData.airQualityValid;
  doc["air"]["aqi"] = latestWeatherData.aqi;
  doc["air"]["pm25"] = latestWeatherData.pm25;

  doc["dht"]["valid"] = latestDhtData.valid;
  doc["dht"]["sim"] = latestDhtData.isSimulated;
  doc["dht"]["temp"] = latestDhtData.temperature;
  doc["dht"]["hum"] = latestDhtData.humidity;

  doc["wifi"]["connected"] = (WiFi.status() == WL_CONNECTED);
  doc["wifi"]["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "-";

  doc["time"] = webDashboard_currentTimeString();

  JsonArray relaysArr = doc["relays"].to<JsonArray>();
  for (uint8_t i = 0; i < 3; i++) {
    JsonObject ro = relaysArr.add<JsonObject>();
    ro["name"] = relayConfigs[i].name;
    ro["state"] = relayConfigs[i].state;
    ro["mode"] = (int)relayConfigs[i].mode;
    ro["thresholdMetric"] = (int)relayConfigs[i].thresholdMetric;
    ro["thresholdTurnOnAt"] = relayConfigs[i].thresholdTurnOnAt;
    ro["thresholdTurnOffAt"] = relayConfigs[i].thresholdTurnOffAt;
    ro["scheduleOnHour"] = relayConfigs[i].scheduleOnHour;
    ro["scheduleOnMinute"] = relayConfigs[i].scheduleOnMinute;
    ro["scheduleOffHour"] = relayConfigs[i].scheduleOffHour;
    ro["scheduleOffMinute"] = relayConfigs[i].scheduleOffMinute;
  }

  doc["mqtt"]["connected"] = mqttClient.connected();
  doc["mqtt"]["broker"] = MQTT_BROKER;

  JsonArray topicsArr = doc["mqtt"]["topics"].to<JsonArray>();
  auto addTopic = [&](const String &topic, const char *dir) {
    JsonObject to = topicsArr.add<JsonObject>();
    to["topic"] = topic;
    to["dir"] = dir;
  };
  addTopic(mqttTopicWeatherTemp, "pub");
  addTopic(mqttTopicWeatherHum, "pub");
  addTopic(mqttTopicWeatherWind, "pub");
  addTopic(mqttTopicWeatherDesc, "pub");
  addTopic(mqttTopicWeatherRain, "pub");
  addTopic(mqttTopicWeatherAqi, "pub");
  addTopic(mqttTopicWeatherPm25, "pub");
  addTopic(mqttTopicDhtTemp, "pub");
  addTopic(mqttTopicDhtHum, "pub");
  addTopic(mqttTopicDhtSim, "pub");
  addTopic(mqttTopicSystemIp, "pub");
  addTopic(mqttTopicSystemWifi, "pub");
  for (uint8_t i = 0; i < 3; i++) {
    JsonObject stateTo = topicsArr.add<JsonObject>();
    stateTo["topic"] = mqttRelayStateTopics[i];
    stateTo["dir"] = "pub";

    JsonObject setTo = topicsArr.add<JsonObject>();
    setTo["topic"] = mqttRelaySetTopics[i];
    setTo["dir"] = "sub";
  }

  String output;
  serializeJson(doc, output);
  request->send(200, "application/json", output);
}

inline void webDashboard_handleRelayPost(AsyncWebServerRequest *request, uint8_t *data, size_t len) {
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, data, len);
  if (err) {
    request->send(400, "application/json", "{\"error\":\"invalid json\"}");
    return;
  }

  int relayIndex = doc["relay"] | -1;
  if (relayIndex < 0 || relayIndex >= 3) {
    request->send(400, "application/json", "{\"error\":\"invalid relay index\"}");
    return;
  }

  RelayConfig &r = relayConfigs[relayIndex];
  int mode = doc["mode"] | (int)r.mode;
  r.mode = (RelayMode)mode;

  if (r.mode == RELAY_MODE_MANUAL) {
    if (doc["state"].is<bool>()) {
      relayController_apply(r, doc["state"].as<bool>());
    }
  } else if (r.mode == RELAY_MODE_THRESHOLD) {
    if (doc["thresholdMetric"].is<int>()) r.thresholdMetric = (ThresholdMetric)doc["thresholdMetric"].as<int>();
    if (doc["turnOnAt"].is<float>()) r.thresholdTurnOnAt = doc["turnOnAt"].as<float>();
    if (doc["turnOffAt"].is<float>()) r.thresholdTurnOffAt = doc["turnOffAt"].as<float>();
  } else if (r.mode == RELAY_MODE_SCHEDULE) {
    if (doc["onHour"].is<int>()) r.scheduleOnHour = doc["onHour"].as<int>();
    if (doc["onMinute"].is<int>()) r.scheduleOnMinute = doc["onMinute"].as<int>();
    if (doc["offHour"].is<int>()) r.scheduleOffHour = doc["offHour"].as<int>();
    if (doc["offMinute"].is<int>()) r.scheduleOffMinute = doc["offMinute"].as<int>();
  }

  request->send(200, "application/json", "{\"ok\":true}");
}

inline void webDashboard_setup() {
  webDashboardServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", WEB_DASHBOARD_HTML);
  });

  webDashboardServer.on("/api/status", HTTP_GET, webDashboard_handleStatus);

  webDashboardServer.on("/api/relay", HTTP_POST,
    [](AsyncWebServerRequest *request) {},
    nullptr,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      webDashboard_handleRelayPost(request, data, len);
    });

  webDashboardServer.begin();
  Serial.println("[WebDashboard] HTTP server started on port 80");
}
