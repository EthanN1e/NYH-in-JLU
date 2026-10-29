//----------------------------------------------
//机器人梦工厂机器狗（180度舵机版本）
//基于 EDA-Robot 开源项目二次开发；新增交互与品牌画面
//---------------导入库--------------------------
#include <Arduino.h>
#include <Servo.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <FS.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include "image.cpp"
//---------------按键部分--------------------------
#define BUTTON_PIN 2 // GPIO2 引脚 (D4)
#define BUTTON_PIN2 15
volatile bool buttonPressed = false;     // 按键标志
volatile bool buttonPressed2 = false;    // 按键标志
unsigned long lastPressTime = 0;         // 上次按键时间
const unsigned long debounceDelay = 50;  // 消抖时间 (ms)
unsigned long lastPressTime2 = 0;        // 上次按键时间
const unsigned long debounceDelay2 = 50; // 消抖时间 (ms)
//---------------ADC部分--------------------------
const float voltageDividerRatio = 8.4; // 分压比（8.4倍缩小）
const float minVoltage = 6.4; // 电压为0%时
const float maxVoltage = 8.4; // 电压为100%时
const int numSamples = 10;//定义采用次数
float batteryVoltage = 0; // 计算电池电压
int batteryPercentage = 0;//电量百分比
//---------------舵机部分--------------------------
Servo servo1;//声明舵机1
Servo servo2;//声明舵机2
Servo servo3;//声明舵机3
Servo servo4;//声明舵机4
int engine1 = 14;                 // 舵机1引脚
int engine2 = 16;                 // 舵机2引脚
int engine3 = 12;                 // 舵机3引脚
int engine4 = 13;                 // 舵机4引脚
//---------------屏幕部分--------------------------
U8G2_SSD1306_128X64_NONAME_1_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/5, /* data=*/4); // 使用SSD1306屏幕驱动,时钟引脚5，数据引脚4
//---------------网络部分--------------------------
const char *ssid = "RobotDreamDog";//WIFI名称
const char *password = ""; // 无密码
AsyncWebServer server(80);//设置服务器端口
WiFiUDP ntpUDP;//声明UDP
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 8 * 3600, 60000);//配置NTP服务器
//---------------API部分--------------------------
const char *weatherAPI = "http://api.seniverse.com/v3/weather/daily.json?key=";//心知天气API地址
String temperature = "";//天气温度
String humidity = "";//天气湿度
String weather = "";//天气
String cityname = "";//城市名称
String weatherapi = "";//心知天气API密钥
//---------------标签部分--------------------------
bool initweather = false;  // 天气初始化
bool freestate = false;//自由模式标签
int prevEmojiState = -1; // 用于跟踪之前的 emojiState
int actionstate = 0;//活动状态标签
int emojiState = 0; // 表情状态标签
//---------------文件系统部分--------------------------
const char *ssidFile = "/ssid.json";//配置存储文件名及路径
//---------------按键中断部分--------------------------
void ICACHE_RAM_ATTR handleButtonPress()
{
    
    unsigned long currentTime = millis();// 获取当前系统运行时间（单位：毫秒）
    
    if (currentTime - lastPressTime > debounceDelay) // 检查按钮1是否满足去抖条件（避免机械抖动导致的误触发）
    {
        buttonPressed = true;     // 设置按钮1按下标志位
        lastPressTime = currentTime; // 更新按钮1的最后有效按下时间
    }
    unsigned long currentTime2 = millis();// 获取当前时间
    if (currentTime2 - lastPressTime2 > debounceDelay2)// 检查按钮2的去抖条件（使用独立的去抖时间和记录变量）
    {
        buttonPressed2 = true;    // 设置按钮2按下标志位
        lastPressTime2 = currentTime2; // 更新按钮2的最后有效按下时间
    }
}
//---------------配置页面路由--------------------------
void handleWiFiConfig()
{

server.on("/front", HTTP_GET, [](AsyncWebServerRequest *request) {// 当访问 /front 路径时触发舵机动作
    actionstate = 1;  // 更新全局动作状态标志（1通常表示前进/前方动作）
    
    
    request->send(200, "text/plain", "Front function started"); // 立即响应客户端，避免阻塞（状态码200，返回纯文本确认信息）
});
//以下函数相同，不再注释
    server.on("/back", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       actionstate = 4;   // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/left", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       actionstate = 2;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/right", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       actionstate = 3;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/toplefthand", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        actionstate = 5;   // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/toprighthand", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        actionstate = 6;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/sitdown", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        actionstate = 8;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/lie", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      actionstate = 7; 
        request->send(200, "text/plain", "Front function started"); });
         server.on("/sleep", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      actionstate = 10; 
        request->send(200, "text/plain", "Front function started"); });
    server.on("/free", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      freestate=true;
        request->send(200, "text/plain", "Front function started"); });
    server.on("/offfree", HTTP_GET, [](AsyncWebServerRequest *request)
              {
      freestate=false;
        request->send(200, "text/plain", "Front function started"); });
    server.on("/histate", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        emojiState = 0;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/angrystate", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        emojiState = 1;   // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/brand", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        emojiState = 9;
        request->send(200, "text/plain; charset=UTF-8", "机器人梦工厂品牌画面"); });
    server.on("/dream", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        freestate = false;
        actionstate = 5;  // 沿用已有抬左手动作，作为招手互动
        emojiState = 10;
        request->send(200, "text/plain; charset=UTF-8", "梦工厂小狗向你问好"); });

    server.on("/errorstate", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       emojiState = 2;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/batteryVoltage", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(batteryVoltage)); });
    server.on("/batteryPercentage", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", String(batteryPercentage)); });
    server.on("/dowhatstate", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       emojiState = 3;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/lovestate", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       emojiState = 4;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/sickstate", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       emojiState = 5;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });
    server.on("/yunstate", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       emojiState = 6; 
        request->send(200, "text/plain", "Front function started"); });
    server.on("/time", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       emojiState = 8; 
        request->send(200, "text/plain", "Front function started"); });
    server.on("/weather", HTTP_GET, [](AsyncWebServerRequest *request)
              {
       emojiState = 7;  // 设置标志，执行舵机动作
        request->send(200, "text/plain", "Front function started"); });

    server.on("/connect", HTTP_POST, [](AsyncWebServerRequest *request)
              {
        // 获取POST参数：ssid、pass、city、api
        String ssid = request->getParam("ssid", true)->value();
        String pass = request->getParam("pass", true)->value();
        String city = request->getParam("city", true)->value();
        String api = request->getParam("api", true)->value();

        // 打印接收到的参数
        Serial.println(ssid);
        Serial.println(pass);

        // 保存WiFi信息到JSON文件
        DynamicJsonDocument doc(1024);
        doc["ssid"] = ssid;
        doc["pass"] = pass;
        doc["city"] = city;
        doc["api"] = api;
        fs::File file = SPIFFS.open(ssidFile, "w");  // 打开文件进行写入
        if (file) {
            serializeJson(doc, file);  // 将JSON内容写入文件
            file.close();  // 关闭文件
        }

        // 更新全局变量
        cityname = city;
        weatherapi = api;

        // 开始连接WiFi
        WiFi.begin(ssid.c_str(), pass.c_str());
        // 发送HTML响应，告知用户正在连接
        // 发送带UTF-8编码声明的HTML响应
request->send(200, "text/html; charset=UTF-8", 
    "<!DOCTYPE html>"
    "<html>"
    "<head>"
    "    <meta charset='UTF-8'>"
    "    <title>状态</title>"
    "</head>"
    "<body>"
    "    <h1>请返回使用在线功能，如果能正常获取则配置成功！</h1>"
    "</body>"
    "</html>"
);}
   );

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        // 检查SPIFFS文件系统中是否存在index.html文件
        if (SPIFFS.exists("/index.html")) {
            fs::File file = SPIFFS.open("/index.html", "r");  // 打开index.html文件
            if (file) {
                size_t fileSize = file.size();  // 获取文件大小
                String fileContent;

                // 逐字节读取文件内容
                while (file.available()) {
                    fileContent += (char)file.read();
                }
                file.close();  // 关闭文件

                // 返回HTML内容
                request->send(200, "text/html", fileContent);
                return;
            }
        }
        // 如果文件不存在，返回404错误
        request->send(404, "text/plain", "File Not Found"); });
    server.on("/control.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        // 检查SPIFFS文件系统中是否存在index.html文件
        if (SPIFFS.exists("/control.html")) {
            fs::File file = SPIFFS.open("/control.html", "r");  // 打开index.html文件
            if (file) {
                size_t fileSize = file.size();  // 获取文件大小
                String fileContent;

                // 逐字节读取文件内容
                while (file.available()) {
                    fileContent += (char)file.read();
                }
                file.close();  // 关闭文件

                // 返回HTML内容
                request->send(200, "text/html", fileContent);
                return;
            }
        }
        // 如果文件不存在，返回404错误
        request->send(404, "text/plain", "File Not Found"); });
    server.on("/engine.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        // 检查SPIFFS文件系统中是否存在index.html文件
        if (SPIFFS.exists("/engine.html")) {
            fs::File file = SPIFFS.open("/engine.html", "r");  // 打开index.html文件
            if (file) {
                size_t fileSize = file.size();  // 获取文件大小
                String fileContent;

                // 逐字节读取文件内容
                while (file.available()) {
                    fileContent += (char)file.read();
                }
                file.close();  // 关闭文件

                // 返回HTML内容
                request->send(200, "text/html", fileContent);//发送文件内容
                return;
            }
        }
        // 如果文件不存在，返回404错误
        request->send(404, "text/plain", "File Not Found"); });
    server.on("/setting.html", HTTP_GET, [](AsyncWebServerRequest *request)
              {
        // 检查SPIFFS文件系统中是否存在index.html文件
        if (SPIFFS.exists("/setting.html")) {
            fs::File file = SPIFFS.open("/setting.html", "r");  // 打开index.html文件
            if (file) {
                size_t fileSize = file.size();  // 获取文件大小
                String fileContent;

                // 逐字节读取文件内容
                while (file.available()) {
                    fileContent += (char)file.read();
                }
                file.close();  // 关闭文件

                // 返回HTML内容
                request->send(200, "text/html", fileContent);
                return;
            }
        }
        // 如果文件不存在，返回404错误
        request->send(404, "text/plain", "File Not Found"); });
    // 启动服务器
    server.begin();
};
void loadWiFiConfig()
{
    // 初始化SPIFFS文件系统（存储WiFi配置等信息）
if (SPIFFS.begin()) // 成功挂载文件系统
{
    // 尝试打开存储WiFi配置的JSON文件（需提前创建）
    fs::File file = SPIFFS.open(ssidFile, "r"); // "r"表示只读模式
    if (file) // 文件存在且可访问
    {
        // 创建动态JSON文档（容量需根据实际配置数据调整）
        DynamicJsonDocument doc(1024); // 建议至少1024字节存储配置参数
        
        // 反序列化JSON数据（将文件内容解析为JSON对象）
        DeserializationError error = deserializeJson(doc, file);
        
        if (!error) // JSON解析成功
        {
            // 从JSON对象中提取配置参数
            String ssid = doc["ssid"];     // WiFi名称字段
            String pass = doc["pass"];     // WiFi密码字段
            String city = doc["city"];     // 城市代码字段
            String api = doc["api"];       // 天气API密钥字段
            
            // 将配置参数赋给全局变量
            cityname = city;        // 存储城市代码
            weatherapi = api;       // 存储API密钥
            
            // 使用存储的凭证尝试连接WiFi
            WiFi.begin(ssid.c_str(), pass.c_str()); // 转换为C风格字符串
            
            // 设置5秒连接超时（5000ms）
            unsigned long startAttemptTime = millis();
            while (WiFi.status() != WL_CONNECTED && 
                  millis() - startAttemptTime < 5000)
            {
                delay(500); // 等待连接，每0.5秒检测一次
            }

            // 连接状态检测
            if (WiFi.status() != WL_CONNECTED) 
            {
                Serial.println("WiFi connection failed, starting captive portal...");
                handleWiFiConfig(); // 启动强制配置门户（如AP模式）
            }
            else 
            {
                Serial.println("WiFi connected");
                timeClient.begin(); // 初始化NTP时间客户端
            }
        }
        file.close(); // 关闭文件释放资源
    }
}

}
void fetchWeather()
{ // 天气捕捉
    // 天气数据初始化模块（首次运行或需要更新时触发）
if (initweather == false) 
{
    // 检测WiFi连接状态（确保网络可用性）
    if (WiFi.status() == WL_CONNECTED) 
    {
        WiFiClient client;      // 创建TCP客户端
        HTTPClient http;        // 初始化HTTP客户端
        
        // 构建带参数的API请求URL（包含动态参数）
        String apiUrl = weatherAPI + weatherapi + "&location=" + cityname + "&language=zh-Hans&unit=c&start=0&days=1";
        
        // 发起HTTPS连接（注意：实际需确认weatherAPI是否支持SSL）
        if (http.begin(client, apiUrl)) 
        {
            int httpCode = http.GET();  // 发送GET请求
            
            // 成功接收响应（httpCode 200表示成功）
            if (httpCode > 0) 
            {
                String payload = http.getString();  // 获取完整响应数据
                
                // 调试输出原始JSON数据（建议在开发阶段开启）
                Serial.println("JSON Response:");
                Serial.println(payload);
                
                // 创建JSON文档并解析数据
                DynamicJsonDocument doc(2048);  // 建议扩大至2048字节防止数据截断
                DeserializationError error = deserializeJson(doc, payload);
                
                if (!error) 
                {
                    // 提取天气数据（注意字段路径需与API响应结构匹配）
                    String temperature2 = doc["results"][0]["daily"][0]["high"];     // 最高温度
                    String humidity2 = doc["results"][0]["daily"][0]["humidity"];     // 湿度值
                    String weathe2r = doc["results"][0]["daily"][0]["text_day"];     // 天气描述（变量名疑似拼写错误）
                    
                    // 更新全局天气变量
                    temperature = temperature2;
                    humidity = humidity2;
                    weather = weathe2r;
                    initweather = true;  // 标记已完成初始化
                    
                    // 调试输出解析结果
                    Serial.print("Data received: ");
                    Serial.println(temperature);
                    Serial.println(humidity);
                    Serial.println(weather);
                }
                else 
                {
                    Serial.println("JSON解析失败: " + String(error.c_str()));
                }
            }
            else 
            {
                Serial.printf("HTTP请求失败，错误代码: %d，详情: %s\n", 
                            httpCode, http.errorToString(httpCode).c_str());
            }
            http.end();  // 必须释放资源
        }
        else 
        {
            Serial.println("服务器连接失败，请检查API地址");
        }
    }
}
    if (weather == "小雨" || weather == "大雨" || weather == "暴雨" || weather == "雨")//识别天气
    {
        do
        {
            u8g2.setFont(u8g2_font_ncenB08_tr);//配置字体
            u8g2.drawXBMP(0, 0, 64, 64, rain);//展示图片
            u8g2.drawStr(64, 20, "Temp");//显示温度
            String temperatureString = String(temperature) + " C";//拼接字符串
            u8g2.drawStr(64, 30, temperatureString.c_str());//屏幕显示
            u8g2.drawStr(64, 50, "Humidity");//内容同上不再注释
            String humidityString = String(humidity) + " %";
            u8g2.drawStr(64, 60, humidityString.c_str());
        } while (u8g2.nextPage());
    }
    else if (weather == "晴")
    {
        do
        {
            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.drawStr(64, 20, "Temp");
            u8g2.drawXBMP(0, 0, 64, 64, sun);
            String temperatureString = String(temperature) + " %";
            u8g2.drawStr(64, 30, temperatureString.c_str());
            u8g2.drawStr(64, 50, "Humidity");
            String humidityString = String(humidity) + " %";
            u8g2.drawStr(64, 60, humidityString.c_str());
        } while (u8g2.nextPage());
    }
    else 
    {
        do
        {
            u8g2.setFont(u8g2_font_ncenB08_tr);
            u8g2.drawXBMP(0, 0, 64, 64, cloud);
            u8g2.drawStr(64, 20, "Temp");
            String temperatureString = String(temperature) + " C";
            u8g2.drawStr(64, 30, temperatureString.c_str());
            u8g2.drawStr(64, 50, "Humidity");
            String humidityString = String(humidity) + " %";
            u8g2.drawStr(64, 60, humidityString.c_str());
        } while (u8g2.nextPage());
    }
}

