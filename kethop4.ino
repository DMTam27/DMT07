#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "time.h"
#include <ESPAsyncWebServer.h>
#include <ESP32Servo.h>



#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// WiFi credentials
const char* ssid = "tam";
const char* password = "27112006";
//MQTT

const char* mqtt_server = "test.mosquitto.org";   
WiFiClient espClient;
PubSubClient client(espClient);
QueueHandle_t mqttQueue;
// Callback khi nhận tin nhắn từ MQTT
void callback(char* topic, byte* payload, unsigned int length) {
    char msg[length + 1];
    memcpy(msg, payload, length);
    msg[length] = '\0';  // Chuyển payload thành chuỗi C

    Serial.print("Nhận tin nhắn từ MQTT: ");
    Serial.println(msg);

    // Đưa tin nhắn vào hàng đợi
    xQueueSend(mqttQueue, &msg, portMAX_DELAY);
}


// Task kết nối MQTT
void mqttTask(void *parameter) {
    while (true) {
        if (!client.connected()) {
            Serial.print("Kết nối MQTT...");
            if (client.connect("ESP32_Client")) {
                Serial.println("Thành công!");
                client.subscribe("iot/led");
            } else {
                Serial.print("Thất bại, mã lỗi: ");
                Serial.println(client.state());
                vTaskDelay(500 / portTICK_PERIOD_MS);
            }
        }
        client.loop();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

// API thời tiết
const char* weatherUrl = "https://api.open-meteo.com/v1/forecast?latitude=21.0285&longitude=105.8542&current=temperature_2m,weathercode";

// Cấu hình NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7 * 3600;  // GMT+7
const int   daylightOffset_sec = 0;

// Dữ liệu thời tiết lưu trữ
struct WeatherData {
    float temperature;
    int weatherCode;
};
QueueHandle_t weatherQueue;
// Chuyển mã thời tiết sang mô tả
String getWeatherDescription(int code) {
    switch (code) {
        case 0: return "Clear sky";
        case 1: return "Few clouds";
        case 2: return "Scattered clouds";
        case 3: return "Overcast";
        case 45: case 48: return "Fog";
        case 51: case 53: case 55: return "Drizzle";
        case 56: case 57: return "Freezing drizzle";
        case 61: case 63: case 65: return "Rain";
        case 66: case 67: return "Freezing rain";
        case 71: case 73: case 75: return "Snow";
        case 77: return "Snow grains";
        case 80: case 81: case 82: return "Rain showers";
        case 85: case 86: return "Snow showers";
        case 95: return "Thunderstorm";
        case 96: case 99: return "Thunderstorm w/ hail";
        default: return "Unknown";
    }
}

// Task cập nhật dữ liệu thời tiết từ API
void TaskWeatherUpdate(void* pvParameters) {
    while (1) {
        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(weatherUrl);
            int httpResponseCode = http.GET();

            if (httpResponseCode > 0) {
                String response = http.getString();
                DynamicJsonDocument doc(1024);
                deserializeJson(doc, response);

                WeatherData data;
                data.temperature = doc["current"]["temperature_2m"];
                data.weatherCode = doc["current"]["weathercode"];

                // Gửi dữ liệu vào Queue
                xQueueOverwrite(weatherQueue, &data);
            }
            http.end();
        }

        vTaskDelay(pdMS_TO_TICKS(10000)); // Cập nhật mỗi 10 giây
    }
}


// Vẽ mắt tròn
void drawRoundEye(int x, int y) {
    display.fillCircle(x, y, 20, SSD1306_WHITE);
    display.fillCircle(x, y, 10, SSD1306_BLACK);
    display.fillCircle(x, y - 5, 4, SSD1306_WHITE);
}
// Vẽ mắt hẹp
void drawNarrowEye(int x, int y) {
    display.fillRoundRect(x - 20, y - 5, 40, 10, 5, SSD1306_WHITE);
    display.fillRoundRect(x - 10, y - 3, 20, 6, 3, SSD1306_BLACK);
}
// Vẽ mắt hình hạt đậu
void drawBeanEye(int x, int y) {
    display.fillRoundRect(x - 20, y - 10, 40, 20, 10, SSD1306_WHITE);
    display.fillCircle(x, y, 8, SSD1306_BLACK);
}
// Vẽ mắt giận dữ
void drawAngryEye(int x, int y) {
    display.fillCircle(x, y, 20, SSD1306_WHITE);
    display.fillTriangle(x - 15, y - 10, x + 15, y - 10, x, y - 20, SSD1306_BLACK);
    display.fillCircle(x, y, 10, SSD1306_BLACK);
}
// Vẽ mắt long lanh đáng yêu
void drawCuteEye(int x, int y) {
    display.fillCircle(x, y, 20, SSD1306_WHITE);
    display.fillCircle(x, y, 10, SSD1306_BLACK);
    display.fillCircle(x - 4, y - 6, 3, SSD1306_WHITE);
    display.fillCircle(x + 4, y - 8, 2, SSD1306_WHITE);
}
// Vẽ mắt ngôi sao
void drawStarEye(int x, int y) {
    display.fillCircle(x, y, 20, SSD1306_WHITE);
    display.fillCircle(x, y, 10, SSD1306_BLACK);
    display.drawPixel(x - 3, y - 5, SSD1306_WHITE);
    display.drawPixel(x + 3, y - 5, SSD1306_WHITE);
    display.drawPixel(x - 2, y - 6, SSD1306_WHITE);
    display.drawPixel(x + 2, y - 6, SSD1306_WHITE);
}
// Vẽ mắt hình tam giác
void drawTriangleEye(int x, int y) {
    display.fillTriangle(x - 15, y + 15, x + 15, y + 15, x, y - 15, SSD1306_WHITE);
    display.fillTriangle(x - 7, y + 7, x + 7, y + 7, x, y - 7, SSD1306_BLACK);
}
// Vẽ mắt lượn sóng
void drawWaveEye(int x, int y) {
    for (int i = -10; i <= 10; i += 4) {
        display.drawLine(x - 15, y + i, x + 15, y + i, SSD1306_WHITE);
    }
}
// Vẽ mắt mèo
void drawCatEye(int x, int y) {
    display.fillRoundRect(x - 18, y - 10, 36, 20, 10, SSD1306_WHITE);
    display.fillRect(x - 2, y - 10, 4, 20, SSD1306_BLACK);
}
// Vẽ mắt robot
void drawRobotEye(int x, int y) {
    display.fillRect(x - 15, y - 10, 30, 20, SSD1306_WHITE);
    display.fillCircle(x, y, 8, SSD1306_BLACK);
    display.drawRect(x - 17, y - 12, 34, 24, SSD1306_BLACK);
}
// Hàm hiển thị mắt với kiểu mắt tùy chọn
void blinkEyes(void (*drawEye)(int, int)) {
    display.clearDisplay();
    drawEye(40, 32);
    drawEye(88, 32);
    display.display();
    vTaskDelay(pdMS_TO_TICKS(1000));

    display.clearDisplay();
    display.fillRect(20, 30, 40, 5, SSD1306_WHITE);
    display.fillRect(80, 30, 40, 5, SSD1306_WHITE);
    display.display();
    vTaskDelay(pdMS_TO_TICKS(1000));
}



// Task hiển thị thời gian và thời tiết lên OLED
void TaskDisplayUpdate(void* pvParameters) {
    struct tm timeinfo;
    WeatherData currentWeather = {0, 0}; // Khởi tạo giá trị mặc định

    while (1) {
        // Lấy thời gian hiện tại
        if (getLocalTime(&timeinfo)) {
            display.clearDisplay();
            display.setTextSize(2);
            display.setTextColor(WHITE);
            
            // Hiển thị ngày/tháng/năm
            display.setCursor(0, 0);
            display.printf("%02d/%02d/%04d", timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);
            
            // Hiển thị giờ/phút
            display.setCursor(20, 30);
            display.setTextSize(3);
            display.printf("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);// chỗ để đặt báo thức 
            
            display.display();
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Hiển thị thời gian trong 1 giây

        // Lấy dữ liệu thời tiết từ Queue (nếu có)
        if (xQueueReceive(weatherQueue, &currentWeather, 0) == pdTRUE) {
            display.clearDisplay();
            display.setTextSize(2);
            display.setCursor(0, 0);
            display.println(getWeatherDescription(currentWeather.weatherCode));

            display.setTextSize(3);
            display.setCursor(0, 35);
            display.println(String(currentWeather.temperature) + " C");


            display.display();
        }

        vTaskDelay(pdMS_TO_TICKS(1000)); // Hiển thị thời tiết trong 2 giây

        if(currentWeather.temperature>=10 && currentWeather.temperature<=15){
        blinkEyes(drawCuteEye);
       }
       else if(currentWeather.temperature<10){
        blinkEyes(drawAngryEye);
       }
       else if(currentWeather.temperature>15&&currentWeather.temperature<=25){
        blinkEyes(drawRobotEye);
       }
       else if(currentWeather.temperature>25&&currentWeather.temperature<35){
         blinkEyes(drawWaveEye);
       }
       else{
         blinkEyes(drawTriangleEye);
       }
       
    }     
}
Servo servo_TP1, servo_TP2;  // Chân trước bên phải
Servo servo_SP1, servo_SP2;  // Chân sau bên phải
Servo servo_TT1, servo_TT2;  // Chân trước bên trái
Servo servo_ST1, servo_ST2;  // Chân sau bên trái
Servo cameraServo;
Servo tailServo;

const int TP1 = 15, TP2 = 5;  // Chân trước phải
const int SP1 = 26, SP2 = 25; // Chân sau phải
const int TT1 = 14, TT2 = 27; // Chân trước trái
const int ST1 = 13, ST2 = 12; // Chân sau trái
#define CAM 33// cam 
#define TAIL 32// đuôi 





// Create Web Server
AsyncWebServer server(80);
QueueHandle_t commandQueue;

// Struct chứa lệnh điều khiển
struct Command {
    String action;
};

// HTML control page
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Robot Dog Control</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; background-color: pink; }
    button { font-size: 20px; margin: 10px; padding: 10px; width: 150px; border: none; border-radius: 10px; background-color: white; cursor: pointer; }
    button:hover { background-color: lightgray; }
  </style>
</head>
<body>
  <h2>Robot Dog Control</h2>
  <button onclick="sendCommand('forward')">Forward</button>
  <button onclick="sendCommand('backward')">Backward</button><br>
  <button onclick="sendCommand('turn_left')">Turn Left</button>
  <button onclick="sendCommand('turn_right')">Turn Right</button><br>
  <button onclick="sendCommand('sit')">Sit</button>
  <button onclick="sendCommand('wave_tail')">Wave Tail</button><br>
  <button onclick="sendCommand('cam_on')">Turn On Camera</button>
  <button onclick="sendCommand('cam_off')">Turn Off Camera</button>
  
  <script>
    function sendCommand(cmd) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", "/command?cmd=" + cmd, true);
      xhr.send();
    }
  </script>
</body>
</html>
)rawliteral";

