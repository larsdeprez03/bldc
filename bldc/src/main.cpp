#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <ArduinoOTA.h>
#include <MPU9250.h>
#include <LittleFS.h>

// --- Configuration ---
const char* ssid = "IB3";
const char* password = "ingenieursbeleving3";

Servo esc1, esc2;
const int esc1Pin = 18;
const int esc2Pin = 19;

MPU9250 mpu;
WebServer server(80);

float pitchOffset = 0.0, rollOffset = 0.0;

// Helper to serve the HTML file
void handleRoot() {
    if (LittleFS.exists("/index.html")) {
        File file = LittleFS.open("/index.html", "r");
        server.streamFile(file, "text/html");
        file.close();
    } else {
        server.send(404, "text/plain", "Error: index.html not found on Flash. Did you 'Upload Filesystem Image'?");
    }
}

// Endpoint to see what files are actually on the chip
void handleListFiles() {
    String str = "Files list:\n";
    File root = LittleFS.open("/");
    File file = root.openNextFile();
    while(file){
        str += String(file.name()) + " (" + String(file.size()) + " bytes)\n";
        file = root.openNextFile();
    }
    server.send(200, "text/plain", str);
}

void handleSetZero() {
    if (mpu.update()) {
        pitchOffset = mpu.getPitch();
        rollOffset = mpu.getRoll();
        server.send(200, "text/plain", "Zero Set");
    }
}

void handleAngles() {
    if (mpu.update()) {
        String json = "{\"pitch\":" + String(mpu.getPitch() - pitchOffset) + 
                      ",\"roll\":" + String(mpu.getRoll() - rollOffset) + "}";
        server.send(200, "application/json", json);
    }
}

void handleThrottle() {
    if (server.hasArg("motor") && server.hasArg("val")) {
        int m = server.arg("motor").toInt();
        int v = server.arg("val").toInt();
        if (m == 1) esc1.writeMicroseconds(v);
        else if (m == 2) esc2.writeMicroseconds(v);
        server.send(200, "text/plain", "OK");
    }
}

void setup() {
    Serial.begin(115200);
    Wire.begin();

    // 1. Initialize File System
    if (!LittleFS.begin(true)) {
        Serial.println("LittleFS Mount Failed");
    }

    // 2. Setup Sensors and Motors
    if (!mpu.setup(0x68)) Serial.println("MPU Failed");
    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    esc1.attach(esc1Pin, 1000, 2000);
    esc2.attach(esc2Pin, 1000, 2000);
    esc1.writeMicroseconds(1000);
    esc2.writeMicroseconds(1000);

    // 3. Connect WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
    
    // 4. Setup OTA
    ArduinoOTA.setHostname("ESC-IMU-Controller");
    ArduinoOTA.begin();

    // 5. Routes
    server.on("/", handleRoot);
    server.on("/list", handleListFiles); // Secret debug route
    server.on("/getAngles", handleAngles);
    server.on("/setZero", handleSetZero);
    server.on("/setThrottle", handleThrottle);
    
    server.begin();
    Serial.println("\nSystem Ready at: " + WiFi.localIP().toString());
}

void loop() {
    server.handleClient();
    ArduinoOTA.handle();
    mpu.update();

    // Safety: Stop motors if WiFi drops
    if (WiFi.status() != WL_CONNECTED) {
        esc1.writeMicroseconds(1000); 
        esc2.writeMicroseconds(1000);
        WiFi.begin(ssid, password);
        delay(1000);
    }
}