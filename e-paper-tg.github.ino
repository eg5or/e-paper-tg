#define ENABLE_GxEPD2_GFX 0  // Важно для совместимости с U8g2

// Версия скетча - обновляйте при каждом изменении
#define SKETCH_VERSION "1.9"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include <FastBot2.h>
#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <time.h>
#include <EEPROM.h>
#include <LittleFS.h>


// ============ НАСТРОЙКИ ============
// Публичная версия для GitHub: полностью рабочая, но без секретов
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* botToken = "YOUR_TELEGRAM_BOT_TOKEN";

// ID пользователя, от которого принимаются текстовые сообщения
// Оставьте 0 чтобы принимать от всех, или укажите ваш Telegram user ID
const int64_t ALLOWED_USER_ID = 0;  // 0 = принимать от всех

// Username бота-обработчика изображений (Python бот)
// Файлы от этого бота будут автоматически обрабатываться
const char* IMAGE_PROCESSOR_BOT_USERNAME = "";  // Оставьте пустым или укажите username бота (например: "your_image_bot")

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
const unsigned long WEB_POLL_INTERVAL = 15 * 1000;     // 15 секунд

// Альтернативный канал (web fallback), если Telegram недоступен
// Сервер вернет:
// 204 - команд нет
// 200 + X-Cmd-Type: text/image
// Для text body содержит UTF-8 текст.
// Для image body содержит RAW|W|H|<binary>.
const bool WEB_FALLBACK_ENABLED = true;
const char* WEB_API_BASE_URL = "https://paper.example.com";
const char* WEB_DEVICE_ID = "YOUR_DEVICE_ID";
const char* WEB_DEVICE_API_KEY = "YOUR_DEVICE_API_KEY";

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
#define FONT_SIZE_MODE 2  // Измените это значение для изменения размера шрифта

// Размеры шрифтов в зависимости от режима
// Шрифты U8g2 с кириллицей поддерживают и латиницу
#if FONT_SIZE_MODE == 0
  // Маленький размер
  #define CYRILLIC_FONT u8g2_font_haxrcorp4089_t_cyrillic  // маленький кириллический шрифт
  // Альтернативы: u8g2_font_6x12_t_cyrillic, u8g2_font_5x7_t_cyrillic
  #define LINE_SPACING 12
#elif FONT_SIZE_MODE == 1
  // Средний размер (по умолчанию) - перемещен с режима 2
  #define CYRILLIC_FONT u8g2_font_8x13_t_cyrillic  // 8x13 пикселей - средний (был большой режим 2)
  // Альтернативы: u8g2_font_7x13_t_cyrillic (если 8x13 не работает)
  #define LINE_SPACING 20
#elif FONT_SIZE_MODE == 2
  // Большой размер - новый, больше чем средний
  #define CYRILLIC_FONT u8g2_font_9x15_t_cyrillic  // 9x15 пикселей - большой
  // Альтернативы: u8g2_font_inr42_t_cyrillic (очень большой)
  #define LINE_SPACING 17
#else
  // Очень большой размер (режим 3)
  #define CYRILLIC_FONT u8g2_font_10x20_t_cyrillic  // 10x20 пикселей - очень большой
  // Альтернативы: u8g2_font_inr42_t_cyrillic (если нужен еще больше)
  #define LINE_SPACING 23
#endif

// ============ ИНИЦИАЛИЗАЦИЯ ============
FastBot2 bot;

// Правильный драйвер для WeAct Studio 4.2" экрана
// GxEPD2_420_GDEY042T81 - для WeAct Studio 4.2" (400x300, SSD1683)
GxEPD2_BW<GxEPD2_420_GDEY042T81, GxEPD2_420_GDEY042T81::HEIGHT> display(GxEPD2_420_GDEY042T81(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// U8g2 для поддержки кириллицы
U8G2_FOR_ADAFRUIT_GFX u8g2_for_adafruit_gfx;

// EEPROM для сохранения данных (не нужна отдельная переменная)

// Переменные
String lastMessage = "";
String lastFromName = "";
uint32_t lastTimestamp = 0;
bool lastIsImage = false;
uint16_t lastImageWidth = 0;
uint16_t lastImageHeight = 0;
unsigned long lastTimeUpdate = 0;
unsigned long lastWebPoll = 0;
bool timeSynced = false;

// Forward declarations
bool displayTextMessage(String text, String from_name, uint32_t timestamp, bool showCurrentTime = true, bool forceRedraw = false);
void saveLastMessage(String text, String from_name, uint32_t timestamp);
void saveLastImageMeta(String from_name, uint32_t timestamp, uint16_t width, uint16_t height);
bool loadLastState(String& text, String& from_name, uint32_t& timestamp, bool& isImage, uint16_t& imgW, uint16_t& imgH);
void syncTime();
String getCurrentDateTime();
void updateDisplayTime();
bool processImageMessage(String text);
bool processImageMessageBuffer(const char* data, int len);
bool displayProcessedImageBase64(const char* base64Data, int base64Len, int imgWidth, int imgHeight, String from_name, uint32_t timestamp);
bool processImageStream(fb::Fetcher& fetch, String from_name, uint32_t timestamp);
bool displayImageFromFile(String from_name, uint32_t timestamp, uint16_t width, uint16_t height, bool showCurrentTime);
bool processImageHttpStream(Stream& stream, String from_name, uint32_t timestamp);
void pollWebFallback();
void safeDisplayUpdate();
void drawFooter(bool showCurrentTime);
void updateFooterTimePartial();
bool downloadAndDisplayImage(String url);
void convertToMonochrome(uint8_t* imageData, int width, int height, int bytesPerPixel);

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n=== E-Paper Telegram Bot ===");
  
  // Подключение к WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Подключение к WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi подключен!");
    Serial.print("IP адрес: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nОшибка подключения к WiFi!");
    return;
  }
  
  // Настройка FastBot2
  bot.setToken(botToken);
  bot.onUpdate(handleUpdate);  // Обработчик всех обновлений
  
  // Инициализация e-ink дисплея с правильными параметрами
  Serial.println("Инициализация e-ink дисплея...");
  display.init(115200, true, 50, false);
  Serial.println("Экран инициализирован");
  
  // Инициализация U8g2 для поддержки кириллицы
  u8g2_for_adafruit_gfx.begin(display);
  
  delay(1000);
  
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  
  // Инициализация EEPROM для сохранения данных
  EEPROM.begin(EEPROM_SIZE);

  // Инициализация LittleFS для хранения изображения
  if (!LittleFS.begin()) {
    Serial.println("Ошибка инициализации LittleFS");
  }
  
  // Синхронизация времени через NTP
  syncTime();
  
  // Загружаем последнее состояние из памяти
  String savedText, savedFromName;
  uint32_t savedTimestamp;
  bool savedIsImage = false;
  uint16_t savedW = 0;
  uint16_t savedH = 0;
  bool hasState = loadLastState(savedText, savedFromName, savedTimestamp, savedIsImage, savedW, savedH);
  
  if (hasState) {
    if (savedIsImage) {
      Serial.println("Загружено сохраненное изображение из памяти");
      Serial.print("От: ");
      Serial.println(savedFromName);
      Serial.print("Размер: ");
      Serial.print(savedW);
      Serial.print("x");
      Serial.println(savedH);
      lastIsImage = true;
      lastFromName = savedFromName;
      lastTimestamp = savedTimestamp;
      lastImageWidth = savedW;
      lastImageHeight = savedH;
      displayImageFromFile(savedFromName, savedTimestamp, savedW, savedH, true);
    } else if (savedText.length() > 0) {
      Serial.println("Загружено сохраненное сообщение из памяти");
      Serial.print("От: ");
      Serial.println(savedFromName);
      Serial.print("Текст: ");
      Serial.println(savedText);
      displayTextMessage(savedText, savedFromName, savedTimestamp, true);
    } else {
      Serial.println("Сохраненных сообщений не найдено, экран пустой");
      display.fillScreen(GxEPD_WHITE);
      safeDisplayUpdate();
      display.hibernate();
    }
  } else {
    Serial.println("Сохраненных данных не найдено, экран пустой");
    display.fillScreen(GxEPD_WHITE);
    safeDisplayUpdate();
    display.hibernate();
  }
  
  Serial.println("Бот готов к работе!");
}

