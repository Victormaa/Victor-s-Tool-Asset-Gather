#include <WiFi.h>
#include <Wire.h>//d:\GameDev\Don'tSendMetoBed\Assets\Resources\Arduino\test_tuoluo\test_tuoluo.ino
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Preferences.h>

// 添加蜂鸣器引脚定义
#define BUZZER_PIN 23

// 添加蜂鸣器响应类型枚举
enum BuzzerType {
  BUZZER_NONE = 0,
  BUZZER_SHORT_BEEP,
  BUZZER_DOUBLE_BEEP,
  BUZZER_LONG_BEEP,
  BUZZER_SUCCESS,
  BUZZER_ERROR,
  BUZZER_ALERT,
  BUZZER_KEEP_BEEP
};

Preferences preferences;
WiFiUDP udp;
Adafruit_MPU6050 mpu;

// 添加蜂鸣器状态变量
BuzzerType buzzerCommand = BUZZER_NONE;
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;

struct WiFiConfig {
  char ssid[32];
  char password[64];
  char udpAddress[16];
  int udpPort;
  bool configured;
};

WiFiConfig config;
bool systemReady = false;

// 在文件开头添加新的变量
#define UDP_COMMAND_PORT 5002  // 专门用于接收命令的端口
WiFiUDP udpCommand;  // 专门用于命令接收的UDP实例

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // 初始化蜂鸣器引脚
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  
  // 启动蜂鸣器测试
  playBuzzer(BUZZER_SHORT_BEEP);
  
  preferences.begin("wifi-config", false);
  loadConfig();
  
  if (config.configured) {
    Serial.println("Found saved configuration, attempting auto-connect...");
    Serial.print("SSID: "); Serial.println(config.ssid);
    Serial.print("Target IP: "); Serial.println(config.udpAddress);
    Serial.print("Port: "); Serial.println(config.udpPort);
    
    if (connectToWiFi()) {
      initializeMPU();
      udp.begin(config.udpPort);

      // 初始化命令接收UDP端口
      udpCommand.begin(UDP_COMMAND_PORT);

      systemReady = true;
      Serial.println("System ready, starting data transmission");
    } else {
      Serial.println("Auto-connect failed, waiting for commands...");
      systemReady = false;
    }
  } else {
    Serial.println("No saved configuration, waiting for commands...");
    Serial.println("Available commands:");
    Serial.println("  STATUS - Check current status");
    Serial.println("  CONFIG:ssid,password,ip,port - Send configuration");
    Serial.println("  RESET_CONFIG - Clear saved configuration");
    systemReady = false;
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

bool connectToWiFi() {
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
    return true;
  } else {
    Serial.println("\nWiFi connection failed!");
    return false;
  }
}