void front()
{
    servo2.write(140); //舵机2旋转至140度
    servo3.write(40);  //舵机旋转至40度
    delay(100);//延时100s
    servo1.write(40);  //内容同上
    servo4.write(140); 
    delay(100);
    servo2.write(90); 
    servo3.write(90); 
    delay(100);
    servo1.write(90); 
    servo4.write(90); 
    delay(100);
    servo1.write(140); 
    servo4.write(40);  
    delay(100);
    servo2.write(40);  
    servo3.write(140); 
    delay(100);
    servo1.write(90); 
    servo4.write(90); 
    delay(100);
    servo2.write(90); 
    servo3.write(90); 

}
void back()
{
    servo3.write(140); 
    servo2.write(40);  
    delay(100);
    servo4.write(40);  
    servo1.write(140); 
    delay(100);
    servo3.write(90); 
    servo2.write(90); 
    delay(100);
    servo4.write(90); 
    servo1.write(90); 
    delay(100);
    servo4.write(140); 
    servo1.write(40);  
    delay(100);
    servo3.write(40);  
    servo2.write(140); 
    delay(100);
    servo4.write(90); 
    servo1.write(90); 
    delay(100);
    servo3.write(90); 
    servo2.write(90); 

}
void right()
{
    int num = 0;
    while (num < 3)//调用一次执行3次
    {
        servo1.write(100); 
        servo4.write(100); 
        delay(100);
        servo3.write(60); 
        servo2.write(60); 
        delay(100);
        servo1.write(140); 
        servo4.write(140); 
        delay(100);
        servo3.write(40); 
        servo2.write(40); 
        delay(100);
        servo3.write(90); 
        servo2.write(90); 
        servo1.write(90); 
        servo4.write(90); 
        delay(100);
        servo1.write(80); 
        servo4.write(80); 
        delay(100);
        servo3.write(120); 
        servo2.write(120); 
        delay(100);
        servo1.write(90); 
        servo4.write(90); 
        delay(100);
        servo3.write(140); 
        servo2.write(140); 
        delay(100);
        servo3.write(90); 
        servo2.write(90); 

        num++;
    }
   
}
void left()
{

    int num = 0;
    while (num < 3)
    {
        servo1.write(80); 
        servo4.write(80); 
        delay(100);
        servo3.write(120); 
        servo2.write(120); 
        delay(100);
        servo1.write(40); 
        servo4.write(40); 
        delay(100);
        servo3.write(140); 
        servo2.write(140); 
        delay(100);
        servo3.write(90); 
        servo2.write(90); 
        servo1.write(90); 
        servo4.write(90); 
        delay(100);
        servo1.write(100); 
        servo4.write(100); 
        delay(100);
        servo3.write(60); 
        servo2.write(60); 
        delay(100);
        servo1.write(90); 
        servo4.write(90); 
        delay(100);
        servo3.write(40); 
        servo2.write(40); 
        delay(100);
        servo3.write(90); 
        servo2.write(90); 

        num++;
    }
}