void loop() {
  // FastBot2 обрабатывает сообщения через tick()
  bot.tick();
  
  // Обновляем время каждые 5 минут
  unsigned long currentMillis = millis();
  if (timeSynced && (currentMillis - lastTimeUpdate >= TIME_UPDATE_INTERVAL)) {
    syncTime();
    updateDisplayTime();
    lastTimeUpdate = currentMillis;
  }

  if (WEB_FALLBACK_ENABLED && (currentMillis - lastWebPoll >= WEB_POLL_INTERVAL)) {
    pollWebFallback();
    lastWebPoll = currentMillis;
  }
  
  delay(10);
}

void pollWebFallback() {
  if (WiFi.status() != WL_CONNECTED) return;

  String url = String(WEB_API_BASE_URL) +
               "/api/device/pull?device_id=" + WEB_DEVICE_ID +
               "&api_key=" + WEB_DEVICE_API_KEY;

  HTTPClient http;
  const char* headerKeys[] = {"X-Cmd-Type", "X-From", "X-Timestamp", "X-Cmd-Id"};
  http.collectHeaders(headerKeys, 4);
  http.setTimeout(12000);

  bool ok = false;
  if (url.startsWith("https://")) {
    BearSSL::WiFiClientSecure secureClient;
    secureClient.setInsecure();  // Для простоты: без pinning сертификата
    ok = http.begin(secureClient, url);
  } else {
    WiFiClient plainClient;
    ok = http.begin(plainClient, url);
  }

  if (!ok) {
    Serial.println("WEB fallback: не удалось открыть HTTP соединение");
    return;
  }

  int code = http.GET();
  if (code == 204) {
    http.end();
    return;  // Команд нет
  }

  if (code != 200) {
    Serial.print("WEB fallback HTTP error: ");
    Serial.println(code);
    http.end();
    return;
  }

  String cmdType = http.header("X-Cmd-Type");
  String fromName = http.header("X-From");
  if (fromName.length() == 0) fromName = "Web";
  String tsHeader = http.header("X-Timestamp");
  uint32_t timestamp = tsHeader.length() ? (uint32_t)tsHeader.toInt() : (uint32_t)time(nullptr);

  if (cmdType == "text") {
    String text = http.getString();
    http.end();
    if (text.length() == 0) return;
    saveLastMessage(text, fromName, timestamp);
    displayTextMessage(text, fromName, timestamp, true, true);
    Serial.println("WEB fallback: текст отображен");
    return;
  }

  if (cmdType == "image") {
    WiFiClient* stream = http.getStreamPtr();
    if (!stream) {
      Serial.println("WEB fallback: stream not available");
      http.end();
      return;
    }
    bool success = processImageHttpStream(*stream, fromName, timestamp);
    http.end();
    if (success) {
      Serial.println("WEB fallback: изображение отображено");
    } else {
      Serial.println("WEB fallback: ошибка обработки изображения");
    }
    return;
  }

  Serial.print("WEB fallback: неизвестный тип команды: ");
  Serial.println(cmdType);
  http.end();
}

// Функция для форматирования даты и времени из Unix timestamp
String formatDateTime(uint32_t timestamp) {
  // Unix timestamp - количество секунд с 1 января 1970 года
  // Добавляем смещение для локального времени (используется TIMEZONE_OFFSET из настроек)
  uint32_t localTime = timestamp + (TIMEZONE_OFFSET * 3600);
  
  // Вычисляем дату и время
  uint32_t totalSeconds = localTime;
  uint32_t days = totalSeconds / 86400;
  uint32_t seconds = totalSeconds % 86400;
  
  // Более точный расчет даты (учитываем високосные годы приблизительно)
  uint32_t year = 1970;
  uint32_t daysRemaining = days;
  
  // Приблизительный расчет года
  while (daysRemaining >= 365) {
    uint32_t daysInYear = 365;
    // Простая проверка на високосный год (каждые 4 года)
    if ((year % 4) == 0) {
      daysInYear = 366;
    }
    if (daysRemaining >= daysInYear) {
      daysRemaining -= daysInYear;
      year++;
    } else {
      break;
    }
  }
  
  // Приблизительный расчет месяца и дня
  uint32_t monthDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((year % 4) == 0) {
    monthDays[1] = 29;  // Високосный год
  }
  
  uint32_t month = 1;
  uint32_t day = daysRemaining + 1;
  
  for (int i = 0; i < 12; i++) {
    if (day > monthDays[i]) {
      day -= monthDays[i];
      month++;
    } else {
      break;
    }
  }
  
  // Вычисляем время
  uint32_t hour = seconds / 3600;
  uint32_t minute = (seconds % 3600) / 60;
  
  // Форматируем строку: ДД.ММ ЧЧ:ММ
  String dateTime = "";
  if (day < 10) dateTime += "0";
  dateTime += String(day);
  dateTime += ".";
  if (month < 10) dateTime += "0";
  dateTime += String(month);
  dateTime += " ";
  if (hour < 10) dateTime += "0";
  dateTime += String(hour);
  dateTime += ":";
  if (minute < 10) dateTime += "0";
  dateTime += String(minute);
  
  return dateTime;
}