void setupServos() {
    // set up servo
    servo_TP1.attach(TP1);
    servo_TP2.attach(TP2);
    servo_SP1.attach(SP1);
    servo_SP2.attach(SP2);
    servo_TT1.attach(TT1);
    servo_TT2.attach(TT2);
    servo_ST1.attach(ST1);
    servo_ST2.attach(ST2);
    cameraServo.attach(CAM);
    tailServo.attach(TAIL);


    // Vị trí ban đầu 
    servo_TP1.write(90);
    servo_TP2.write(70);
    servo_SP1.write(90);
    servo_SP2.write(70);
    servo_TT1.write(85);
    servo_TT2.write(110);
    servo_ST1.write(90);
    servo_ST2.write(110);
    cameraServo.write(90);
    tailServo.write(90);

   
}

void handleCommand(String cmd) {
    Serial.println("Command received: " + cmd);
    Command command;
    command.action = cmd;
    xQueueSend(commandQueue, &command, portMAX_DELAY);
}
int temp =0;
void robotTask(void *parameter) {
    Command command;
    while (1) {
        if (xQueueReceive(commandQueue, &command, portMAX_DELAY) == pdTRUE) {
            if (command.action=="forward") {
              // hàm tiến 
 for (temp = 0; temp <=10; temp++){
    servo_TP1.write(90-temp);
    servo_TP2.write(90-temp);
    servo_SP1.write(90+temp);
    servo_SP2.write(90+temp);
    servo_TT1.write(90-temp);
    servo_TT2.write(90-temp);
    servo_ST1.write(90+temp);
    servo_ST2.write(90+temp);
    delay(20);
  }
  for (temp=10; temp >=-10; temp--){
    servo_TP1.write(90-temp);
    servo_TP2.write(90-temp);
    servo_SP1.write(90+temp);
    servo_SP2.write(90+temp);
    servo_TT1.write(90-temp);
    servo_TT2.write(90-temp);
    servo_ST1.write(90+temp);
    servo_ST2.write(90+temp);
    delay(20);
  }
  for (temp=-10; temp<0; temp++){
   servo_TP1.write(90-temp);
    servo_TP2.write(90-temp);
    servo_SP1.write(90+temp);
    servo_SP2.write(90+temp);
    servo_TT1.write(90-temp);
    servo_TT2.write(90-temp);
    servo_ST1.write(90+temp);
    servo_ST2.write(90+temp);
    delay(20);
  }
   
                Serial.println("Moving Forward");
            }
             else if (command.action=="backward") {
              // hàm lùi 
              
  servo_TT2.write(140);
    servo_SP2.write(40);
    
    vTaskDelay(pdMS_TO_TICKS(200));
    servo_TT1.write(115);
    servo_SP1.write(70);
    vTaskDelay(pdMS_TO_TICKS(200));
    servo_SP1.write(90);
    servo_SP2.write(70);
    servo_TT1.write(85);
    servo_TT2.write(110);
    vTaskDelay(pdMS_TO_TICKS(200));
    servo_TP2.write(40);
    servo_ST2.write(140);
    
    vTaskDelay(pdMS_TO_TICKS(200));
    servo_TP1.write(70);
    servo_ST1.write(110);
    vTaskDelay(pdMS_TO_TICKS(200));
    servo_TP1.write(90);
    servo_TP2.write(70);
    servo_ST1.write(90);
    servo_ST2.write(110);
    vTaskDelay(pdMS_TO_TICKS(200));
blinkEyes(drawBeanEye);
              
                Serial.println("Moving Backward");
            } 
            else if (command.action=="turn_left") {
              // hàm rẽ phải 
servo_TP1.write(0);
  servo_TP2.write(180);
  servo_SP1.write(0);
  servo_SP2.write(180);
  servo_TT1.write(180);
  servo_TT2.write(0);
  servo_ST1.write(180);
  servo_ST2.write(0);


                Serial.println("Turning Left");
            } else if (command.action=="turn_right") {
              //hàm rẽ trái 
servo_TP1.write(0);
  servo_TP2.write(90);
  servo_SP1.write(0);
  servo_SP2.write(90);
  servo_TT1.write(180);
  servo_TT2.write(90);
  servo_ST1.write(180);
  servo_ST2.write(90);


                Serial.println("Turning Right");
            } else if (command.action=="sit") {
              //hàm ngồi xuống 
 
    servo_ST2.write(120);
    vTaskDelay(pdMS_TO_TICKS(200));
    servo_ST2.write(180);
    vTaskDelay(pdMS_TO_TICKS(200));
    servo_SP2.write(50);
    vTaskDelay(pdMS_TO_TICKS(200));
    servo_SP2.write(0);
    vTaskDelay(pdMS_TO_TICKS(200));
    servo_TT1.write(90);
    servo_TT2.write(100);
    servo_ST1.write(90);
    servo_TP1.write(90);
    servo_TP2.write(80);
    servo_SP1.write(90);

blinkEyes(drawAngryEye);


                Serial.println("Sitting");
            } else if (command.action=="wave_tail") {
                Serial.println("Waving Tail");
                // hàm vẫy đuôi 
                for (int i = 0; i < 4; i++) {
                    tailServo.write(160);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    tailServo.write(20);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
                tailServo.write(90);
                 blinkEyes(drawCatEye);
            } else if (command.action=="cam_on") {
              // hàm bật cam
                Serial.println("Turning On Camera");
                cameraServo.write(0);
                vTaskDelay(6000 / portTICK_PERIOD_MS);
                cameraServo.write(90);
                blinkEyes(drawRobotEye);
            } else if (command.action=="cam_off") {
              // hàm tắt cam 
                Serial.println("Turning Off Camera");
                cameraServo.write(180);
                vTaskDelay(6000 / portTICK_PERIOD_MS);
                cameraServo.write(90);
                blinkEyes(drawNarrowEye);
            }
        }
    }
}

void webServerTask(void *parameter) {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", index_html);
    });
    
    server.on("/command", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("cmd")) {
            String cmd = request->getParam("cmd")->value();
            handleCommand(cmd);
        }
        request->send(200, "text/plain", "OK");
    });

    server.begin();
    vTaskDelete(NULL);
}

