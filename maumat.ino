#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // Định nghĩa chiều rộng màn hình OLED
#define SCREEN_HEIGHT 64   // Định nghĩa chiều cao màn hình OLED
#define OLED_RESET    -1   // Chân reset OLED, -1 nếu không sử dụng
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);




















// Hàm hiển thị  mắt với kiểu mắt tùy chọn
void blinkEyes(void (*drawEye)(int, int)) {
    display.clearDisplay();
    drawEye(40, 32);
    drawEye(88, 32);
    display.display();
    delay(2000);

    display.clearDisplay();
    display.fillRect(20, 30, 40, 5, SSD1306_WHITE);
    display.fillRect(80, 30, 40, 5, SSD1306_WHITE);
    display.display();
    delay(1000);
}

void setup() {
    Serial.begin(115200);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("SSD1306 OLED not found!");
        while (1);
    }
    display.clearDisplay();
}

void loop() {
  blinkEyes(drawStarEye);
}