void sitdown()
{
    servo2.write(140); 
    servo4.write(40);  
    delay(3000);
    servo2.write(90); 
    servo4.write(90); 

}
void lie()
{
    servo1.write(180); 
    servo3.write(0);   
    servo2.write(0);   
    servo4.write(180); 
    delay(3000);
    servo1.write(90); 
    servo3.write(90); 
    servo2.write(90); 
    servo4.write(90); 

}

void toplefthand()
{
    int num = 0;
    while (num < 3)
    {
        servo3.write(0); 
        delay(100);
        servo3.write(30);
        delay(100);
   
        num++;
    }
    servo3.write(90);
}
void toprighthand()
{

    int num = 0;
    while (num < 3)
    {
        servo1.write(180); 
        delay(100);
        servo1.write(150);
        delay(100);
    
        num++;
    }
    servo1.write(90);
}
void dosleep()
{
    servo1.write(0);                   
    servo3.write(180);                 
    servo2.write(180);                 
    servo4.write(0); 
}

// 对 ADC 数据多次采样并计算平均值
float getAverageAdcVoltage()
{
    long totalAdcValue = 0;

    // 多次采样
    for (int i = 0; i < numSamples; i++)
    {
        totalAdcValue += analogRead(A0); // 读取 ADC 数据
        delay(10);                       // 每次采样间隔 10ms
    }

    // 计算平均 ADC 值
    float averageAdcValue = totalAdcValue / (float)numSamples;

    // 将 ADC 值转换为电压
    return (averageAdcValue / 1023.0) * 1.0; // ESP8266 的参考电压为 1.0V
}

