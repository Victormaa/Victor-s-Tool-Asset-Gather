#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// 全局变量
char ssid[32] = "";
char password[64] = "";
char udpAddress[16] = "";
int udpPort = 0;
bool wifiConfigured = false;
bool mpuInitialized = false;

WiFiUDP udp;
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  Serial.println("ESP32 等待 Unity 配置...");
  Serial.println("请发送格式: SSID,PASSWORD,IP,PORT");
  
  // 等待接收 Unity 的配置信息
  while (!wifiConfigured) {
    if (Serial.available() > 0) {
      String configData = Serial.readStringUntil('\n');
      configData.trim();
      
      if (parseConfig(configData)) {
        wifiConfigured = true;
        Serial.println("配置接收成功!");
      }
    }
    delay(100);
  }
  
  // 连接 WiFi
  connectToWiFi();
  
  // 初始化 MPU6050
  initializeMPU();
  
  // 初始化 UDP
  udp.begin(udpPort);
  Serial.println("UDP 初始化完成");
  Serial.println("开始传输 MPU6050 数据...");
}

bool parseConfig(String configData) {
  int firstComma = configData.indexOf(',');
  int secondComma = configData.indexOf(',', firstComma + 1);
  int thirdComma = configData.indexOf(',', secondComma + 1);
  
  if (firstComma == -1 || secondComma == -1 || thirdComma == -1) {
    Serial.println("配置格式错误! 请使用: SSID,PASSWORD,IP,PORT");
    return false;
  }
  
  // 解析 SSID
  String ssidStr = configData.substring(0, firstComma);
  ssidStr.toCharArray(ssid, sizeof(ssid));
  
  // 解析 Password
  String passwordStr = configData.substring(firstComma + 1, secondComma);
  passwordStr.toCharArray(password, sizeof(password));
  
  // 解析 IP 地址
  String ipStr = configData.substring(secondComma + 1, thirdComma);
  ipStr.toCharArray(udpAddress, sizeof(udpAddress));
  
  // 解析端口
  String portStr = configData.substring(thirdComma + 1);
  udpPort = portStr.toInt();
  
  // 打印配置信息
  Serial.println("接收到的配置:");
  Serial.print("SSID: "); Serial.println(ssid);
  Serial.print("Password: "); Serial.println(password);
  Serial.print("IP: "); Serial.println(udpAddress);
  Serial.print("Port: "); Serial.println(udpPort);
  
  return true;
}

void connectToWiFi() {
  Serial.print("正在连接 WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi 连接成功!");
    Serial.print("IP 地址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi 连接失败!");
    while(1) { delay(1000); } // 停止执行
  }
}

void initializeMPU() {
  Serial.println("初始化 MPU6050...");
  
  if (!mpu.begin()) {
    Serial.println("无法找到 MPU6050，请检查连接！");
    while (1) {
      delay(10);
    }
  }
  
  // 设置 MPU6050 参数
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  
  mpuInitialized = true;
  Serial.println("MPU6050 初始化完成");
}

void loop() {
  if (wifiConfigured && mpuInitialized && WiFi.status() == WL_CONNECTED) {
    // 获取传感器事件
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // 构造数据字符串
    String data = String(a.acceleration.x) + "," +
                  String(a.acceleration.y) + "," +
                  String(a.acceleration.z) + "," +
                  String(g.gyro.x) + "," +
                  String(g.gyro.y) + "," +
                  String(g.gyro.z);

    // 发送 UDP 数据
    udp.beginPacket(udpAddress, udpPort);
    udp.print(data);
    udp.endPacket();

    // 可选：在串口也输出数据用于调试
    // Serial.println("发送数据: " + data);
  }
  
  delay(20); // 50Hz
}