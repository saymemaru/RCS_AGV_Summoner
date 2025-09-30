#define LED_PIN     13
#define BUTTON_PIN  16

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WebServer.h>

//wifi
const char* ssid = "T_2.4G";
const char* password = "";

// 定义 LED 逻辑值
int led_logic = 0;
// 判断 LED 的状态是否改变过
bool status = false;

//http配置
HTTPClient http;
String serverUrl = "http://192.168.2.101:50060/Task/CreateTask";
// 创建WebServer对象，监听端口239
WebServer server(239);

// 时间跟踪变量
unsigned long lastPressedTime = 0;
const unsigned long cooldownPeriod = 5000;  // 5秒冷却时间

//json
StaticJsonDocument<200> doc;
String globalJsonString;
JsonArray variables;

void setup() 
{
  //串口波特率
  Serial.begin(115200);

  //初始化led和按钮
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);

  //构建request
  buildJsonRequest();
  
  // 连接 WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    }
  delay(1000); 
  Serial.println("Connected");
  Serial.print("IP :");
  Serial.println(WiFi.localIP());

  server.begin();
  Serial.println("HTTP server start");
}

void loop() {

  // 按键消抖
  if (digitalRead(BUTTON_PIN) == HIGH) 
  {
    // 睡眠 10ms，如果依然为高电平，说明抖动已消失。
    delay(10);
    //按下按钮，且之前未按下
    if (digitalRead(BUTTON_PIN) && status == false) 
    {
      //当前时间距离上次按下时间超过了cooldownPeriod
      if (millis() - lastPressedTime > cooldownPeriod)
      {
        led_logic = !led_logic;
        digitalWrite(LED_PIN, led_logic);
       
        sendPostRequest();  // 发送HTTP请求

        Serial.print("更新请求时间");
        // led 的状态发生了变化，即使持续按着按键，LED 的状态也不应该改变。
        status = true;
        lastPressedTime = millis();  // 更新按下时间
         
      }
    }
  }
  else
  {
    //当前时间距离上次按下时间超过了cooldownPeriod
    if (millis() - lastPressedTime > cooldownPeriod)
    {
      status = false;  
      led_logic = false;
      digitalWrite(LED_PIN, led_logic);
    }
  }  
  // put your main code here, to run repeatedly:
  //digitalWrite(2, HIGH);
  //delay(1000);
  //digitalWrite(2, LOW);
  //delay(1000);
}

String hardwareRandom()
{
  // 获取当前时间（秒数）
  //time_t now;
  //time(&now);
  //randomSeed(now);
  //return String(random(1000000));
  return String(esp_random() % 1000000);
}

//setup时调用
void buildJsonRequest() 
{
  // 创建DynamicJsonDocument对象（根据JSON大小调整容量）
  //const size_t capacity = JSON_OBJECT_SIZE(7) + JSON_ARRAY_SIZE(1) + 200;
  //DynamicJsonDocument doc(capacity);
  // 添加键值对到JSON对象
  //doc.clear();
  doc["SysToken"] = "192168239";  // 使用全局token变量
  doc["ReceiveTaskID"] = hardwareRandom();
  doc["MapCode"] = "2";
  doc["TaskCode"] = "ESP32MOVE";
  doc["AgvGroupCode"] = "";
  doc["AGVCode"] = "3";
  doc["IsGoParking"] = "";
  // Variables数组
  variables = doc.createNestedArray("Variables");
  // 向数组中添加对象
  //JsonObject var1 = variables.createNestedObject();
  //var1["Code"] = "StartPoint";
  //var1["Value"] = "PNT_30";

  // 序列化JSON到全局字符串变量
  serializeJson(doc, globalJsonString);
}


// 修改sendPostRequest方法，每次创建新的HTTPClient实例
void sendPostRequest() 
{
  // 检查WiFi连接状态，必要时重新连接
  if (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("WiFi连接断开,尝试重新连接...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(1000); // 等待1秒
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi重新连接失败,无法发送请求");
      return;
    }
  }

  // 设置超时（重要！）
  http.setConnectTimeout(10000);  // 10秒连接超时
  http.setTimeout(10000);         // 10秒响应超时
  
  // 开始HTTP连接
  Serial.print("连接到服务器: ");
  Serial.println(serverUrl);
  if (!http.begin(serverUrl)) {
    Serial.println("无法开始HTTP连接");
    http.end();
    return;}
  // 禁止连接重用
  //http.setReuse(false); 

  // 设置HTTP头,创建request
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "ESP32-HTTPClient");
  buildJsonRequest();
  
  // 发送POST请求并获取响应代码
  int httpResponseCode = http.POST(globalJsonString);
  Serial.print("已发送请求");
  
  delay(2000);
  // 检查响应
  if (httpResponseCode > 0) {
    Serial.print("HTTP响应代码: ");
    Serial.println(httpResponseCode);
    // 获取响应内容
    String response = http.getString();
    Serial.println("服务器响应: ");
    Serial.println(response);} 
  else {
    Serial.print("错误代码: ");
    Serial.println(httpResponseCode);
    Serial.println("请求失败");}
  
  // 结束连接
  http.end();
}