// Callback функция для обработки всех обновлений от Telegram
void handleUpdate(fb::Update& u) {
  // ДИАГНОСТИКА: Логируем ВСЕ обновления
  Serial.println("\n=== DEBUG: Получено обновление ===");
  Serial.print("Тип обновления: ");
  if (u.isMessage()) {
    Serial.println("Message");
  } else {
    Serial.println("Not a Message - пропускаем");
    return;
  }
  
  auto msg = u.message();
  
  // ДИАГНОСТИКА: Проверяем наличие файла и текста ДО обработки
  Serial.print("hasDocument: ");
  Serial.println(msg.hasDocument() ? "ДА" : "НЕТ");
  Serial.print("hasText: ");
  Serial.println(msg.text().length() > 0 ? "ДА" : "НЕТ");
  
  // Получаем информацию об отправителе
  String from_name = "";
  int64_t from_id = 0;
  String from_username = "";
  bool is_bot = false;
  
  auto from = msg.from();
  from_id = from.id();
  is_bot = from.isBot();
  
  // ДИАГНОСТИКА: Важная информация об отправителе
  Serial.print("Отправитель - Is Bot: ");
  Serial.println(is_bot ? "ДА (это бот!)" : "НЕТ (это пользователь)");
  
  if (from.username().length() > 0) {
    from_username = from.username().c_str();
    from_name = from_username;
  } else if (from.firstName().length() > 0) {
    from_name = from.firstName().c_str();
  }
  
  // Получаем chat_id для ответа
  int64_t chatId = msg.chat().id();
  
  // Получаем timestamp сообщения
  uint32_t timestamp = msg.date();
  
  Serial.println("\n=== Новое сообщение ===");
  Serial.print("Chat ID: ");
  Serial.println(chatId);
  Serial.print("От: ");
  Serial.println(from_name);
  Serial.print("User ID: ");
  Serial.println(from_id);
  Serial.print("Is Bot: ");
  Serial.println(is_bot ? "Да" : "Нет");
  
  // Проверяем наличие файла (обработанное изображение от Python бота)
  if (msg.hasDocument()) {
    Serial.println(">>> ОБНАРУЖЕН ФАЙЛ! <<<");
    auto doc = msg.document();
    String fileName = String(doc.name().c_str());
    
    Serial.print("Имя файла: ");
    Serial.println(fileName);
    Serial.print("File ID: ");
    Serial.println(doc.id());
    
    // ВРЕМЕННО: Обрабатываем ВСЕ файлы для диагностики
    Serial.println(">>> ОБРАБОТКА ВСЕХ ФАЙЛОВ (диагностика) <<<");
    
    // Проверяем, что это обработанное изображение (файл с расширением .epd или специальное имя)
    bool isProcessedImage = (fileName.endsWith(".epd") || fileName.startsWith("epaper_img_"));
    
    if (isProcessedImage) {
      Serial.println("Файл соответствует критериям (.epd или epaper_img_)");
    } else {
      Serial.println("Файл НЕ соответствует критериям, но обрабатываем для диагностики");
    }
    
    Serial.println("Начинаем скачивание файла...");
    
    // Скачиваем файл через FastBot2 API
    fb::Fetcher fetch = bot.downloadFile(doc.id());
    
    if (fetch) {
      Serial.println("Файл успешно скачан, читаем поток...");
      bool success = processImageStream(fetch, from_name, timestamp);
      if (success) {
        fb::Message replyMsg;
        replyMsg.chatID = chatId;
        replyMsg.text = "Изображение успешно выведено на экран!";
        bot.sendMessage(replyMsg);
        Serial.println("Ответное сообщение отправлено");
      } else {
        fb::Message errorMsg;
        errorMsg.chatID = chatId;
        errorMsg.text = "Ошибка при обработке изображения";
        bot.sendMessage(errorMsg);
        Serial.println("Ошибка при обработке изображения");
      }
    } else {
      Serial.println("ОШИБКА: Не удалось скачать файл через downloadFile()");
    }
    
    Serial.println(">>> Обработка файла завершена <<<");
    return;  // Файл обработан (или не обработан), выходим
  }
  
  // Проверяем наличие текста
  auto msgText = msg.text();
  bool hasText = (msgText.length() > 0);
  
  if (hasText) {
    // Проверяем, является ли это предобработанным изображением в текстовом формате
    if (msgText.startsWith("IMG|")) {
      Serial.println("Обнаружено предобработанное изображение в тексте");
      bool success = processImageMessage(String(msgText.c_str()));
      
      if (success) {
        fb::Message replyMsg;
        replyMsg.chatID = chatId;
        replyMsg.text = "Изображение успешно выведено на экран!";
        bot.sendMessage(replyMsg);
        Serial.println("Ответное сообщение отправлено");
      }
      return;  // Изображение обработано, выходим
    }
    
    // Текстовое сообщение - принимаем только от пользователя (не от ботов)
    // Если ALLOWED_USER_ID = 0, принимаем от всех пользователей (но не от ботов)
    // Если ALLOWED_USER_ID установлен, принимаем только от этого пользователя
    
    bool shouldProcess = false;
    
    if (is_bot) {
      Serial.println("Сообщение от бота - игнорируем текстовые сообщения от ботов");
      shouldProcess = false;
    } else if (ALLOWED_USER_ID == 0) {
      // Принимаем от всех пользователей (но не от ботов)
      shouldProcess = true;
    } else if (from_id == ALLOWED_USER_ID) {
      // Принимаем только от указанного пользователя
      shouldProcess = true;
    } else {
      Serial.print("Сообщение от пользователя ");
      Serial.print(from_id);
      Serial.println(" - не разрешено");
      shouldProcess = false;
    }
    
    if (shouldProcess) {
      String text = String(msgText.c_str());
      Serial.print("Текст: ");
      Serial.println(text);
      Serial.print("Время: ");
      Serial.println(formatDateTime(timestamp));
      
      // Сохраняем сообщение в память
      saveLastMessage(text, from_name, timestamp);
      
      bool success = displayTextMessage(text, from_name, timestamp, true);
      
      // Отправляем ответное сообщение только если все успешно
      if (success) {
        fb::Message replyMsg;
        replyMsg.chatID = chatId;
        replyMsg.text = "Сообщение успешно выведено на экран!";
        bot.sendMessage(replyMsg);
        Serial.println("Ответное сообщение отправлено");
      } else {
        fb::Message errorMsg;
        errorMsg.chatID = chatId;
        errorMsg.text = "Ошибка при выводе сообщения на экран";
        bot.sendMessage(errorMsg);
        Serial.println("Ошибка при выводе сообщения");
      }
    }
  }
}

// Проверка, содержит ли строка кириллицу (проверка по байтам UTF-8)
bool hasCyrillic(String str) {
  const char* utf8 = str.c_str();
  int len = str.length();
  for (int i = 0; i < len; i++) {
    unsigned char c = (unsigned char)utf8[i];
    // Кириллица в UTF-8 начинается с байтов 0xD0 или 0xD1
    if (c == 0xD0 || c == 0xD1) {
      return true;
    }
  }
  return false;
}

// Функция для получения длины UTF-8 символа в байтах
int getUTF8CharLength(unsigned char firstByte) {
  if ((firstByte & 0x80) == 0) return 1;        // ASCII (1 байт)
  if ((firstByte & 0xE0) == 0xC0) return 2;     // 2 байта (кириллица)
  if ((firstByte & 0xF0) == 0xE0) return 3;      // 3 байта
  if ((firstByte & 0xF8) == 0xF0) return 4;     // 4 байта (эмодзи)
  return 1;  // По умолчанию
}

// Функция для получения следующего UTF-8 символа из строки
String getNextUTF8Char(String str, int startPos, int& bytesRead) {
  if (startPos >= str.length()) {
    bytesRead = 0;
    return "";
  }
  
  const char* utf8 = str.c_str();
  unsigned char firstByte = (unsigned char)utf8[startPos];
  int charLen = getUTF8CharLength(firstByte);
  
  if (startPos + charLen > str.length()) {
    bytesRead = str.length() - startPos;
    return str.substring(startPos);
  }
  
  bytesRead = charLen;
  return str.substring(startPos, startPos + charLen);
}

