#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <HTTPClient.h>

// Sensor Pins
#define DHTPIN 21
#define DHTTYPE DHT11
#define PIR_PIN 22
#define TRIG_PIN 18
#define ECHO_PIN 19
#define MQ135_PIN 34
#define MQ6_PIN 35

// Fan Motor Control (L298N)
#define FAN_IN1 25
#define FAN_IN2 26
#define FAN_PWM 27

// LED Alert
#define ALERT_LED 23

// WiFi Credentials - YAHAN APNA WIFI DALNA
const char* ssid = "Aditya's Galaxy F23 5G";
const char* password = "12345678";

String botToken = "7656805521:AAGoCOI-UV5EZj_lLddfS-StShstalHbQTw";
String chatID = "6837514414";

// ThingSpeak API Key and URL
String tsUrl = "http://api.thingspeak.com/update?api_key=WI6QOAHG8S7AMYP4";

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Web server on port 80
WebServer server(80);

// Thresholds (default values)
float tempThreshold = 30.0;
float humidityThreshold = 70.0;
int gasHighThreshold = 500;
int gasMediumThreshold = 400;
int distanceThreshold = 100;

// Manual override values
bool manualOverride = false;
float manualTemp = 0;
float manualHum = 0;
int manualMQ135 = 0;
int manualMQ6 = 0;
int manualFanSpeed = 0;

unsigned long lastMotionTime = 0;
const unsigned long timeout = 2000;
bool airWasBadWithPerson = false;
int fanSpeed = 0;

// PWM Settings
#define FAN_PWM_CHANNEL 0
#define PWM_FREQ 1000
#define PWM_RESOLUTION 8

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(FAN_IN1, OUTPUT);
  pinMode(FAN_IN2, OUTPUT);
  pinMode(FAN_PWM, OUTPUT);

  pinMode(ALERT_LED, OUTPUT);

  ledcSetup(FAN_PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(FAN_PWM, FAN_PWM_CHANNEL);
  stopFan();

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }
  Serial.println();
  Serial.print("Connected. IP = ");
  Serial.println(WiFi.localIP());

  // Web routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/set", handleSet);
  server.on("/reset", handleReset);
  server.on("/dashboard", handleDashboard);
  
  server.begin();
  Serial.println("HTTP server started");
  Serial.println("Dashboard URL: http://" + WiFi.localIP().toString() + "/dashboard");
}

void loop() {
  server.handleClient();

  bool alreadySent = false;
  bool person = detectPerson();
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  int mq135 = analogRead(MQ135_PIN);
  int mq6 = analogRead(MQ6_PIN);

  // Manual override branch
  if (manualOverride) {
    temp = manualTemp;
    hum = manualHum;
    mq135 = manualMQ135;
    mq6 = manualMQ6;
    fanSpeed = manualFanSpeed;
    controlFanManual();

    sendDataToThingSpeak(temp, hum, mq6, mq135, person, fanSpeed);
    return;
  }

  // Automatic logic
  bool gasLeak = (mq135 > gasHighThreshold || mq6 > gasHighThreshold);
  bool airBad = (temp > tempThreshold || hum > humidityThreshold ||
                 mq135 > gasMediumThreshold || mq6 > gasMediumThreshold);
  bool airNormal = (!gasLeak && !airBad);

  if (gasLeak && !alreadySent) {
    sendTelegramAlert("⚠ ALERT: Dangerous Gas Leak detected!");
    alreadySent = true;
  } else if (!gasLeak) {
    alreadySent = false;
  }

  if (gasLeak) {
    runFanHigh();
    digitalWrite(ALERT_LED, HIGH);
    airWasBadWithPerson = true;
    Serial.println("⚠ GAS LEAK DETECTED! Fan HIGH");
  } else {
    digitalWrite(ALERT_LED, LOW);
    if (person && airBad) {
      airWasBadWithPerson = true;
      runFanMedium();
      Serial.println("🟡 Person + bad air. Fan MEDIUM");
    } else if (airWasBadWithPerson && !airNormal) {
      runFanMedium();
      Serial.println("🟠 Holding fan due to earlier bad air. Fan MEDIUM");
    } else {
      airWasBadWithPerson = false;
      stopFan();
      Serial.println("🟢 Normal air. Fan OFF");
    }
  }

  Serial.print("Temp: "); Serial.print(temp); Serial.print(" °C, ");
  Serial.print("Hum: "); Serial.print(hum); Serial.print(" %, ");
  Serial.print("MQ135: "); Serial.print(mq135); Serial.print(", ");
  Serial.print("MQ6: "); Serial.print(mq6); Serial.print(", ");
  Serial.print("Fan: "); Serial.println(fanSpeed);

  sendDataToThingSpeak(temp, hum, mq6, mq135, person, fanSpeed);
  delay(1000);
}