// 计算电池电量百分比的函数
int mapBatteryPercentage(float voltage)
{
    if (voltage <= minVoltage)
        return 0; // 小于等于最小电压时，电量为 0%
    if (voltage >= maxVoltage)
        return 100; // 大于等于最大电压时，电量为 100%

    // 根据线性比例计算电量百分比
    return (int)((voltage - minVoltage) / (maxVoltage - minVoltage) * 100);
}
void serialListen(){

    // 读取完整字符串（直到换行符）
    String receivedString = Serial.readStringUntil('\n');
    // 去掉可能的回车符或空格
    receivedString.trim();
    // 处理接收到的字符串
    Serial.print("Received: ");
    Serial.println(receivedString);
    if(receivedString=="front"){//当接收到“front”时
        front();//执行前进
    };
    if(receivedString=="back"){
        back();
    };
    if(receivedString=="toplefthand"){
        toplefthand();
    };
    if(receivedString=="toprighthand"){
        toprighthand();
    };
    if(receivedString=="left"){
        left();
    };
    if(receivedString=="right"){
        right();
    };
    if(receivedString=="sitdown"){
        sitdown();
    };
    if(receivedString=="lie"){
        lie();
    };
    if(receivedString=="dosleep"){
        dosleep();
    };
    if(receivedString=="kaixin"){
        emojiState=0;
    };
    if(receivedString=="shengqi"){
        emojiState=1;
    };
    if(receivedString=="nanshou"){
        emojiState=5;
    };
    if(receivedString=="haoqi"){
        emojiState=3;
    };
    if(receivedString=="xihuan"){
        emojiState=4;
    };
    if(receivedString=="cuowu"){
        emojiState=2;
    };
    if(receivedString=="yun"){
        emojiState=6;
    };
    if(receivedString=="shijian"){
        emojiState=8;
    };
    if(receivedString=="tianqi"){
        emojiState=7;
    };
    if(receivedString=="logo" || receivedString=="brand"){
        emojiState=9;
    };
    if(receivedString=="dream"){
        freestate=false;
        actionstate=5;
        emojiState=10;
    };
          

}
void setup()
{
u8g2.begin();
u8g2.setDisplayRotation(U8G2_R2);
// OLED 显示初始化与按钮中断配置
u8g2.firstPage();  // 启动U8g2页面缓冲绘制
do {
    // 开机展示中文品牌和定制者姓名（128x64 XBM 位图）。
    u8g2.drawXBMP(0, 0, 128, 64, dreamBrand);
} while (u8g2.nextPage());  // 循环刷新直至完成全帧绘制

// 按钮1配置（通常对应GPIO2）
// 硬件要求：按钮接地触发，内置上拉保持高电平
pinMode(BUTTON_PIN, INPUT_PULLUP); 
// 配置下降沿中断（物理按下时产生低电平）
attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);