bool displayTextMessage(String text, String from_name, uint32_t timestamp, bool showCurrentTime, bool forceRedraw) {
  // Обновляем только если сообщение новое (если не принудительно)
  if (!forceRedraw && text == lastMessage && from_name == lastFromName && timestamp == lastTimestamp) {
    return true;  // Уже отображено
  }
  display.setFullWindow();
  lastMessage = text;
  lastFromName = from_name;
  lastTimestamp = timestamp;
  lastIsImage = false;
  lastImageWidth = 0;
  lastImageHeight = 0;
  
  Serial.println("Отображение текстового сообщения...");
  
  // Очистка экрана
  display.fillScreen(GxEPD_WHITE);
  
  // Настройка U8g2 для кириллицы
  u8g2_for_adafruit_gfx.setFontDirection(0);  // left to right
  u8g2_for_adafruit_gfx.setFontMode(1);       // transparent mode (1 = transparent, 0 = solid)
  u8g2_for_adafruit_gfx.setForegroundColor(GxEPD_BLACK);  // черный цвет текста
  u8g2_for_adafruit_gfx.setBackgroundColor(GxEPD_WHITE);  // белый фон (важно для правильного отображения)
  
  // Имя отправителя - используем U8g2 для всех символов
  String fromLabel = "От: ";
  fromLabel += from_name;
  u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);
  u8g2_for_adafruit_gfx.setCursor(10, 25);
  u8g2_for_adafruit_gfx.print(fromLabel);
  
  // Дата и время отправки - используем U8g2 для всех символов
  String dateTimeLabel = formatDateTime(timestamp);
  u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);
  int labelWidth = u8g2_for_adafruit_gfx.getUTF8Width(dateTimeLabel.c_str());
  u8g2_for_adafruit_gfx.setCursor(SCREEN_WIDTH - labelWidth - 10, 25);
  u8g2_for_adafruit_gfx.print(dateTimeLabel);
  
  // Разделительная линия
  display.drawFastHLine(10, 35, SCREEN_WIDTH - 20, GxEPD_BLACK);
  
  // Текст сообщения с переносами - используем U8g2 для всех символов
  // Разбиваем текст на строки с учетом переносов по словам
  int yPos = 60;
  int maxWidth = SCREEN_WIDTH - 20;
  String remainingText = text;
  
  while (remainingText.length() > 0 && yPos < SCREEN_HEIGHT - 30) {
    String line = "";
    int charCount = 0;
    int lastSpacePos = -1;  // Позиция последнего пробела в байтах
    
    // Используем U8g2 для всех символов (кириллица и латиница)
    u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);
    
    // Обрабатываем текст посимвольно с учетом переносов по словам и явных переносов строк
    while (charCount < remainingText.length()) {
      // Получаем следующий UTF-8 символ правильно
      int bytesRead = 0;
      String nextChar = getNextUTF8Char(remainingText, charCount, bytesRead);
      
      if (bytesRead == 0) break;  // Конец строки
      
      // Проверяем, является ли символ пробелом или переносом строки
      bool isSpace = (nextChar == " " || nextChar == "\n" || nextChar == "\r");
      bool isNewline = (nextChar == "\n");
      
      // Если это пробел (но не перенос строки), запоминаем его позицию
      if (isSpace && !isNewline) {
        lastSpacePos = charCount;
      }
      
      // Если встретили явный перенос строки, делаем перенос независимо от ширины
      if (isNewline) {
        charCount += bytesRead;  // Пропускаем \n
        break;  // Завершаем текущую строку
      }
      
      // Проверяем ширину строки С ДОБАВЛЕННЫМ символом
      String testLine = line + nextChar;
      int width = u8g2_for_adafruit_gfx.getUTF8Width(testLine.c_str());
      
      // Проверяем, помещается ли строка
      if (width > maxWidth) {
        if (line.length() > 0) {
          // Если есть пробел в строке, переносим по слову
          if (lastSpacePos >= 0) {
            // Берем текст до последнего пробела (используем позицию в байтах)
            int spaceBytes = 0;
            String spaceChar = getNextUTF8Char(remainingText, lastSpacePos, spaceBytes);
            line = remainingText.substring(0, lastSpacePos);
            charCount = lastSpacePos + spaceBytes;  // Пропускаем пробел
          } else {
            // Нет пробела - переносим по символу
            // Текущая строка уже заполнена максимально (line), выводим её
            // Обновляем charCount на длину обработанной строки line (в байтах UTF-8)
            charCount = line.length();
          }
          break;
        } else {
          // Даже один символ не помещается - все равно добавляем его
          line = testLine;
          charCount += bytesRead;
          break;
        }
      }
      
      // Символ помещается, добавляем его
      line = testLine;
      charCount += bytesRead;
      
      // Если достигли конца текста, выходим
      if (charCount >= remainingText.length()) {
        break;
      }
    }
    
    // Выводим строку через U8g2
    u8g2_for_adafruit_gfx.setCursor(10, yPos);
    u8g2_for_adafruit_gfx.print(line);
    yPos += LINE_SPACING; // Межстрочный интервал
    
    // Удаляем обработанную часть (включая пробелы в начале следующей строки)
    remainingText = remainingText.substring(charCount);
    // Убираем пробелы в начале следующей строки
    while (remainingText.length() > 0) {
      int bytesRead = 0;
      String firstChar = getNextUTF8Char(remainingText, 0, bytesRead);
      if (firstChar != " " && firstChar != "\r") break;
      remainingText = remainingText.substring(bytesRead);
    }
  }
  // Футер: текущее время и версия
  drawFooter(showCurrentTime);
  
  Serial.println("Запуск обновления экрана...");
  safeDisplayUpdate();  // В GxEPD2 используется display() вместо update()
  display.hibernate();  // Важно! Вызываем hibernate() после каждого обновления
  Serial.println("Обновление экрана завершено");
  
  Serial.println("Сообщение отображено!");
  return true;  // Успешно отображено
}

// Сохранение последнего сообщения в EEPROM
void saveLastMessage(String text, String from_name, uint32_t timestamp) {
  // Ограничиваем длину строк
  if (text.length() > MAX_TEXT_LENGTH - 1) {
    text = text.substring(0, MAX_TEXT_LENGTH - 1);
  }
  if (from_name.length() > MAX_FROMNAME_LENGTH - 1) {
    from_name = from_name.substring(0, MAX_FROMNAME_LENGTH - 1);
  }
  
  // Записываем magic number для проверки валидности данных
  EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  EEPROM.write(EEPROM_TYPE_ADDR, CONTENT_TEXT);
  
  // Записываем timestamp
  EEPROM.put(EEPROM_TIMESTAMP_ADDR, timestamp);
  uint16_t w = 0;
  uint16_t h = 0;
  EEPROM.put(EEPROM_IMG_WIDTH_ADDR, w);
  EEPROM.put(EEPROM_IMG_HEIGHT_ADDR, h);
  
  // Записываем имя отправителя (с нулевым терминатором)
  char fromBuffer[MAX_FROMNAME_LENGTH];
  from_name.toCharArray(fromBuffer, MAX_FROMNAME_LENGTH);
  for (int i = 0; i < MAX_FROMNAME_LENGTH; i++) {
    EEPROM.write(EEPROM_FROMNAME_ADDR + i, fromBuffer[i]);
  }
  
  // Записываем текст (с нулевым терминатором)
  char textBuffer[MAX_TEXT_LENGTH];
  text.toCharArray(textBuffer, MAX_TEXT_LENGTH);
  for (int i = 0; i < MAX_TEXT_LENGTH; i++) {
    EEPROM.write(EEPROM_TEXT_ADDR + i, textBuffer[i]);
  }
  
  // Сохраняем изменения
  EEPROM.commit();

  // Сохраняем текст во флэш
  File f = LittleFS.open(LAST_TEXT_PATH, "w");
  if (f) {
    f.print(text);
    f.close();
  } else {
    Serial.println("Ошибка: не удалось сохранить текст в LittleFS");
  }
  Serial.println("Сообщение сохранено в память");
}