// ========== Web Handlers ==========

void handleRoot() {
  String page = "<!DOCTYPE html><html><head><title>ESP32 Sensors</title></head><body>";
  page += "<h1>🌬️ Smart Ventilation System - ESP32</h1>";
  page += "<p><strong>Device IP:</strong> " + WiFi.localIP().toString() + "</p>";
  page += "<h3>Available Endpoints:</h3>";
  page += "<ul>";
  page += "<li><a href='/data'>/data</a> - JSON API</li>";
  page += "<li><a href='/dashboard'>/dashboard</a> - Web Dashboard</li>";
  page += "<li><a href='/set?fan=100'>/set?fan=100</a> - Manual control</li>";
  page += "<li><a href='/reset'>/reset</a> - Auto mode</li>";
  page += "</ul>";
  page += "</body></html>";
  server.send(200, "text/html", page);
}

void handleData() {
  // Current sensor values read karo
  float currentTemp = manualOverride ? manualTemp : dht.readTemperature();
  float currentHum = manualOverride ? manualHum : dht.readHumidity();
  int currentMQ135 = manualOverride ? manualMQ135 : analogRead(MQ135_PIN);
  int currentMQ6 = manualOverride ? manualMQ6 : analogRead(MQ6_PIN);
  bool currentMotion = detectPerson();

  // JSON response banayo
  String json = "{";
  json += "\"temperature\":" + String(currentTemp) + ",";
  json += "\"humidity\":" + String(currentHum) + ",";
  json += "\"mq135\":" + String(currentMQ135) + ",";
  json += "\"mq6\":" + String(currentMQ6) + ",";
  json += "\"motion\":" + String(currentMotion ? "true" : "false") + ",";
  json += "\"fanSpeed\":" + String(fanSpeed) + ",";
  json += "\"manualOverride\":" + String(manualOverride ? "true" : "false") + ",";
  json += "\"deviceIP\":\"" + WiFi.localIP().toString() + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleSet() {
  if (server.hasArg("temp")) manualTemp = server.arg("temp").toFloat();
  if (server.hasArg("hum")) manualHum = server.arg("hum").toFloat();
  if (server.hasArg("mq135")) manualMQ135 = server.arg("mq135").toInt();
  if (server.hasArg("mq6")) manualMQ6 = server.arg("mq6").toInt();
  if (server.hasArg("fan")) manualFanSpeed = server.arg("fan").toInt();
  manualOverride = true;
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Manual mode activated - Fan: " + String(manualFanSpeed));
}

void handleReset() {
  manualOverride = false;
  manualTemp = manualHum = manualMQ135 = manualMQ6 = manualFanSpeed = 0;
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "text/plain", "Auto mode activated");
}