// Task điều khiển LED
void ledTask(void *parameter) {
    char message[10];
    while (true) {
        if (xQueueReceive(mqttQueue, &message, portMAX_DELAY) == pdPASS) {
            if (strcmp(message, "ON") == 0) {
                for (int i = 0; i < 3; i++) {
                    tailServo.write(160);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                    tailServo.write(20);
                    vTaskDelay(500 / portTICK_PERIOD_MS);
                }
                tailServo.write(90);
                 blinkEyes(drawCatEye);
            } else if (strcmp(message, "OFF") == 0) {
                blinkEyes(drawAngryEye);
            }
        }
    }
}
void setup() {
    Serial.begin(115200);
    setupServos();
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
        Serial.print(".");
    }
    Serial.println("\nWiFi Connected!");
    Serial.println(WiFi.localIP());

    // Cấu hình NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    // Khởi động màn hình OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 OLED not found!");
        while (1);
    }
    display.clearDisplay();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
mqttQueue = xQueueCreate(5, sizeof(char[10])); // Hàng đợi lưu tin nhắn MQTT
    
    commandQueue = xQueueCreate(10, sizeof(Command));
    
    
    // Tạo Queue lưu trữ dữ liệu thời tiết
    weatherQueue = xQueueCreate(1, sizeof(WeatherData));

    // Tạo các Task FreeRTOS
    xTaskCreate(TaskWeatherUpdate, "Weather Update", 4096, NULL, 1, NULL);
    xTaskCreate(TaskDisplayUpdate, "Display Update", 4096, NULL, 1, NULL);
    xTaskCreatePinnedToCore(robotTask, "RobotTask", 4096, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(webServerTask, "WebServerTask", 4096, NULL, 1, NULL, 0);
    xTaskCreate(mqttTask, "MQTT Task", 4096, NULL, 1, NULL);
    xTaskCreate(ledTask, "LED Task", 2048, NULL, 1, NULL);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000)); // Không cần dùng loop()
}