// Сохранение метаданных последнего изображения в EEPROM
void saveLastImageMeta(String from_name, uint32_t timestamp, uint16_t width, uint16_t height) {
  if (from_name.length() > MAX_FROMNAME_LENGTH - 1) {
    from_name = from_name.substring(0, MAX_FROMNAME_LENGTH - 1);
  }
  EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC);
  EEPROM.write(EEPROM_TYPE_ADDR, CONTENT_IMAGE);
  EEPROM.put(EEPROM_TIMESTAMP_ADDR, timestamp);
  EEPROM.put(EEPROM_IMG_WIDTH_ADDR, width);
  EEPROM.put(EEPROM_IMG_HEIGHT_ADDR, height);
  
  char fromBuffer[MAX_FROMNAME_LENGTH];
  from_name.toCharArray(fromBuffer, MAX_FROMNAME_LENGTH);
  for (int i = 0; i < MAX_FROMNAME_LENGTH; i++) {
    EEPROM.write(EEPROM_FROMNAME_ADDR + i, fromBuffer[i]);
  }
  
  // Очищаем текст
  for (int i = 0; i < MAX_TEXT_LENGTH; i++) {
    EEPROM.write(EEPROM_TEXT_ADDR + i, 0);
  }
  
  EEPROM.commit();
  Serial.println("Метаданные изображения сохранены в память");
}

// Загрузка последнего состояния из EEPROM
bool loadLastState(String& text, String& from_name, uint32_t& timestamp, bool& isImage, uint16_t& imgW, uint16_t& imgH) {
  // Проверяем magic number
  uint32_t magic = 0;
  EEPROM.get(EEPROM_MAGIC_ADDR, magic);
  
  if (magic != EEPROM_MAGIC) {
    Serial.println("В памяти нет сохраненных данных (magic number не совпадает)");
    text = "";
    from_name = "";
    timestamp = 0;
    isImage = false;
    imgW = 0;
    imgH = 0;
    return false;
  }

  uint8_t type = EEPROM.read(EEPROM_TYPE_ADDR);
  isImage = (type == CONTENT_IMAGE);
  
  // Читаем timestamp и размеры
  EEPROM.get(EEPROM_TIMESTAMP_ADDR, timestamp);
  EEPROM.get(EEPROM_IMG_WIDTH_ADDR, imgW);
  EEPROM.get(EEPROM_IMG_HEIGHT_ADDR, imgH);
  
  // Читаем имя отправителя
  char fromBuffer[MAX_FROMNAME_LENGTH];
  for (int i = 0; i < MAX_FROMNAME_LENGTH; i++) {
    fromBuffer[i] = EEPROM.read(EEPROM_FROMNAME_ADDR + i);
  }
  fromBuffer[MAX_FROMNAME_LENGTH - 1] = '\0';  // Гарантируем нулевой терминатор
  from_name = String(fromBuffer);
  
  // Читаем текст из флэша
  text = "";
  if (LittleFS.exists(LAST_TEXT_PATH)) {
    File tf = LittleFS.open(LAST_TEXT_PATH, "r");
    if (tf) {
      text = tf.readString();
      tf.close();
    }
  }
  
  if (isImage) {
    Serial.println("Загружены метаданные изображения из памяти");
  } else if (text.length() > 0) {
    Serial.println("Загружено сообщение из памяти:");
    Serial.print("  Текст: ");
    Serial.println(text);
    Serial.print("  От: ");
    Serial.println(from_name);
    Serial.print("  Время: ");
    Serial.println(timestamp);
  }
  
  return true;
}

// Синхронизация времени через NTP
void syncTime() {
  Serial.println("Синхронизация времени через NTP...");
  
  // Настройка NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
  
  // Ждем синхронизации (максимум 10 секунд)
  int attempts = 0;
  while (time(nullptr) < 1000000000 && attempts < 20) {
    delay(500);
    attempts++;
    Serial.print(".");
  }
  Serial.println();
  
  if (time(nullptr) > 1000000000) {
    timeSynced = true;
    lastTimeUpdate = millis();
    Serial.print("Время синхронизировано: ");
    Serial.println(getCurrentDateTime());
  } else {
    timeSynced = false;
    Serial.println("Ошибка синхронизации времени");
  }
}

// Получение текущей даты и времени в формате ДД.ММ ЧЧ:ММ
String getCurrentDateTime() {
  if (!timeSynced) {
    return "";
  }
  
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%02d.%02d %02d:%02d",
           timeinfo->tm_mday,
           timeinfo->tm_mon + 1,
           timeinfo->tm_hour,
           timeinfo->tm_min);
  
  return String(buffer);
}

// Обновление времени на экране без полной перерисовки
void updateDisplayTime() {
  if (!timeSynced) {
    return;
  }
  
  Serial.println("Обновление времени на экране...");
  
  // Обновляем только область времени
  updateFooterTimePartial();
}

// Безопасное обновление экрана с защитой от WDT
void safeDisplayUpdate() {
  yield();
  ESP.wdtDisable();
  display.display();
  ESP.wdtEnable(8000);
  yield();
}

// Отрисовка футера (время + версия)
void drawFooter(bool showCurrentTime) {
  u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);

  if (showCurrentTime && timeSynced) {
    String currentTime = getCurrentDateTime();
    u8g2_for_adafruit_gfx.setCursor(10, SCREEN_HEIGHT - 10);
    u8g2_for_adafruit_gfx.print(currentTime);
  }

  String versionText = String(SKETCH_VERSION);
  int versionWidth = u8g2_for_adafruit_gfx.getUTF8Width(versionText.c_str());
  u8g2_for_adafruit_gfx.setCursor(SCREEN_WIDTH - versionWidth - 10, SCREEN_HEIGHT - 10);
  u8g2_for_adafruit_gfx.print(versionText);
}

