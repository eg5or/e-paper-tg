// Тест с библиотекой GxEPD2 (альтернатива GxEPD)
// Попробуем использовать GxEPD2, которая может лучше работать с некоторыми экранами

#include <GxEPD2_BW.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

// Пины для NodeMCU v3 (ESP8266)
#define EPD_CS     15  // D8 = GPIO15 - Chip Select
#define EPD_DC     4   // D2 = GPIO4 - Data/Command
#define EPD_RST    5   // D1 = GPIO5 - Reset
#define EPD_BUSY   12  // D6 = GPIO12 - Busy

// Правильный драйвер для WeAct Studio 4.2" экрана
// GxEPD2_420_GDEY042T81 - для WeAct Studio 4.2" (400x300, SSD1683)
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  Serial.println("START GxEPD2");
  
  // Инициализация экрана с правильными параметрами
  // init(115200, true, 50, false) - для WeAct Studio экрана
  display.init(115200, true, 50, false);
  delay(1000);
  
  Serial.println("INIT DONE");
  
  // Полная очистка экрана при первом запуске (5-30 раз для максимального контраста)
  Serial.println("Full screen clear...");
  for (int i = 0; i < 10; i++) {
    display.fillScreen(GxEPD_BLACK);
    display.display();
    delay(200);
    display.fillScreen(GxEPD_WHITE);
    display.display();
    delay(200);
  }
  Serial.println("Clear done");
  
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  
  // Очистка и рисование
  display.fillScreen(GxEPD_WHITE);
  
  display.setFont(&FreeMonoBold18pt7b);
  display.setCursor(50, 100);
  display.print("TEST");
  
  display.setFont(&FreeMonoBold12pt7b);
  display.setCursor(50, 150);
  display.print("Hello!");
  
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(50, 200);
  display.print("NodeMCU");
  
  display.drawRect(10, 220, 380, 60, GxEPD_BLACK);
  display.setCursor(20, 260);
  display.print("400x300");
  
  Serial.println("UPDATE");
  
  // Обновление экрана - в GxEPD2 используется display()
  display.display();
  
  // Важно! Вызываем hibernate() после каждого обновления для повышения контраста
  display.hibernate();
  
  Serial.println("DONE");
}

void loop() {
  delay(1000);
}

