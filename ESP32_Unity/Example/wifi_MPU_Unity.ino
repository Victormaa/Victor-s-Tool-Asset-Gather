#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

const char* ssid = "shen2.4G";//"AmiukeiPhone";//"马伟鹏的iPhone";
const char* password = "13011301";//"artdicosogood";
const char* udpAddress = "192.168.0.114";//"192.168.33.199"; // Unity 运行的电脑 IP
const int udpPort = 5001;

WiFiUDP udp;
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // 初始化 MPU6050
  if (!mpu.begin()) {
    Serial.println("无法找到 MPU6050，请检查连接！");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 已初始化");

  // 设置 MPU6050 量程（可选）
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // 连接 WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP Address: ");

  Serial.println(WiFi.localIP());
  Serial.println("\nWiFi connected");

  // 初始化 UDP
  udp.begin(udpPort);
}

void loop() {
  // 获取传感器事件
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // 构造字符串（加速度 + 陀螺仪）
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

  delay(20); // 50Hz
}
