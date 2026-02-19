# E-Paper Telegram Bot

Проект состоит из двух частей:
- `e-paper-tg.ino` — прошивка ESP8266 + e-paper дисплея.
- `telegram_bot_processor.py` — серверный бот, который принимает фото, конвертирует их в RAW-формат для e-paper и отправляет файл в Telegram.

## Содержание

- [1. Функционал системы](#1-функционал-системы)
- [2. Как это работает](#2-как-это-работает)
- [3. Прошивка Arduino (ESP8266)](#3-прошивка-arduino-esp8266)
- [4. Установка серверного бота (Docker + Telethon)](#4-установка-серверного-бота-docker--telethon)
- [5. Первая авторизация Telethon в Docker](#5-первая-авторизация-telethon-в-docker)
- [6. Ежедневная эксплуатация и обновления](#6-ежедневная-эксплуатация-и-обновления)
- [7. Частые проблемы](#7-частые-проблемы)

## 1. Функционал системы

### На ESP8266 (`e-paper-tg.ino`)

- Получает текстовые сообщения из Telegram и выводит на e-paper.
- Получает файлы с изображениями в формате `.epd` (`RAW|W|H|<binary>`) и выводит их на экран.
- Сохраняет последнее состояние (текст/картинку) в памяти и восстанавливает после перезагрузки.
- Показывает в футере:
  - текущее время/дату (через NTP),
  - версию прошивки.
- Обновляет время частично (partial update), а при новом сообщении делает полный рендер.

### На сервере (`telegram_bot_processor.py`)

- Принимает фото от пользователя в Telegram.
- Масштабирует изображение под экран, переводит в ч/б (dither Floyd-Steinberg), упаковывает в RAW.
- Отправляет файл в целевой чат (`TARGET_CHAT_ID`) или обратно в исходный чат.
- Может отправлять в группу через userbot (Telethon), если нужен обход ограничения Bot API.

## 2. Как это работает

1. Вы отправляете фото боту-обработчику.
2. Серверный бот конвертирует фото в файл `epaper_img_*.epd`.
3. Файл отправляется в нужный чат/группу.
4. ESP8266-бот читает файл и рисует изображение на e-paper.
5. Последний контент сохраняется и переживает перезапуск питания.

## 3. Прошивка Arduino (ESP8266)

### 3.1 Подключение дисплея (NodeMCU v3)

| E-Paper | NodeMCU | GPIO |
|---|---|---|
| VCC | 3.3V | - |
| GND | GND | - |
| SDA | D7 | GPIO13 |
| SCL | D5 | GPIO14 |
| CS | D8 | GPIO15 |
| D/C | D2 | GPIO4 |
| RES | D1 | GPIO5 |
| BUSY | D6 | GPIO12 |

### 3.2 Библиотеки Arduino IDE

Установите через Library Manager:
- `FastBot2`
- `GxEPD2`
- `U8g2_for_Adafruit_GFX`

Также нужен `ESP8266` board package (NodeMCU/ESP-12E).

### 3.3 Настройка скетча

Откройте локальный `e-paper-tg.ino` и укажите:
- `ssid`
- `password`
- `botToken`
- при необходимости `ALLOWED_USER_ID`
- при необходимости `TIMEZONE_OFFSET`

### 3.4 Заливка в плату

1. Подключите ESP8266 по USB.
2. В Arduino IDE выберите:
   - Board: `NodeMCU 1.0 (ESP-12E Module)` (или эквивалент),
   - правильный COM-порт.
3. Нажмите Upload.
4. Откройте Serial Monitor (`115200`) и проверьте:
   - подключение к Wi-Fi,
   - инициализацию дисплея,
   - старт Telegram-бота.

## 4. Установка серверного бота (Docker + Telethon)

### 4.1 Подготовка сервера

```bash
git clone <REPO_URL>
cd e-paper-tg
cp .env.example .env
mkdir -p sessions
```

### 4.2 Заполнение `.env`

Минимально:

```env
BOT_TOKEN=...
TARGET_CHAT_ID=-100XXXXXXXXXX
TARGET_WIDTH=400
TARGET_HEIGHT=300
MAX_OUTPUT_BYTES=20000
```

Для режима Telethon (группа):

```env
USE_USERBOT_FOR_GROUP=true
USERBOT_API_ID=12345678
USERBOT_API_HASH=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
USERBOT_PHONE=+79XXXXXXXXX
USERBOT_SESSION=sessions/userbot_session
```

### 4.3 Сборка и запуск

```bash
docker compose up -d --build
```

Логи:

```bash
docker compose logs -f epaper-processor
```

## 5. Первая авторизация Telethon в Docker

Если `USE_USERBOT_FOR_GROUP=true`, первый вход сделайте интерактивно:

```bash
docker compose run --rm epaper-processor python telegram_bot_processor.py
```

Далее:
1. Введите код подтверждения из Telegram.
2. Если включен 2FA — введите пароль.
3. После успеха появится файл сессии в `sessions/`.

Проверка:

```bash
ls -la sessions
```

После этого запускайте как сервис:

```bash
docker compose up -d --build
```

## 6. Ежедневная эксплуатация и обновления

Обновить сервер после изменений в GitHub:

```bash
git pull
docker compose up -d --build
```

Проверить, что бот работает:

```bash
docker compose ps
docker compose logs -f epaper-processor
```

## 7. Частые проблемы

- `Current status is: DISABLED` у бота — в `@BotFather`: `/setprivacy` -> `Disable`.
- Бот в группе не видит сообщение от другого бота — используйте режим Telethon (`USE_USERBOT_FOR_GROUP=true`).
- Telethon просит логин на каждом старте — проверьте volume `./sessions:/app/sessions` и `USERBOT_SESSION=sessions/...`.
- Контейнер не стартует — проверьте, что в `.env` задан `BOT_TOKEN`.

## Безопасность

- Не коммитьте `.env`.
- Не коммитьте `sessions/` и `*.session`.
- Не храните токены и API hash в публичном коде.