void handleDashboard() {
  String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Smart Ventilation Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); 
            min-height: 100vh; padding: 20px; 
        }
        .dashboard { max-width: 1200px; margin: 0 auto; }
        header { 
            display: flex; justify-content: space-between; align-items: center; 
            background: rgba(255, 255, 255, 0.1); backdrop-filter: blur(10px); 
            padding: 20px; border-radius: 15px; margin-bottom: 20px; color: white; 
        }
        .status-indicator { display: flex; align-items: center; gap: 10px; }
        .dot { width: 12px; height: 12px; border-radius: 50%; background: #ff4757; }
        .dot.connected { background: #2ed573; }
        .controls { 
            background: rgba(255, 255, 255, 0.1); backdrop-filter: blur(10px); 
            padding: 20px; border-radius: 15px; margin-bottom: 20px; 
            display: flex; gap: 20px; flex-wrap: wrap; align-items: center; 
        }
        button { 
            background: #ff6b6b; color: white; border: none; padding: 10px 20px; 
            border-radius: 8px; cursor: pointer; transition: background 0.3s; 
        }
        button:hover { background: #ff4757; }
        .grid-container { 
            display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); 
            gap: 20px; margin-bottom: 20px; 
        }
        .card { 
            background: rgba(255, 255, 255, 0.1); backdrop-filter: blur(10px); 
            border-radius: 15px; padding: 20px; color: white; transition: transform 0.3s; 
        }
        .card:hover { transform: translateY(-5px); }
        .card-header { 
            display: flex; justify-content: space-between; align-items: center; 
            margin-bottom: 15px; 
        }
        .card-value { font-size: 2.5em; font-weight: bold; margin-bottom: 10px; }
        .unit { font-size: 0.6em; opacity: 0.8; }
        .card-status { 
            font-size: 0.9em; padding: 5px 10px; border-radius: 20px; 
            background: rgba(255, 255, 255, 0.2); display: inline-block; 
        }
        .progress-bar { 
            width: 100%; height: 8px; background: rgba(255, 255, 255, 0.2); 
            border-radius: 4px; margin: 10px 0; overflow: hidden; 
        }
        .progress-fill { 
            height: 100%; background: #ff6b6b; border-radius: 4px; 
            transition: width 0.3s; width: 0%; 
        }
        .manual-control-panel { 
            background: rgba(255, 255, 255, 0.1); backdrop-filter: blur(10px); 
            border-radius: 15px; padding: 20px; margin-bottom: 20px; color: white; 
        }
        .control-group { display: flex; gap: 15px; align-items: center; flex-wrap: wrap; }
        .control-group input[type="range"] { width: 200px; }
        .info { 
            background: rgba(255, 255, 255, 0.1); backdrop-filter: blur(10px); 
            border-radius: 15px; padding: 20px; color: white; 
        }
        .temperature { border-left: 4px solid #ff6b6b; }
        .humidity { border-left: 4px solid #74b9ff; }
        .mq135 { border-left: 4px solid #55efc4; }
        .mq6 { border-left: 4px solid #ffeaa7; }
        .motion { border-left: 4px solid #a29bfe; }
        .fan { border-left: 4px solid #fd79a8; }
        @media (max-width: 768px) {
            .grid-container { grid-template-columns: 1fr; }
            .controls { flex-direction: column; }
            .control-group { flex-direction: column; align-items: stretch; }
        }
    </style>
</head>
<body>
    <div class="dashboard">
        <header>
            <h1>🌬️ Smart Ventilation System</h1>
            <div class="status-indicator">
                <span id="connection-status">Connecting...</span>
                <div id="status-dot" class="dot"></div>
            </div>
        </header>

        <div class="controls">
            <button id="refresh-btn">🔄 Refresh Data</button>
            <button id="auto-mode">🔄 Auto Mode</button>
            <div class="ip-input">
                <label>ESP32 IP: </label>
                <input type="text" id="esp32-ip" placeholder="192.168.1.100" value=")=====";
                
  html += WiFi.localIP().toString();
  html += R"=====(">
                <button id="connect-btn">🔗 Connect</button>
            </div>
        </div>

        <div class="grid-container">
            <div class="card temperature">
                <div class="card-header"><h3>🌡️ Temperature</h3><span class="threshold">Threshold: 30°C</span></div>
                <div class="card-value"><span id="temp-value">--</span><span class="unit">°C</span></div>
                <div class="card-status" id="temp-status">--</div>
            </div>
            <div class="card humidity">
                <div class="card-header"><h3>💧 Humidity</h3><span class="threshold">Threshold: 70%</span></div>
                <div class="card-value"><span id="hum-value">--</span><span class="unit">%</span></div>
                <div class="card-status" id="hum-status">--</div>
            </div>
            <div class="card air-quality mq135">
                <div class="card-header"><h3>🌫️ MQ135 (Air Quality)</h3><span class="threshold">High: 500</span></div>
                <div class="card-value"><span id="mq135-value">--</span></div>
                <div class="card-status" id="mq135-status">--</div>
            </div>
            <div class="card air-quality mq6">
                <div class="card-header"><h3>🔥 MQ6 (Gas)</h3><span class="threshold">High: 500</span></div>
                <div class="card-value"><span id="mq6-value">--</span></div>
                <div class="card-status" id="mq6-status">--</div>
            </div>
            <div class="card motion">
                <div class="card-header"><h3>👤 Motion Detection</h3></div>
                <div class="card-value"><span id="motion-value">--</span></div>
                <div class="card-status" id="motion-status">No motion</div>
            </div>
            <div class="card fan">
                <div class="card-header"><h3>🌀 Fan Status</h3></div>
                <div class="card-value"><span id="fan-speed-value">--</span><span class="unit">/255</span></div>
                <div class="progress-bar"><div class="progress-fill" id="fan-progress"></div></div>
                <div class="card-status" id="fan-status">Auto Mode</div>
            </div>
        </div>

        <div class="manual-control-panel">
            <h3>🎛️ Manual Control</h3>
            <div class="control-group">
                <label>Fan Speed: <span id="fan-display">0</span>/255</label>
                <input type="range" id="fan-slider" min="0" max="255" value="0">
                <button id="set-fan">Set Fan Speed</button>
                <button id="reset-mode">Reset to Auto</button>
            </div>
        </div>

        <div class="info">
            <h3>ℹ️ System Information</h3>
            <div id="system-info">Waiting for data...</div>
        </div>
    </div>

    <script>
        class SmartVentilationDashboard {
            constructor() {
                this.esp32IP = '';
                this.data = {};
                this.updateInterval = null;
                this.init();
            }

            init() {
                this.setupEventListeners();
                this.connectToESP32();
            }

            setupEventListeners() {
                document.getElementById('connect-btn').addEventListener('click', () => this.connectToESP32());
                document.getElementById('refresh-btn').addEventListener('click', () => this.fetchData());
                document.getElementById('auto-mode').addEventListener('click', () => this.setAutoMode());
                document.getElementById('set-fan').addEventListener('click', () => this.setManualFan());
                document.getElementById('reset-mode').addEventListener('click', () => this.resetToAuto());
                
                document.getElementById('esp32-ip').addEventListener('keypress', (e) => {
                    if (e.key === 'Enter') this.connectToESP32();
                });
            }

            async connectToESP32() {
                const ipInput = document.getElementById('esp32-ip').value.trim();
                if (!ipInput) {
                    alert('Please enter ESP32 IP address');
                    return;
                }

                this.esp32IP = ipInput;
                this.updateConnectionStatus('connecting', 'Testing connection...');
                
                try {
                    const response = await fetch(`http://${this.esp32IP}/data`);
                    if (response.ok) {
                        this.updateConnectionStatus('connected', `Connected to ${this.esp32IP}`);
                        this.startAutoUpdate();
                        this.fetchData();
                    } else {
                        throw new Error('Connection failed');
                    }
                } catch (error) {
                    this.updateConnectionStatus('error', `Connection failed: ${error.message}`);
                }
            }

            updateConnectionStatus(status, message) {
                const statusDot = document.getElementById('status-dot');
                const statusText = document.getElementById('connection-status');
                
                statusDot.className = 'dot';
                statusText.textContent = message;

                switch (status) {
                    case 'connected': statusDot.classList.add('connected'); break;
                    case 'error': statusDot.style.background = '#ff4757'; break;
                    case 'connecting': statusDot.style.background = '#ffa502'; break;
                }
            }

            startAutoUpdate() {
                if (this.updateInterval) clearInterval(this.updateInterval);
                this.updateInterval = setInterval(() => this.fetchData(), 2000);
            }

            async fetchData() {
                if (!this.esp32IP) return;

                try {
                    const response = await fetch(`http://${this.esp32IP}/data`);
                    if (!response.ok) throw new Error(`HTTP error! status: ${response.status}`);
                    
                    this.data = await response.json();
                    this.data.timestamp = new Date().toLocaleTimeString();
                    this.updateDashboard();
                    this.updateConnectionStatus('connected', `Connected to ${this.esp32IP}`);
                    
                } catch (error) {
                    this.updateConnectionStatus('error', `Connection lost: ${error.message}`);
                }
            }

            updateDashboard() {
                // Temperature
                if (this.data.temperature !== null && this.data.temperature !== undefined) {
                    document.getElementById('temp-value').textContent = this.data.temperature.toFixed(1);
                    const status = this.data.temperature > 30 ? 'High' : 'Normal';
                    document.getElementById('temp-status').textContent = status;
                    document.getElementById('temp-status').style.background = 
                        this.data.temperature > 30 ? 'rgba(255, 107, 107, 0.5)' : 'rgba(46, 213, 115, 0.5)';
                }

                // Humidity
                if (this.data.humidity !== null && this.data.humidity !== undefined) {
                    document.getElementById('hum-value').textContent = this.data.humidity.toFixed(1);
                    const status = this.data.humidity > 70 ? 'High' : 'Normal';
                    document.getElementById('hum-status').textContent = status;
                    document.getElementById('hum-status').style.background = 
                        this.data.humidity > 70 ? 'rgba(255, 107, 107, 0.5)' : 'rgba(46, 213, 115, 0.5)';
                }

                // MQ135
                if (this.data.mq135 !== null && this.data.mq135 !== undefined) {
                    document.getElementById('mq135-value').textContent = this.data.mq135;
                    let status = 'Normal', color = 'rgba(46, 213, 115, 0.5)';
                    if (this.data.mq135 > 500) { status = 'Danger'; color = 'rgba(255, 107, 107, 0.5)'; }
                    else if (this.data.mq135 > 400) { status = 'Warning'; color = 'rgba(255, 165, 2, 0.5)'; }
                    document.getElementById('mq135-status').textContent = status;
                    document.getElementById('mq135-status').style.background = color;
                }

                // MQ6
                if (this.data.mq6 !== null && this.data.mq6 !== undefined) {
                    document.getElementById('mq6-value').textContent = this.data.mq6;
                    let status = 'Normal', color = 'rgba(46, 213, 115, 0.5)';
                    if (this.data.mq6 > 500) { status = 'Danger'; color = 'rgba(255, 107, 107, 0.5)'; }
                    else if (this.data.mq6 > 400) { status = 'Warning'; color = 'rgba(255, 165, 2, 0.5)'; }
                    document.getElementById('mq6-status').textContent = status;
                    document.getElementById('mq6-status').style.background = color;
                }

                // Motion
                if (this.data.motion !== null && this.data.motion !== undefined) {
                    const motionText = this.data.motion ? 'Motion Detected' : 'No Motion';
                    const motionColor = this.data.motion ? 'rgba(46, 213, 115, 0.5)' : 'rgba(255, 107, 107, 0.5)';
                    document.getElementById('motion-value').textContent = this.data.motion ? 'YES' : 'NO';
                    document.getElementById('motion-status').textContent = motionText;
                    document.getElementById('motion-status').style.background = motionColor;
                }

                // Fan
                if (this.data.fanSpeed !== null && this.data.fanSpeed !== undefined) {
                    const fanSpeed = this.data.fanSpeed;
                    document.getElementById('fan-speed-value').textContent = fanSpeed;
                    document.getElementById('fan-display').textContent = fanSpeed;
                    document.getElementById('fan-slider').value = fanSpeed;
                    document.getElementById('fan-progress').style.width = `${(fanSpeed / 255) * 100}%`;
                    
                    let fanStatus = 'Auto Mode';
                    if (this.data.manualOverride) fanStatus = 'Manual Mode';
                    else if (fanSpeed === 255) fanStatus = 'High Speed';
                    else if (fanSpeed === 150) fanStatus = 'Medium Speed';
                    else if (fanSpeed === 0) fanStatus = 'Stopped';
                    
                    document.getElementById('fan-status').textContent = fanStatus;
                }

                // System info
                const infoDiv = document.getElementById('system-info');
                infoDiv.innerHTML = `
                    <p><strong>Last Update:</strong> ${this.data.timestamp}</p>
                    <p><strong>Device IP:</strong> ${this.data.deviceIP || 'Unknown'}</p>
                    <p><strong>Mode:</strong> ${this.data.manualOverride ? 'Manual' : 'Auto'}</p>
                `;
            }

            async setManualFan() {
                const fanSpeed = document.getElementById('fan-slider').value;
                if (!this.esp32IP) { alert('Not connected to ESP32'); return; }

                try {
                    const response = await fetch(`http://${this.esp32IP}/set?fan=${fanSpeed}`);
                    if (response.ok) this.fetchData();
                    else throw new Error('Failed to set fan speed');
                } catch (error) {
                    alert('Error setting fan speed: ' + error.message);
                }
            }

            async setAutoMode() {
                if (!this.esp32IP) { alert('Not connected to ESP32'); return; }
                try {
                    const response = await fetch(`http://${this.esp32IP}/reset`);
                    if (response.ok) this.fetchData();
                    else throw new Error('Failed to set auto mode');
                } catch (error) {
                    alert('Error setting auto mode: ' + error.message);
                }
            }

            async resetToAuto() { await this.setAutoMode(); }
        }

        // Initialize dashboard when page loads
        document.addEventListener('DOMContentLoaded', () => {
            window.dashboard = new SmartVentilationDashboard();
        });
    </script>
</body>
</html>
)=====";

  server.send(200, "text/html", html);
}

// ========== Sensor & Fan Routines ==========

bool detectPerson() {
  bool pir_ok = digitalRead(PIR_PIN);
  bool us_ok = (getDistance() < distanceThreshold);
  if (pir_ok || us_ok) {
    lastMotionTime = millis();
    return true;
  }
  return (millis() - lastMotionTime < timeout);
}

long getDistance() {
  digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH);
  return dur * 0.0343 / 2;
}

void runFanHigh() {
  digitalWrite(FAN_IN1, HIGH);
  digitalWrite(FAN_IN2, LOW);
  fanSpeed = 255;
  ledcWrite(FAN_PWM_CHANNEL, fanSpeed);
}

void runFanMedium() {
  digitalWrite(FAN_IN1, HIGH);
  digitalWrite(FAN_IN2, LOW);
  fanSpeed = 150;
  ledcWrite(FAN_PWM_CHANNEL, fanSpeed);
}

void stopFan() {
  digitalWrite(FAN_IN1, LOW);
  digitalWrite(FAN_IN2, LOW);
  fanSpeed = 0;
  ledcWrite(FAN_PWM_CHANNEL, fanSpeed);
}

void controlFanManual() {
  if (manualFanSpeed > 0) {
    digitalWrite(FAN_IN1, HIGH);
    digitalWrite(FAN_IN2, LOW);
    ledcWrite(FAN_PWM_CHANNEL, manualFanSpeed);
  } else {
    stopFan();
  }
}

// ========== ThingSpeak Integration ==========

void sendDataToThingSpeak(float t, float h, int mq6v, int mq135v, bool person, int fSpeed) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String fullUrl = tsUrl + "&field1=" + String(t)
                     + "&field2=" + String(h)
                     + "&field3=" + String(mq6v)
                     + "&field4=" + String(mq135v)
                     + "&field5=" + String(person ? 1 : 0)
                     + "&field6=" + String(fSpeed);
    http.begin(fullUrl);
    int code = http.GET();
    Serial.print("TS response: "); Serial.println(code);
    http.end();
  }
}

void sendTelegramAlert(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      Serial.println("Telegram alert sent.");
    } else {
      Serial.println("Telegram alert failed.");
    }
    http.end();
  }
}
