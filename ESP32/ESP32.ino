#include <WiFi.h>
#include <time.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <string.h>

const char *ssid = "PandoraBox";
const char *password = "woshiyuxin";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 28800;
const int daylightOffset_sec = 0;

// API server
const char *apiServer = "api.seniverse.com";

struct weather
{
  char city[20];
  char status[10];
  char temperature[10];
} wt;

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/22, /* data=*/21);

void display()
{

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    // configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

  String currentTime = "";
  if (timeinfo.tm_hour < 10)
    currentTime += 0;
  currentTime += timeinfo.tm_hour;
  currentTime += ":";
  if (timeinfo.tm_min < 10)
    currentTime += 0;
  currentTime += timeinfo.tm_min;
  currentTime += ":";
  if (timeinfo.tm_sec < 10)
    currentTime += 0;
  currentTime += timeinfo.tm_sec;

  String currentDay = "";
  currentDay += (timeinfo.tm_year + 1900);
  currentDay += "/";
  if (timeinfo.tm_mon < 10)
    currentDay += 0;
  currentDay += (timeinfo.tm_mon + 1);
  currentDay += "/";
  if (timeinfo.tm_mday < 10)
    currentDay += 0;
  currentDay += timeinfo.tm_mday;

  u8g2.firstPage();
  do
  {
    // u8g2.setFont(u8g2_font_9x18_mf);
    u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    // u8g2.drawStr(0, 15, currentDay.c_str());
    // u8g2.drawStr(96, 15, wt.city);
    u8g2.setCursor(0, 15);
    u8g2.print(currentDay.c_str());
    // u8g2.print("|");
    u8g2.setCursor(96, 15);
    u8g2.print(wt.city);

    u8g2.setFont(u8g2_font_logisoso28_tr);
    u8g2.drawStr(0, 46, currentTime.c_str());

    u8g2.setFont(u8g2_font_wqy16_t_gb2312);
    u8g2.setCursor(0, 62);
    u8g2.print(wt.temperature);
    u8g2.drawCircle(20, 52, 2, U8G2_DRAW_ALL);
    u8g2.setCursor(24, 62);
    u8g2.print("C|");
    u8g2.print(wt.status);
    u8g2.setCursor(80, 62);
    u8g2.print("星期");
    switch (timeinfo.tm_wday)
    {
    case 0:
      u8g2.print("日");
      break;
    case 1:
      u8g2.print("一");
      break;
    case 2:
      u8g2.print("二");
      break;
    case 3:
      u8g2.print("三");
      break;
    case 4:
      u8g2.print("四");
      break;
    case 5:
      u8g2.print("五");
      break;
    case 6:
      u8g2.print("六");
      break;
    }
  } while (u8g2.nextPage());
}

void parseWeatherData(String content)
{
  //利用arduinoJson库解析心知返回的json天气数据
  //可以利用 https://arduinojson.org/v5/assistant/ Arduinojson助手生成相关json解析代码  很方便！！！
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 210;
  DynamicJsonBuffer jsonBuffer(capacity);

  // const char* json = "{\"results\":[{\"location\":{\"id\":\"WM6N2PM3WY2K\",\"name\":\"成都\",\"country\":\"CN\",\"path\":\"成都,成都,四川,中国\",\"timezone\":\"Asia/Shanghai\",\"timezone_offset\":\"+08:00\"},\"now\":{\"text\":\"多云\",\"code\":\"4\",\"temperature\":\"25\"},\"last_update\":\"2020-05-28T12:50:00+08:00\"}]}";

  JsonObject &root = jsonBuffer.parseObject(content);

  JsonObject &results_0 = root["results"][0];

  JsonObject &results_0_location = results_0["location"];
  const char *results_0_location_id = results_0_location["id"];                           // "WM6N2PM3WY2K"
  const char *results_0_location_name = results_0_location["name"];                       // "成都"
  const char *results_0_location_country = results_0_location["country"];                 // "CN"
  const char *results_0_location_path = results_0_location["path"];                       // "成都,成都,四川,中国"
  const char *results_0_location_timezone = results_0_location["timezone"];               // "Asia/Shanghai"
  const char *results_0_location_timezone_offset = results_0_location["timezone_offset"]; // "+08:00"

  JsonObject &results_0_now = results_0["now"];
  const char *results_0_now_text = results_0_now["text"];               // "多云"
  const char *results_0_now_code = results_0_now["code"];               // "4"
  const char *results_0_now_temperature = results_0_now["temperature"]; // "25"

  const char *results_0_last_update = results_0["last_update"]; // "2020-05-28T12:50:00+08:00"

  Serial.println(results_0_location_name); //通过串口打印出需要的信息
  Serial.println(results_0_now_text);
  Serial.println(results_0_now_code);
  Serial.println(results_0_now_temperature);
  Serial.println(results_0_last_update);
  Serial.print("\r\n");

  strcpy(wt.city, results_0_location_name);
  strcpy(wt.status, results_0_now_text);
  strcpy(wt.temperature, results_0_now_temperature);
}

void getWeather()
{
  //Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED)
  {
    // WiFi client
    HTTPClient client;
    String url = "https://api.seniverse.com/v3/weather/now.json?key=SDRGziTOF68no2Fnp&location=chengdu&language=zh-Hans&unit=c";

    // Your Domain name with URL path or IP address with path
    client.begin(url.c_str());

    // Send HTTP GET request
    int httpResponseCode = client.GET();

    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String content = client.getString();
      // Serial.println(content);
      parseWeatherData(content);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }

    // Free resources
    client.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

void setup()
{
  {
    Serial.begin(115200);
  }

  {
    u8g2.begin();
    u8g2.enableUTF8Print();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_unifont_t_chinese3);
    u8g2.setCursor(0, 14);
    u8g2.print("Waiting for WiFi");
    u8g2.setCursor(0, 30);
    u8g2.print("connection...");
    u8g2.setCursor(0, 47);
    u8g2.print("By:FisherCloud");
    u8g2.sendBuffer();
    delay(500);

    //connect to WiFi
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(200);
      Serial.print(".");
    }
    Serial.println(" CONNECTED");
  }

  {
    //init time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }

  // get weather from seniverse
  {
    getWeather();
  }

  {
    display();
  }
}

static int count = 0;

void loop()
{
  display();
  if (count == 3000)
  {
    getWeather();
    count = 0;
  }
  count += 1;
  delay(100);
  if (WiFi.status() != WL_CONNECTED)
  {
    //connect to WiFi
    Serial.printf("Connecting to %s ", ssid);

    u8g2.firstPage();
    do
    {
      u8g2.setFont(u8g2_font_wqy16_t_gb2312);
      u8g2.setCursor(0, 15);
      u8g2.print("WiFi已断开");
      u8g2.setCursor(0, 31);
      u8g2.print("重连中>>>");
    } while (u8g2.nextPage());

    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }

    Serial.println(" CONNECTED");

    u8g2.firstPage();
    do
    {
      u8g2.setFont(u8g2_font_wqy16_t_gb2312);
      u8g2.setCursor(0, 15);
      u8g2.print("WiFi已断开");
      u8g2.setCursor(0, 31);
      u8g2.print("重连中>>>");
      u8g2.setCursor(0, 47);
      u8g2.print("连接成功");
    } while (u8g2.nextPage());
    delay(500);
  }
}