void initializeMPU() {
  Serial.println("Initializing MPU6050...");
  
  bool success = false;
  for (int i = 0; i < 5; i++) {
    if (mpu.begin()) {
      success = true;
      break;
    }
    Serial.println("MPU init failed, retrying...");
    delay(200);
  }

  if (!success) {
    Serial.println("Error: MPU6050 not found after retries!");
    return;
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  delay(100);  // 给滤波器一点时间稳定

  Serial.println("MPU6050 initialized successfully");
}

void resetConfig() {
  preferences.clear();
  Serial.println("Configuration cleared");
  config.configured = false;
  systemReady = false;
}

void processCommand(String command) {
  command.trim();
  
  if (command == "STATUS") {
    handleStatusCommand();
  } 
  else if (command == "RESET_CONFIG") {
    handleResetCommand();
  }
  else if (command.startsWith("CONFIG:")) {
    handleConfigCommand(command.substring(7)); // Remove "CONFIG:" prefix
  }
  //else if (command.startsWith("BEEP:")) {
  //  handleBuzzerCommand(command);
  //}
  else {
    Serial.println("Unknown command: " + command);
    Serial.println("Available commands: STATUS, CONFIG:ssid,pass,ip,port, RESET_CONFIG");
  }
}

void handleStatusCommand() {
  Serial.println("=== SYSTEM STATUS ===");
  Serial.print("Configuration saved: ");
  Serial.println(config.configured ? "Yes" : "No");
  
  if (config.configured) {
    Serial.print("SSID: "); Serial.println(config.ssid);
    Serial.print("Target IP: "); Serial.println(config.udpAddress);
    Serial.print("Port: "); Serial.println(config.udpPort);
  }
  
  Serial.print("WiFi Status: ");
  Serial.println(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
  }
  
  Serial.print("MPU6050: ");
  Serial.println(mpu.begin() ? "Detected" : "Not found");
  Serial.print("System Ready: ");
  Serial.println(systemReady ? "Yes" : "No");
  Serial.println("====================");
}

void handleResetCommand() {
  resetConfig();
  Serial.println("Configuration reset complete");
  Serial.println("Please send CONFIG command to setup new configuration");
}

void handleConfigCommand(String configData) {
  if (parseConfig(configData)) {
    config.configured = true;
    saveConfig();
    Serial.println("Configuration received and saved successfully!");
    playBuzzer(BUZZER_SHORT_BEEP); // 配置保存成功

    if (connectToWiFi()) {
      initializeMPU();
      udp.begin(config.udpPort);
      systemReady = true;
      Serial.println("System ready, starting data transmission");
      playBuzzer(BUZZER_SUCCESS); // 系统就绪
    } else {
      Serial.println("Configuration saved but WiFi connection failed");
      systemReady = false;
      playBuzzer(BUZZER_ERROR); // 连接失败
    }
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

// ==================== 蜂鸣器控制函数 ====================
void playBuzzer(BuzzerType type) {
  buzzerCommand = type;
  buzzerStartTime = millis();
  buzzerActive = true;
}

void handleBuzzer() {
  if (!buzzerActive) return;
  
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - buzzerStartTime;
  
  switch (buzzerCommand) {
    case BUZZER_SHORT_BEEP:
      if (elapsed < 500) {
        digitalWrite(BUZZER_PIN, HIGH);
      } else if (elapsed < 600) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
      }
      break;
      
    case BUZZER_DOUBLE_BEEP:
      if (elapsed < 100) {
        digitalWrite(BUZZER_PIN, HIGH);
      } else if (elapsed < 200) {
        digitalWrite(BUZZER_PIN, LOW);
      } else if (elapsed < 300) {
        digitalWrite(BUZZER_PIN, HIGH);
      } else if (elapsed < 400) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
      }
      break;
      
    case BUZZER_LONG_BEEP:
      if (elapsed < 1000) {
        digitalWrite(BUZZER_PIN, HIGH);
      } else if (elapsed < 1200) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
      }
      break;
    case BUZZER_KEEP_BEEP:
    digitalWrite(BUZZER_PIN, HIGH);
    
    break;

    case BUZZER_SUCCESS:
      if (elapsed < 100) digitalWrite(BUZZER_PIN, HIGH);
      else if (elapsed < 200) digitalWrite(BUZZER_PIN, LOW);
      else if (elapsed < 300) digitalWrite(BUZZER_PIN, HIGH);
      else if (elapsed < 400) digitalWrite(BUZZER_PIN, LOW);
      else if (elapsed < 500) digitalWrite(BUZZER_PIN, HIGH);
      else if (elapsed < 600) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
      }
      break;
      
    case BUZZER_ERROR:
      if (elapsed < 300) digitalWrite(BUZZER_PIN, HIGH);
      else if (elapsed < 1000) digitalWrite(BUZZER_PIN, LOW);
      else if (elapsed < 1300) digitalWrite(BUZZER_PIN, HIGH);
      else if (elapsed < 2000) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
      }
      break;
      
    case BUZZER_ALERT:
      // 急促的警报声
      if (elapsed % 200 < 100) {
        digitalWrite(BUZZER_PIN, HIGH);
      } else {
        digitalWrite(BUZZER_PIN, LOW);
      }
      if (elapsed > 2000) {
        digitalWrite(BUZZER_PIN, LOW);
        buzzerActive = false;
      }
      break;
      
    case BUZZER_NONE:
    default:
      digitalWrite(BUZZER_PIN, LOW);
      buzzerActive = false;
      break;
  }
}
// 处理来自Unity的蜂鸣器命令
void handleBuzzerCommand(String command) {
  if (command == "BEEP:SHORT") {
    playBuzzer(BUZZER_SHORT_BEEP);
    Serial.println("Playing short beep");
  }
  else if (command == "BEEP:DOUBLE") {
    playBuzzer(BUZZER_DOUBLE_BEEP);
    Serial.println("Playing double beep");
  }
  else if (command == "BEEP:LONG") {
    playBuzzer(BUZZER_LONG_BEEP);
    Serial.println("Playing long beep");
  }
  else if (command == "BEEP:KEEP") {
    playBuzzer(BUZZER_KEEP_BEEP);
    Serial.println("Playing long beep");
  }
  else if (command == "BEEP:SUCCESS") {
    playBuzzer(BUZZER_SUCCESS);
    Serial.println("Playing success tone");
  }
  else if (command == "BEEP:ERROR") {
    playBuzzer(BUZZER_ERROR);
    Serial.println("Playing error tone");
  }
  else if (command == "BEEP:ALERT") {
    playBuzzer(BUZZER_ALERT);
    Serial.println("Playing alert tone");
  }
  else if (command == "BEEP:STOP") {
    buzzerActive = false;
    digitalWrite(BUZZER_PIN, LOW);
    Serial.println("Buzzer stopped");
  }
}
// 添加UDP命令处理函数
void handleUDPCommands() {
  int packetSize = udpCommand.parsePacket();
  if (packetSize) {
    char packetBuffer[255];
    int len = udpCommand.read(packetBuffer, 255);
    if (len > 0) {
      packetBuffer[len] = '\0';
      String command = String(packetBuffer);
      command.trim();
      
      Serial.print("Received UDP command: ");
      Serial.println(command);
      
      // 处理蜂鸣器命令
      if (command.startsWith("BEEP:")) {
        handleBuzzerCommand(command);
      }
    }
  }
}
void loop() {
  // 处理串口命令
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    processCommand(command);
  }
  
  // 处理UDP网络命令（新增）
  handleUDPCommands();
  
  // 处理蜂鸣器
  handleBuzzer();
  
  // 如果系统就绪，发送传感器数据
  if (systemReady && WiFi.status() == WL_CONNECTED) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // String data = String(a.acceleration.x) + "," +
    //               String(a.acceleration.y) + "," +
    //               String(a.acceleration.z) + "," +
    //               String(g.gyro.x) + "," +
    //               String(g.gyro.y) + "," +
    //               String(g.gyro.z);

    // 构建数据字符串，包含IP地址信息
    String data = "IP:" + WiFi.localIP().toString() + "," +
                  "DATA:" + String(a.acceleration.x) + "," +
                  String(a.acceleration.y) + "," +
                  String(a.acceleration.z) + "," +
                  String(g.gyro.x) + "," +
                  String(g.gyro.y) + "," +
                  String(g.gyro.z);

    udp.beginPacket(config.udpAddress, config.udpPort);
    udp.print(data);
    udp.endPacket();
  } 
  // 如果 WiFi 断开但配置存在，尝试重连
  else if (config.configured && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, attempting to reconnect...");
    if (connectToWiFi()) {
      systemReady = true;
    }
  }
  
  delay(20);
}