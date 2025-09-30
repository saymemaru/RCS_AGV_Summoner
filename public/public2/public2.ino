#define BUTTON_4  16
#define BUTTON_3  17
#define BUTTON_2  18
#define BUTTON_1  19
#define LED_B     32
#define LED_4     27
#define LED_3     26
#define LED_2     25
#define LED_1     33

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WebServer.h>

enum SystemState 
{
  STATE_IDLE,
  STATE_LED_ON,
  STATE_COOLDOWN
};

SystemState currentState = STATE_IDLE;

const int ledPins[] = {LED_1, LED_2, LED_3, LED_4};

//WIFI
const char* ssid = "";
const char* password = "";

//http配置
HTTPClient http;
// RCS API地址
String serverUrl = "http://192.168.2.101:50060/Task/CreateTask";
// 监听端口239
WebServer server(239);

//json
StaticJsonDocument<200> doc;
String globalJsonString;
JsonArray variables;

//中断标识
volatile bool interruptEnabled = true;
volatile unsigned long interruptTime = 0; //上次记录的中断时间
volatile int activeLed = -1; // -1:无LED亮, 3:LED3亮, 4:LED4亮
const unsigned long DEBOUNCE_DELAY = 50; // 防抖延时50ms
const unsigned long cooldownPeriod = 5000;  // 5秒冷却时间

// 中断服务函数
void IRAM_ATTR button1ISR()  
{
  // 如果中断被禁用（LED正在亮），直接返回
  if (!interruptEnabled) 
  {
    return;
  }
  // 禁用中断，防止重复触发
  interruptEnabled = false;
  activeLed = 1;
  // 记录LED开始点亮的时间
  interruptTime = millis();
}
void IRAM_ATTR button2ISR()  
{
  // 如果中断被禁用（LED正在亮），直接返回
  if (!interruptEnabled) 
  {
    return;
  }
  // 禁用中断，防止重复触发
  interruptEnabled = false;
  activeLed = 2;
  // 记录LED开始点亮的时间
  interruptTime = millis();
}
void IRAM_ATTR button3ISR()  
{
  // 如果中断被禁用（LED正在亮），直接返回
  if (!interruptEnabled) 
  {
    return;
  }
  // 禁用中断，防止重复触发
  interruptEnabled = false;
  activeLed = 3;
  // 记录LED开始点亮的时间
  interruptTime = millis();
}
void IRAM_ATTR button4ISR() 
{
  // 如果中断被禁用（LED正在亮），直接返回
  if (!interruptEnabled) 
  {
    return;
  }
  // 禁用中断，防止重复触发
  interruptEnabled = false;
  activeLed = 4;
  // 记录LED开始点亮的时间
  interruptTime = millis();
}

void setup() 
{
  //串口波特率
  Serial.begin(115200);

  //配置led和button引脚
  pinMode(LED_B, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);
  pinMode(BUTTON_1, INPUT_PULLDOWN);
  pinMode(BUTTON_2, INPUT_PULLDOWN);
  pinMode(BUTTON_3, INPUT_PULLDOWN);
  pinMode(BUTTON_4, INPUT_PULLDOWN);

  //配置中断引脚
  attachInterrupt(digitalPinToInterrupt(BUTTON_1), button1ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_2), button2ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_3), button3ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_4), button4ISR, FALLING);

  // 设置超时（重要！）
  http.setConnectTimeout(10000);  // 10秒连接超时
  http.setTimeout(10000);         // 10秒响应超时

  
  // 连接 WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_B, HIGH);
    delay(500);
    digitalWrite(LED_B, LOW);
    //Serial.print(".");
    }
  Serial.print("IP :");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_B, LOW);

  server.begin();
  Serial.println("HTTP server start");

}

void loop() 
{
  switch (currentState)
  {
    case STATE_IDLE:
    if (activeLed != -1) 
    {
      digitalWrite(ledPins[activeLed - 1], HIGH);
      sendPostRequest();
      currentState = STATE_LED_ON;
      activeLed = -1;
    }
    break;

    case STATE_LED_ON:
    if(millis() - interruptTime >= cooldownPeriod)
    {
      digitalWrite(LED_1, LOW);
      digitalWrite(LED_2, LOW);
      digitalWrite(LED_3, LOW);
      digitalWrite(LED_4, LOW);
      interruptEnabled = true;
      currentState = STATE_IDLE;
    }
    break;
  }
  // 短暂延时，减少CPU占用
  delay(50);
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

//改为工厂
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

  digitalWrite(LED_B, HIGH);
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
  digitalWrite(LED_B, LOW);
}