// 按钮2配置（通常对应GPIO15）
// 硬件要求：需外接上拉电阻，按钮接3.3V触发
pinMode(BUTTON_PIN2, INPUT);  // 无内部上拉模式
// 配置上升沿中断（物理按下时产生高电平）
attachInterrupt(digitalPinToInterrupt(BUTTON_PIN2), handleButtonPress, RISING);
//启用SPIFFS文件系统
    SPIFFS.begin();
    servo1.attach(engine1, 500, 2500); // 配置舵机PWM，500µs=0度，2500µs=180度
    servo2.attach(engine2, 500, 2500); 
    servo3.attach(engine3, 500, 2500); 
    servo4.attach(engine4, 500, 2500); 
    servo1.write(90);//舵机旋转到90度       
    servo3.write(90);                 
    servo2.write(90);                 
    servo4.write(90);                   
    // 初始化串口
    Serial.begin(115200);
    // 设置WiFi为热点模式
    WiFi.softAP(ssid, password);
    Serial.println("热点已启动");
    // 访问的IP地址是 ESP8266 的默认IP：192.168.4.1
    Serial.print("访问地址: ");
    Serial.print(WiFi.softAPIP());
    // 加载WiFi配置
    loadWiFiConfig();
    if (WiFi.status() != WL_CONNECTED)//当WIFI未连接时
    {
        Serial.println("Starting captive portal...");//串口输出
        handleWiFiConfig();//加载WIFI配置
    }
    else
    {
        handleWiFiConfig();//加载WIFI配置
        Serial.println("WiFi connected");
        timeClient.begin();//NTP服务初始化
        timeClient.update(); // 获取初始时间
    }
    delay(5000);
    u8g2.clearDisplay();//清屏
    do
    {
        u8g2.setFont(u8g2_font_ncenB08_tr);
        u8g2.drawXBMP(0, 0, 64, 64, ipip);
        u8g2.drawStr(64, 10, "WIFI_AP");
        u8g2.drawStr(64, 27, "DreamDog");
        u8g2.drawStr(64, 43, "192.168.4.1");
        u8g2.drawStr(64, 60, "WIFI CTRL");
    } while (u8g2.nextPage());
    delay(5000);
    u8g2.clearDisplay();
}

