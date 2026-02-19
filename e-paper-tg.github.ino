#define ENABLE_GxEPD2_GFX 0  // Важно для совместимости с U8g2

// Версия скетча - обновляйте при каждом изменении
#define SKETCH_VERSION "1.8"

#include <ESP8266WiFi.h>
#include <FastBot2.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <time.h>
#include <EEPROM.h>
#include <LittleFS.h>

// ============ НАСТРОЙКИ ============
// Публичная версия для GitHub (без секретов)
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* botToken = "YOUR_TELEGRAM_BOT_TOKEN";

// ID пользователя, от которого принимаются текстовые сообщения
// Оставьте 0 чтобы принимать от всех, или укажите ваш Telegram user ID
const int64_t ALLOWED_USER_ID = 0;  // 0 = принимать от всех

// Username бота-обработчика изображений (Python бот)
// Файлы от этого бота будут автоматически обрабатываться
const char* IMAGE_PROCESSOR_BOT_USERNAME = "";  // Например: "your_image_bot"

// Настройка часового пояса
// Укажите смещение от UTC в часах (например, 3 для Москвы UTC+3, 2 для Киева UTC+2, -5 для Нью-Йорка UTC-5)
const int TIMEZONE_OFFSET = 3;  // Москва UTC+3

// NTP серверы
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = TIMEZONE_OFFSET * 3600;
const int daylightOffset_sec = 0;  // Летнее время (0 = отключено)

// Интервал обновления времени (1 минута в миллисекундах)
const unsigned long TIME_UPDATE_INTERVAL = 60 * 1000;  // 1 минута

// Размер EEPROM для сохранения данных
// Структура:
// 4 байта (magic), 1 байт (type), 3 байта padding,
// 4 байта (timestamp), 2 байта (img width), 2 байта (img height),
// 64 байта (from_name), 512 байт (text)
#define EEPROM_SIZE 1024
#define EEPROM_MAGIC 0xABCD1234
#define EEPROM_MAGIC_ADDR 0
#define EEPROM_TYPE_ADDR 4
#define EEPROM_TIMESTAMP_ADDR 8
#define EEPROM_IMG_WIDTH_ADDR 12
#define EEPROM_IMG_HEIGHT_ADDR 14
#define EEPROM_FROMNAME_ADDR 16
#define EEPROM_TEXT_ADDR 80
#define MAX_TEXT_LENGTH 512
#define MAX_FROMNAME_LENGTH 64
// RAW формат изображения: "RAW|W|H|<binary>"
const char* LAST_IMAGE_PATH = "/last_image.raw";
const char* LAST_TEXT_PATH = "/last_text.txt";
const uint8_t CONTENT_TEXT = 1;
const uint8_t CONTENT_IMAGE = 2;

// Пины для e-ink дисплея (WeAct Studio 4.2")
// Для NodeMCU v3 (ESP8266) - избегаем GPIO0 и GPIO2 (пины загрузки)
#define EPD_CS     15  // D8 = GPIO15 - Chip Select (должен быть подтянут к GND)
#define EPD_DC     4   // D2 = GPIO4 - Data/Command (безопасный пин)
#define EPD_RST    5   // D1 = GPIO5 - Reset (безопасный пин)
#define EPD_BUSY   12  // D6 = GPIO12 - Busy (безопасный пин)
#define EPD_MOSI   13  // D7 = GPIO13 - SPI MOSI
#define EPD_SCK    14  // D5 = GPIO14 - SPI Clock

// Размеры экрана
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 300

// ============ НАСТРОЙКИ РАЗМЕРА ШРИФТА ============
// Измените эти значения для изменения размера шрифта:
// 0 = маленький, 1 = средний, 2 = большой, 3 = очень большой
#define FONT_SIZE_MODE 2

#if FONT_SIZE_MODE == 0
  #define CYRILLIC_FONT u8g2_font_haxrcorp4089_t_cyrillic
  #define LINE_SPACING 12
#elif FONT_SIZE_MODE == 1
  #define CYRILLIC_FONT u8g2_font_8x13_t_cyrillic
  #define LINE_SPACING 20
#elif FONT_SIZE_MODE == 2
  #define CYRILLIC_FONT u8g2_font_9x15_t_cyrillic
  #define LINE_SPACING 17
#else
  #define CYRILLIC_FONT u8g2_font_10x20_t_cyrillic
  #define LINE_SPACING 23
#endif

/*
  ПУБЛИЧНАЯ ВЕРСИЯ (REDACTED):
  - Этот файл предназначен для GitHub без секретов.
  - Полный рабочий код храните локально в `e-paper-tg.ino` (он добавлен в .gitignore).
  - Чтобы собрать проект, используйте ваш локальный `e-paper-tg.ino` с реальными данными.
*/

void setup() {
  Serial.begin(115200);
  Serial.println("This is a public redacted template.");
}

void loop() {
  delay(1000);
}