// Частичное обновление только области времени
void updateFooterTimePartial() {
  if (!timeSynced) return;
  int y = SCREEN_HEIGHT - 20;
  int h = 20;
  int w = SCREEN_WIDTH / 2;

  display.setPartialWindow(0, y, w, h);
  display.firstPage();
  do {
    display.fillRect(0, y, w, h, GxEPD_WHITE);

    u8g2_for_adafruit_gfx.setFontDirection(0);
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(GxEPD_BLACK);
    u8g2_for_adafruit_gfx.setBackgroundColor(GxEPD_WHITE);
    u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);

    String currentTime = getCurrentDateTime();
    u8g2_for_adafruit_gfx.setCursor(10, SCREEN_HEIGHT - 10);
    u8g2_for_adafruit_gfx.print(currentTime);
  } while (display.nextPage());
}

// Обработка RAW потока: "RAW|W|H|<binary>"
bool processImageStream(fb::Fetcher& fetch, String from_name, uint32_t timestamp) {
  Serial.println("Ожидаю RAW|W|H|...");
  const char* magic = "RAW|";
  int magicPos = 0;
  int width = 0;
  int height = 0;
  bool hasWidth = false;
  bool hasHeight = false;

  // Ищем заголовок RAW|
  while (fetch.available()) {
    int c = fetch.read();
    if (c < 0) continue;
    if (c == magic[magicPos]) {
      magicPos++;
      if (magicPos == 4) break;
    } else {
      magicPos = (c == 'R') ? 1 : 0;
    }
    yield();
  }

  if (magicPos < 4) {
    Serial.println("Неверный формат: RAW| не найден");
    return false;
  }

  // Читаем ширину до '|'
  while (fetch.available()) {
    int c = fetch.read();
    if (c < 0) continue;
    if (c >= '0' && c <= '9') {
      hasWidth = true;
      width = width * 10 + (c - '0');
    } else if (c == '|' && hasWidth) {
      break;
    } else {
      Serial.println("Неверный формат: ширина");
      return false;
    }
    yield();
  }

  // Читаем высоту до '|'
  while (fetch.available()) {
    int c = fetch.read();
    if (c < 0) continue;
    if (c >= '0' && c <= '9') {
      hasHeight = true;
      height = height * 10 + (c - '0');
    } else if (c == '|' && hasHeight) {
      break;
    } else {
      Serial.println("Неверный формат: высота");
      return false;
    }
    yield();
  }

  if (!hasWidth || !hasHeight || width <= 0 || height <= 0) {
    Serial.println("Неверные размеры изображения");
    return false;
  }

  if (width > SCREEN_WIDTH || height > SCREEN_HEIGHT) {
    Serial.println("Изображение больше экрана");
    return false;
  }

  Serial.print("RAW размер: ");
  Serial.print(width);
  Serial.print("x");
  Serial.println(height);

  int bytesPerRow = (width + 7) / 8;
  int totalBytes = bytesPerRow * height;
  int byteIndex = 0;

  int xOffset = (SCREEN_WIDTH - width) / 2;
  int yOffset = (SCREEN_HEIGHT - height) / 2;

  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  Serial.println("Отображение RAW...");

  File outFile = LittleFS.open(LAST_IMAGE_PATH, "w");
  if (!outFile) {
    Serial.println("Ошибка: не удалось открыть файл для записи");
  }

  while (byteIndex < totalBytes && fetch.available()) {
    int c = fetch.read();
    if (c < 0) continue;
    uint8_t byte = (uint8_t)c;

    if (outFile) {
      outFile.write(byte);
    }

    int row = byteIndex / bytesPerRow;
    int colByte = byteIndex % bytesPerRow;
    int x = colByte * 8;
    int y = row;

    for (int bit = 7; bit >= 0 && (x + (7 - bit)) < width; bit--) {
      if (byte & (1 << bit)) {
        display.drawPixel(xOffset + x + (7 - bit), yOffset + y, GxEPD_BLACK);
      }
    }

    byteIndex++;
    if ((byteIndex % 64) == 0) {
      yield();
    }
  }

  if (outFile) {
    outFile.close();
  }

  if (byteIndex < totalBytes) {
    Serial.println("Ошибка: недостаточно данных RAW");
    return false;
  }

  // Футер с временем и версией
  drawFooter(true);

  // Хедер: от кого и когда
  if (from_name.length() > 0) {
    u8g2_for_adafruit_gfx.setFontDirection(0);
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(GxEPD_BLACK);
    u8g2_for_adafruit_gfx.setBackgroundColor(GxEPD_WHITE);
    u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);
    
    String fromLabel = "От: " + from_name;
    u8g2_for_adafruit_gfx.setCursor(10, 25);
    u8g2_for_adafruit_gfx.print(fromLabel);
    
    if (timestamp > 0) {
      String dateTimeLabel = formatDateTime(timestamp);
      int labelWidth = u8g2_for_adafruit_gfx.getUTF8Width(dateTimeLabel.c_str());
      u8g2_for_adafruit_gfx.setCursor(SCREEN_WIDTH - labelWidth - 10, 25);
      u8g2_for_adafruit_gfx.print(dateTimeLabel);
    }
    display.drawFastHLine(10, 35, SCREEN_WIDTH - 20, GxEPD_BLACK);
  }

  safeDisplayUpdate();
  display.hibernate();
  Serial.println("RAW изображение отображено!");

  // Сохраняем метаданные последнего изображения
  saveLastImageMeta(from_name, timestamp, width, height);
  lastIsImage = true;
  lastFromName = from_name;
  lastTimestamp = timestamp;
  lastImageWidth = width;
  lastImageHeight = height;
  lastMessage = "";
  return true;
}