void loop()
{
    if (Serial.available() > 0) {
        serialListen();
    }
    // 对 ADC 数据多次采样并求平均
    float adcVoltage = getAverageAdcVoltage();

    // 将采样的 ADC 电压转换为实际电池电压
    batteryVoltage = adcVoltage * voltageDividerRatio; // 计算电池电压

    // 根据电池电压计算电量百分比
    batteryPercentage = mapBatteryPercentage(batteryVoltage);

    if (buttonPressed)//按键1按下时
    {
        buttonPressed = false; // 清除按键标志
        front();
    }
    if (buttonPressed2)
    {
        buttonPressed2 = false; // 清除按键标志
        back();
    }
    if (emojiState != prevEmojiState)
    {
        u8g2.clearDisplay();         // 状态变化时清屏
        prevEmojiState = emojiState; // 更新状态
    }
    if (freestate)
    {
        delay(3000);
        actionstate = random(0, 10);
    }
    // 可以使用switch优化效率
    switch (actionstate)
    {
    case 0 /* constant-expression */:
        /* code */
        break;
    case 1:
        front(); // 执行一次舵机动作
        actionstate = 0;
        break;
    case 2:
        left(); // 执行一次舵机动作
        actionstate = 0;
        break;
    case 3:
        right(); // 执行一次舵机动作
        actionstate = 0;
        break;
    case 4:
        back(); // 执行一次舵机动作
        actionstate = 0;
        break;
    case 5:
        toplefthand(); // 执行一次舵机动作
        actionstate = 0;
        break;
    case 6:
        toprighthand(); // 执行一次舵机动作
        actionstate = 0;
        break;
   
    case 10:
        dosleep(); // 执行一次舵机动作
        actionstate = 0;
        break;
    case 7:
        lie(); // 执行一次舵机动作
        actionstate = 0;
        break;
    case 8:
        sitdown(); // 执行一次舵机动作
        actionstate = 0;
        break;
    case 9:
        emojiState = random(0, 7); // 执行一次舵机动作
        actionstate = 0;
        break;
    default:
        break;
    }

    switch (emojiState)
    {
    case 0: // 首页
        u8g2.setFont(u8g2_font_ncenB14_tr);
        do
        {

            u8g2.drawXBMP(0, 0, 128, 64, hi);
        } while (u8g2.nextPage());

        break;
    case 1: // 第二页

        u8g2.setFont(u8g2_font_ncenB14_tr);
        do
        {

            u8g2.drawXBMP(0, 0, 128, 64, angry);
        } while (u8g2.nextPage());

        break;
    case 2: // 第三页
        do
        {
            u8g2.setFont(u8g2_font_ncenB14_tr);
            u8g2.drawXBMP(0, 0, 128, 64, error);
        } while (u8g2.nextPage());

        break;
    case 3: // 第四页
        do
        {
            u8g2.setFont(u8g2_font_ncenB14_tr);
            u8g2.drawXBMP(0, 0, 128, 64, dowhat);
        } while (u8g2.nextPage());

        break;
    case 4: // 第四页

        do
        {
            u8g2.setFont(u8g2_font_ncenB14_tr);
            u8g2.drawXBMP(0, 0, 128, 64, love);
        } while (u8g2.nextPage());
        break;
    case 5: 

        do
        {
            u8g2.setFont(u8g2_font_ncenB14_tr);
            u8g2.drawXBMP(0, 0, 128, 64, sick);
        } while (u8g2.nextPage());
        break;
    case 6: 
        do
        {
            u8g2.setFont(u8g2_font_ncenB14_tr);
            u8g2.drawXBMP(0, 0, 128, 64, yun);
        } while (u8g2.nextPage());

        break;
    case 7: 
        if (WiFi.status() != WL_CONNECTED)
        {
            do
            {
                u8g2.setFont(u8g2_font_ncenB08_tr);
                u8g2.drawXBMP(0, 0, 64, 64, wifi);
                u8g2.drawStr(64, 20, "IP:");
                u8g2.drawStr(64, 40, "192.168.4.1");
                u8g2.drawStr(64, 60, "Need NET");
            } while (u8g2.nextPage());
        }
        else
        {
            fetchWeather();
        }
        break;

        break;
    case 8:
        if (WiFi.status() != WL_CONNECTED)
        {
            do
            {
                u8g2.setFont(u8g2_font_ncenB08_tr);
                u8g2.drawXBMP(0, 0, 64, 64, wifi);
                u8g2.drawStr(64, 20, "IP:");
                u8g2.drawStr(64, 40, "192.168.4.1");
                u8g2.drawStr(64, 60, "Need NET");
            } while (u8g2.nextPage());
        }
        else
        {
            do
            {
                timeClient.update(); // 更新时间
                u8g2.setFont(u8g2_font_ncenB14_tr);
                timeClient.update();
                u8g2.drawXBMP(0, 0, 64, 64, timeimage);
                // 获取当前时间
                // 显示时间到 OLED
                int currentHour = timeClient.getHours();
                int currentMinute = timeClient.getMinutes();
                String timeToDisplay = String(currentHour) + ":" + String(currentMinute);
                u8g2.drawStr(64, 30, "TIME");
                u8g2.setCursor(64, 50);
                u8g2.print(timeToDisplay);

            } while (u8g2.nextPage());
        }
        break;
    case 9: // 梦工厂品牌画面
        do
        {
            u8g2.drawXBMP(0, 0, 128, 64, dreamBrand);
        } while (u8g2.nextPage());
        break;
    case 10: // 招手互动后的问候画面
        do
        {
            u8g2.drawXBMP(0, 0, 128, 64, dreamHello);
        } while (u8g2.nextPage());
        break;
    default:
        // 添加默认 case 来处理其他情况
        break;
    }

 
}
