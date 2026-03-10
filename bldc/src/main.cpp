#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// --- Configuration ---
const char* ssid = "IB3";
const char* password = "ingenieursbeleving3";

Servo esc1;
Servo esc2;
const int esc1Pin = 18;
const int esc2Pin = 19;

WebServer server(80);

// Updated HTML with Master Control and Individual Toggles
const char PAGE_MAIN[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Dual ESC Master Control</title>
    <style>
        body { font-family: 'Segoe UI', sans-serif; text-align: center; background: #1a1a1a; color: white; padding: 20px; }
        .container { max-width: 450px; margin: auto; background: #2d2d2d; padding: 25px; border-radius: 20px; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
        .slider { width: 100%; height: 15px; border-radius: 10px; background: #444; outline: none; margin: 15px 0; -webkit-appearance: none; }
        .slider::-webkit-slider-thumb { -webkit-appearance: none; width: 25px; height: 25px; border-radius: 50%; background: #00adb5; cursor: pointer; border: 3px solid #fff; }
        h1 { color: #00adb5; margin-bottom: 30px; }
        h3 { color: #ccc; margin: 10px 0 5px 0; }
        .val-display { font-size: 1.8rem; color: #00adb5; font-weight: bold; font-family: monospace; }
        .master-section { border: 2px solid #00adb5; padding: 15px; border-radius: 15px; margin-bottom: 25px; background: #252525; }
        .sync-toggle { margin: 10px; padding: 10px; background: #444; display: inline-block; border-radius: 8px; cursor: pointer; }
        .sync-active { background: #00adb5; color: #1a1a1a; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>ESC Controller</h1>
        
        <div class="master-section">
            <h3>MASTER CONTROL</h3>
            <div id="valM" class="val-display">1000</div>
            <input type="range" min="1000" max="2000" value="1000" class="slider" id="sliderM">
            <div id="syncBtn" class="sync-toggle sync-active" onclick="toggleSync()">SYNC MOTORS: ON</div>
        </div>

        <h3>Motor 1 (GPIO 18)</h3>
        <div id="val1" class="val-display">1000</div>
        <input type="range" min="1000" max="2000" value="1000" class="slider" id="slider1">

        <h3>Motor 2 (GPIO 19)</h3>
        <div id="val2" class="val-display">1000</div>
        <input type="range" min="1000" max="2000" value="1000" class="slider" id="slider2">
    </div>
    
    <script>
        let isSynced = true;
        const s1 = document.getElementById("slider1");
        const s2 = document.getElementById("slider2");
        const sM = document.getElementById("sliderM");

        function toggleSync() {
            isSynced = !isSynced;
            const btn = document.getElementById("syncBtn");
            btn.innerHTML = isSynced ? "SYNC MOTORS: ON" : "SYNC MOTORS: OFF";
            btn.className = isSynced ? "sync-toggle sync-active" : "sync-toggle";
        }

        function sendData(motor, val) {
            document.getElementById("val" + motor).innerHTML = val;
            fetch(`/setThrottle?motor=${motor}&val=${val}`);
        }

        sM.oninput = function() {
            let v = this.value;
            document.getElementById("valM").innerHTML = v;
            s1.value = v;
            s2.value = v;
            sendData(1, v);
            sendData(2, v);
        }

        s1.oninput = function() {
            sendData(1, this.value);
            if(isSynced) {
                s2.value = this.value;
                sM.value = this.value;
                document.getElementById("valM").innerHTML = this.value;
                sendData(2, this.value);
            }
        }

        s2.oninput = function() {
            sendData(2, this.value);
            if(isSynced) {
                s1.value = this.value;
                sM.value = this.value;
                document.getElementById("valM").innerHTML = this.value;
                sendData(1, this.value);
            }
        }
    </script>
</body>
</html>
)=====";

// ... [Rest of the handleRoot, handleThrottle, and loop code remains the same as the previous response] ...

void handleRoot() {
  server.send(200, "text/html", PAGE_MAIN);
}

void handleThrottle() {
  if (server.hasArg("motor") && server.hasArg("val")) {
    int motorNum = server.arg("motor").toInt();
    int val = server.arg("val").toInt();
    if (motorNum == 1) esc1.writeMicroseconds(val);
    else if (motorNum == 2) esc2.writeMicroseconds(val);
    server.send(200, "text/plain", "OK");
  }
}

void setup() {
  Serial.begin(115200);
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  
  esc1.setPeriodHertz(50);
  esc1.attach(esc1Pin, 1000, 2000);
  esc1.writeMicroseconds(1000);

  esc2.setPeriodHertz(50);
  esc2.attach(esc2Pin, 1000, 2000);
  esc2.writeMicroseconds(1000);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi Connected! IP: " + WiFi.localIP().toString());

  server.on("/", handleRoot);
  server.on("/setThrottle", handleThrottle);
  server.begin();
}

void loop() {
  server.handleClient();
  if (WiFi.status() != WL_CONNECTED) {
    esc1.writeMicroseconds(1000); esc2.writeMicroseconds(1000);
    WiFi.begin(ssid, password);
    delay(1000);
  }
}