// Обработка RAW потока из HTTP response body: RAW|W|H|<binary>
bool processImageHttpStream(Stream& stream, String from_name, uint32_t timestamp) {
  Serial.println("WEB fallback: ожидаю RAW|W|H|...");

  auto readByteWithTimeout = [&](unsigned long timeoutMs) -> int {
    unsigned long start = millis();
    while (millis() - start < timeoutMs) {
      int c = stream.read();
      if (c >= 0) return c;
      yield();
      delay(1);
    }
    return -1;
  };

  const char* magic = "RAW|";
  int magicPos = 0;
  int width = 0;
  int height = 0;
  bool hasWidth = false;
  bool hasHeight = false;

  // Ищем заголовок RAW|
  while (magicPos < 4) {
    int c = readByteWithTimeout(12000);
    if (c < 0) {
      Serial.println("WEB fallback: таймаут чтения заголовка");
      return false;
    }
    if (c == magic[magicPos]) {
      magicPos++;
    } else {
      magicPos = (c == 'R') ? 1 : 0;
    }
  }

  // Читаем ширину до '|'
  while (true) {
    int c = readByteWithTimeout(6000);
    if (c < 0) {
      Serial.println("WEB fallback: таймаут ширины");
      return false;
    }
    if (c >= '0' && c <= '9') {
      hasWidth = true;
      width = width * 10 + (c - '0');
    } else if (c == '|' && hasWidth) {
      break;
    } else {
      Serial.println("WEB fallback: неверный формат ширины");
      return false;
    }
  }

  // Читаем высоту до '|'
  while (true) {
    int c = readByteWithTimeout(6000);
    if (c < 0) {
      Serial.println("WEB fallback: таймаут высоты");
      return false;
    }
    if (c >= '0' && c <= '9') {
      hasHeight = true;
      height = height * 10 + (c - '0');
    } else if (c == '|' && hasHeight) {
      break;
    } else {
      Serial.println("WEB fallback: неверный формат высоты");
      return false;
    }
  }

  if (!hasWidth || !hasHeight || width <= 0 || height <= 0) {
    Serial.println("WEB fallback: неверные размеры");
    return false;
  }
  if (width > SCREEN_WIDTH || height > SCREEN_HEIGHT) {
    Serial.println("WEB fallback: изображение больше экрана");
    return false;
  }

  int bytesPerRow = (width + 7) / 8;
  int totalBytes = bytesPerRow * height;
  int byteIndex = 0;
  int xOffset = (SCREEN_WIDTH - width) / 2;
  int yOffset = (SCREEN_HEIGHT - height) / 2;

  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);

  File outFile = LittleFS.open(LAST_IMAGE_PATH, "w");
  if (!outFile) {
    Serial.println("WEB fallback: не удалось открыть файл изображения");
  }

  while (byteIndex < totalBytes) {
    int c = readByteWithTimeout(12000);
    if (c < 0) {
      if (outFile) outFile.close();
      Serial.println("WEB fallback: таймаут чтения bitmap");
      return false;
    }
    uint8_t byte = (uint8_t)c;
    if (outFile) outFile.write(byte);

    int row = byteIndex / bytesPerRow;
    int colByte = byteIndex % bytesPerRow;
    int x = colByte * 8;
    int y = row;

    for (int bit = 7; bit >= 0 && (x + (7 - bit)) < width; bit--) {
      if (byte & (1 << bit)) {
        display.drawPixel(xOffset + x + (7 - bit), yOffset + y, GxEPD_BLACK);
      }
    }

    byteIndex++;
    if ((byteIndex % 64) == 0) yield();
  }

  if (outFile) outFile.close();

  drawFooter(true);

  if (from_name.length() > 0) {
    u8g2_for_adafruit_gfx.setFontDirection(0);
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(GxEPD_BLACK);
    u8g2_for_adafruit_gfx.setBackgroundColor(GxEPD_WHITE);
    u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);

    String fromLabel = "От: " + from_name;
    u8g2_for_adafruit_gfx.setCursor(10, 25);
    u8g2_for_adafruit_gfx.print(fromLabel);

    if (timestamp > 0) {
      String dateTimeLabel = formatDateTime(timestamp);
      int labelWidth = u8g2_for_adafruit_gfx.getUTF8Width(dateTimeLabel.c_str());
      u8g2_for_adafruit_gfx.setCursor(SCREEN_WIDTH - labelWidth - 10, 25);
      u8g2_for_adafruit_gfx.print(dateTimeLabel);
    }
    display.drawFastHLine(10, 35, SCREEN_WIDTH - 20, GxEPD_BLACK);
  }

  safeDisplayUpdate();
  display.hibernate();

  saveLastImageMeta(from_name, timestamp, width, height);
  lastIsImage = true;
  lastFromName = from_name;
  lastTimestamp = timestamp;
  lastImageWidth = width;
  lastImageHeight = height;
  lastMessage = "";
  return true;
}

// Обработка предобработанного изображения в формате IMG|WIDTH|HEIGHT|BASE64_DATA
// Файлы от Python бота содержат данные в этом формате
bool processImageMessage(String text) {
  return processImageMessageBuffer(text.c_str(), text.length());
}

bool processImageMessageBuffer(const char* data, int len) {
  Serial.println("Обработка предобработанного изображения...");
  
  // Проверяем формат: IMG|WIDTH|HEIGHT|BASE64_DATA
  // Иногда в начале могут быть служебные байты/пробелы, поэтому ищем "IMG|" вручную
  int imgPos = -1;
  for (int i = 0; i + 3 < len; i++) {
    if (data[i] == 'I' && data[i + 1] == 'M' && data[i + 2] == 'G' && data[i + 3] == '|') {
      imgPos = i;
      break;
    }
  }
  if (imgPos < 0) {
    Serial.println("Неверный формат сообщения");
    return false;
  }

  // Парсим формат: IMG|WIDTH|HEIGHT|BASE64_DATA
  int pos = imgPos + 4;
  int width = 0;
  int height = 0;
  bool hasWidth = false;
  bool hasHeight = false;

  while (pos < len && data[pos] >= '0' && data[pos] <= '9') {
    hasWidth = true;
    width = width * 10 + (data[pos] - '0');
    pos++;
  }
  if (!hasWidth || pos >= len || data[pos] != '|') {
    Serial.println("Неверный формат: не удалось прочитать ширину");
    return false;
  }
  pos++; // пропускаем '|'

  while (pos < len && data[pos] >= '0' && data[pos] <= '9') {
    hasHeight = true;
    height = height * 10 + (data[pos] - '0');
    pos++;
  }
  if (!hasHeight || pos >= len || data[pos] != '|') {
    Serial.println("Неверный формат: не удалось прочитать высоту");
    return false;
  }
  pos++; // пропускаем '|'
  
  Serial.print("Размер изображения: ");
  Serial.print(width);
  Serial.print("x");
  Serial.println(height);
  
  // Извлекаем base64 данные (все, что после заголовка)
  const char* base64Data = data + pos;
  int base64Len = len - pos;
  return displayProcessedImageBase64(base64Data, base64Len, width, height, "", 0);
}

