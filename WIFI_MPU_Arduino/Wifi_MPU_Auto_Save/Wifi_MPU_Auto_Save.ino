#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>

Preferences preferences;
WiFiUDP udp;
Adafruit_MPU6050 mpu;

struct WiFiConfig {
  char ssid[32];
  char password[64];
  char udpAddress[16];
  int udpPort;
  bool configured;
};

WiFiConfig config;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  preferences.begin("wifi-config", false);
  loadConfig();
  
  if (config.configured) {
    Serial.println("Found saved configuration, attempting auto-connect...");
    Serial.print("SSID: "); Serial.println(config.ssid);
    Serial.print("Target IP: "); Serial.println(config.udpAddress);
    Serial.print("Port: "); Serial.println(config.udpPort);
    
    connectToWiFi();
    initializeMPU();
    udp.begin(config.udpPort);
    Serial.println("System ready, starting data transmission");
  } else {
    Serial.println("No saved configuration, waiting for Unity...");
    Serial.println("Please send format: SSID,PASSWORD,IP,PORT");
    waitForConfig();
  }
}

void loadConfig() {
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");
  String savedIP = preferences.getString("ip", "");
  int savedPort = preferences.getInt("port", 0);
  
  if (savedSSID.length() > 0 && savedIP.length() > 0) {
    savedSSID.toCharArray(config.ssid, sizeof(config.ssid));
    savedPassword.toCharArray(config.password, sizeof(config.password));
    savedIP.toCharArray(config.udpAddress, sizeof(config.udpAddress));
    config.udpPort = savedPort;
    config.configured = true;
  } else {
    config.configured = false;
  }
}

void saveConfig() {
  preferences.putString("ssid", config.ssid);
  preferences.putString("password", config.password);
  preferences.putString("ip", config.udpAddress);
  preferences.putInt("port", config.udpPort);
  preferences.putBool("configured", true);
  Serial.println("Configuration saved to flash memory");
}

void waitForConfig() {
  while (!config.configured) {
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      
      if (parseConfig(input)) {
        config.configured = true;
        saveConfig();
        Serial.println("Configuration received and saved successfully!");
        
        connectToWiFi();
        initializeMPU();
        udp.begin(config.udpPort);
        Serial.println("System ready, starting data transmission");
      }
    }
    delay(100);
  }
}

bool parseConfig(String configData) {
  int firstComma = configData.indexOf(',');
  int secondComma = configData.indexOf(',', firstComma + 1);
  int thirdComma = configData.indexOf(',', secondComma + 1);
  
  if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
    Serial.println("Error: Invalid format! Use: SSID,PASSWORD,IP,PORT");
    return false;
  }
  
  String ssidStr = configData.substring(0, firstComma);
  String passwordStr = configData.substring(firstComma + 1, secondComma);
  String ipStr = configData.substring(secondComma + 1, thirdComma);
  String portStr = configData.substring(thirdComma + 1);
  
  ssidStr.toCharArray(config.ssid, sizeof(config.ssid));
  passwordStr.toCharArray(config.password, sizeof(config.password));
  ipStr.toCharArray(config.udpAddress, sizeof(config.udpAddress));
  config.udpPort = portStr.toInt();
  
  Serial.println("Received configuration:");
  Serial.print("SSID: "); Serial.println(config.ssid);
  Serial.print("Target IP: "); Serial.println(config.udpAddress);
  Serial.print("Port: "); Serial.println(config.udpPort);
  
  return true;
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(config.ssid);
  
  WiFi.begin(config.ssid, config.password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected successfully!");
    Serial.print("ESP32 Local IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed! Please check configuration");
    while(1) { delay(1000); }
  }
}

void initializeMPU() {
  Serial.println("Initializing MPU6050...");
  
  if (!mpu.begin()) {
    Serial.println("Error: MPU6050 not found! Please check connections");
    while (1) { delay(10); }
  }
  
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU6050 initialized successfully");
}

void resetConfig() {
  preferences.clear();
  Serial.println("Configuration cleared, restart required for new setup");
  config.configured = false;
}

void loop() {
  if (config.configured && WiFi.status() == WL_CONNECTED) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    String data = String(a.acceleration.x) + "," +
                  String(a.acceleration.y) + "," +
                  String(a.acceleration.z) + "," +
                  String(g.gyro.x) + "," +
                  String(g.gyro.y) + "," +
                  String(g.gyro.z);

    udp.beginPacket(config.udpAddress, config.udpPort);
    udp.print(data);
    udp.endPacket();
  } else if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    connectToWiFi();
  }
  
  delay(20);
}

void serialEvent() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command == "RESET_CONFIG") {
      resetConfig();
      ESP.restart();
    }
    else if (command == "SHOW_CONFIG") {
      Serial.println("Current configuration:");
      Serial.print("SSID: "); Serial.println(config.ssid);
      Serial.print("Target IP: "); Serial.println(config.udpAddress);
      Serial.print("Port: "); Serial.println(config.udpPort);
    }
    else if (command == "STATUS") {
      Serial.print("WiFi Status: ");
      Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print("Signal Strength: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
      }
    }
    else if (command == "TEST_CONNECTION") {
      Serial.println("ESP32_MPU6050_READY");
    }
  }
}