// Отображение предобработанного изображения
bool displayProcessedImageBase64(const char* base64Data, int base64Len, int imgWidth, int imgHeight, String from_name, uint32_t timestamp) {
  Serial.println("Декодирование base64 данных...");
  
  // Декодируем base64 (используем встроенную функцию ESP8266)
  int padding = 0;
  
  // Подсчитываем padding
  if (base64Len > 0 && base64Data[base64Len - 1] == '=') padding++;
  if (base64Len > 1 && base64Data[base64Len - 2] == '=') padding++;
  
  int decodedLen = (base64Len * 3) / 4 - padding;
  if (decodedLen <= 0) {
    Serial.println("Ошибка декодирования base64");
    return false;
  }

  uint8_t* decoded = (uint8_t*)malloc(decodedLen);
  if (!decoded) {
    Serial.println("Ошибка: недостаточно памяти для декодирования");
    return false;
  }

  int outIndex = 0;
  for (int i = 0; i < base64Len; i += 4) {
    if (i + 3 >= base64Len) break;

    uint32_t value = 0;
    for (int j = 0; j < 4; j++) {
      char c = base64Data[i + j];
      uint8_t v = 0;

      if (c >= 'A' && c <= 'Z') v = c - 'A';
      else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
      else if (c >= '0' && c <= '9') v = c - '0' + 52;
      else if (c == '+') v = 62;
      else if (c == '/') v = 63;
      else if (c == '=') v = 0;
      else v = 0;

      value = (value << 6) | v;
    }

    if (outIndex < decodedLen) decoded[outIndex++] = (value >> 16) & 0xFF;
    if (outIndex < decodedLen && (i + 2 < base64Len - padding)) decoded[outIndex++] = (value >> 8) & 0xFF;
    if (outIndex < decodedLen && (i + 3 < base64Len - padding)) decoded[outIndex++] = value & 0xFF;
  }

  if (outIndex == 0) {
    free(decoded);
    Serial.println("Ошибка декодирования base64");
    return false;
  }

  Serial.print("Размер декодированных данных: ");
  Serial.print(outIndex);
  Serial.println(" байт");
  
  // Вычисляем размер изображения из данных
  // Формат: каждый байт содержит 8 пикселей
  if (imgWidth <= 0 || imgHeight <= 0 || imgWidth > SCREEN_WIDTH || imgHeight > SCREEN_HEIGHT) {
    Serial.println("Ошибка: неверные размеры изображения");
    free(decoded);
    return false;
  }

  int expectedBytes = (imgWidth * imgHeight) / 8;
  int width = imgWidth;
  int height = imgHeight;

  if (outIndex < expectedBytes) {
    Serial.print("Предупреждение: размер данных меньше ожидаемого. Ожидалось: ");
    Serial.print(expectedBytes);
    Serial.print(", получено: ");
    Serial.println(outIndex);
  }
  
  // Очищаем экран
  display.fillScreen(GxEPD_WHITE);
  
  Serial.println("Отображение изображения...");
  
  // Отображаем битмап
  // Каждый байт содержит 8 пикселей (1 бит на пиксель)
  int byteIndex = 0;
  int xOffset = (SCREEN_WIDTH - width) / 2;
  int yOffset = (SCREEN_HEIGHT - height) / 2;
  for (int y = 0; y < height && byteIndex < outIndex; y++) {
    for (int x = 0; x < width; x += 8) {
      if (byteIndex >= outIndex) break;
      
      uint8_t byte = decoded[byteIndex];
      
      // Извлекаем 8 пикселей из байта
      for (int bit = 7; bit >= 0 && (x + (7 - bit)) < width; bit--) {
        if (byte & (1 << bit)) {
          display.drawPixel(xOffset + x + (7 - bit), yOffset + y, GxEPD_BLACK);
        }
      }
      
      byteIndex++;
    }
  }

  free(decoded);
  
  // Добавляем информацию об отправителе и времени (если есть)
  if (from_name.length() > 0) {
    u8g2_for_adafruit_gfx.setFontDirection(0);
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(GxEPD_BLACK);
    u8g2_for_adafruit_gfx.setBackgroundColor(GxEPD_WHITE);
    u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);
    
    String fromLabel = "От: " + from_name;
    u8g2_for_adafruit_gfx.setCursor(10, 15);
    u8g2_for_adafruit_gfx.print(fromLabel);
    
    if (timestamp > 0) {
      String dateTimeLabel = formatDateTime(timestamp);
      int labelWidth = u8g2_for_adafruit_gfx.getUTF8Width(dateTimeLabel.c_str());
      u8g2_for_adafruit_gfx.setCursor(SCREEN_WIDTH - labelWidth - 10, 15);
      u8g2_for_adafruit_gfx.print(dateTimeLabel);
    }
  }
  
  // Футер с временем и версией
  u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);
  
  if (timeSynced) {
    String currentTime = getCurrentDateTime();
    u8g2_for_adafruit_gfx.setCursor(10, SCREEN_HEIGHT - 10);
    u8g2_for_adafruit_gfx.print(currentTime);
  }
  
  String versionText = String(SKETCH_VERSION);
  int versionWidth = u8g2_for_adafruit_gfx.getUTF8Width(versionText.c_str());
  u8g2_for_adafruit_gfx.setCursor(SCREEN_WIDTH - versionWidth - 10, SCREEN_HEIGHT - 10);
  u8g2_for_adafruit_gfx.print(versionText);
  
  Serial.println("Обновление экрана...");
  display.display();
  display.hibernate();
  
  Serial.println("Изображение успешно отображено!");
  
  return true;
}

// Отображение последнего изображения из LittleFS
bool displayImageFromFile(String from_name, uint32_t timestamp, uint16_t width, uint16_t height, bool showCurrentTime) {
  if (!LittleFS.exists(LAST_IMAGE_PATH)) {
    Serial.println("Файл последнего изображения не найден");
    return false;
  }

  if (width == 0 || height == 0) {
    Serial.println("Неверные размеры изображения");
    return false;
  }

  File f = LittleFS.open(LAST_IMAGE_PATH, "r");
  if (!f) {
    Serial.println("Не удалось открыть файл последнего изображения");
    return false;
  }

  int bytesPerRow = (width + 7) / 8;
  int totalBytes = bytesPerRow * height;
  int byteIndex = 0;
  int xOffset = (SCREEN_WIDTH - width) / 2;
  int yOffset = (SCREEN_HEIGHT - height) / 2;

  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  Serial.println("Отображение изображения из файла...");

  while (f.available() && byteIndex < totalBytes) {
    int c = f.read();
    if (c < 0) continue;
    uint8_t byte = (uint8_t)c;

    int row = byteIndex / bytesPerRow;
    int colByte = byteIndex % bytesPerRow;
    int x = colByte * 8;
    int y = row;

    for (int bit = 7; bit >= 0 && (x + (7 - bit)) < width; bit--) {
      if (byte & (1 << bit)) {
        display.drawPixel(xOffset + x + (7 - bit), yOffset + y, GxEPD_BLACK);
      }
    }

    byteIndex++;
    if ((byteIndex % 64) == 0) yield();
  }
  f.close();

  // Хедер: от кого и когда
  if (from_name.length() > 0) {
    u8g2_for_adafruit_gfx.setFontDirection(0);
    u8g2_for_adafruit_gfx.setFontMode(1);
    u8g2_for_adafruit_gfx.setForegroundColor(GxEPD_BLACK);
    u8g2_for_adafruit_gfx.setBackgroundColor(GxEPD_WHITE);
    u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);

    String fromLabel = "От: " + from_name;
    u8g2_for_adafruit_gfx.setCursor(10, 25);
    u8g2_for_adafruit_gfx.print(fromLabel);

    if (timestamp > 0) {
      String dateTimeLabel = formatDateTime(timestamp);
      int labelWidth = u8g2_for_adafruit_gfx.getUTF8Width(dateTimeLabel.c_str());
      u8g2_for_adafruit_gfx.setCursor(SCREEN_WIDTH - labelWidth - 10, 25);
      u8g2_for_adafruit_gfx.print(dateTimeLabel);
    }
    display.drawFastHLine(10, 35, SCREEN_WIDTH - 20, GxEPD_BLACK);
  }

  // Футер: текущее время и версия
  u8g2_for_adafruit_gfx.setFont(CYRILLIC_FONT);
  if (showCurrentTime && timeSynced) {
    String currentTime = getCurrentDateTime();
    u8g2_for_adafruit_gfx.setCursor(10, SCREEN_HEIGHT - 10);
    u8g2_for_adafruit_gfx.print(currentTime);
  }
  String versionText = String(SKETCH_VERSION);
  int versionWidth = u8g2_for_adafruit_gfx.getUTF8Width(versionText.c_str());
  u8g2_for_adafruit_gfx.setCursor(SCREEN_WIDTH - versionWidth - 10, SCREEN_HEIGHT - 10);
  u8g2_for_adafruit_gfx.print(versionText);

  safeDisplayUpdate();
  display.hibernate();
  Serial.println("Изображение из файла отображено!");
  return true;
}
