# E-Paper Control (Telegram + Web Fallback)

Проект поддерживает два канала доставки контента на устройство:
- Telegram (как раньше, через `telegram_bot_processor.py` + Telethon при необходимости).
- Web fallback (новый TypeScript сервис `paper-web` с формой отправки текста/картинки и API polling для ESP8266).

## Содержание

- [1. Функционал системы](#1-функционал-системы)
- [2. Как это работает](#2-как-это-работает)
- [3. Прошивка Arduino (ESP8266)](#3-прошивка-arduino-esp8266)
- [4. Docker сервисы](#4-docker-сервисы)
- [5. Telegram канал (как раньше)](#5-telegram-канал-как-раньше)
- [6. Web fallback (новый канал)](#6-web-fallback-новый-канал)
- [7. Эксплуатация и обновления](#7-эксплуатация-и-обновления)
- [8. Частые проблемы](#8-частые-проблемы)

## 1. Функционал системы

### На ESP8266 (`e-paper-tg.ino`)

- Получает текстовые сообщения из Telegram и выводит на e-paper.
- Получает файлы с изображениями в формате `.epd` (`RAW|W|H|<binary>`) и выводит их на экран.
- Дополнительно опрашивает web API (`/api/device/pull`) и принимает:
  - `text` в body,
  - `image` в body (`RAW|W|H|<binary>`).
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

### На сервере (`web-panel`, TypeScript)

- Страница с логином/паролем.
- Страница отправки текста и изображения на конкретное устройство.
- Страница настроек устройств (`device_id`, `api_key`, enabled).
- Очередь команд и endpoint для устройства: `GET /api/device/pull?device_id=...&api_key=...`.

## 2. Как это работает

Канал A (Telegram):
1. Вы отправляете фото боту-обработчику.
2. Серверный бот конвертирует фото в `epaper_img_*.epd`.
3. ESP8266 получает файл через Telegram и отображает.

Канал B (Web fallback):
1. Вы открываете web-панель (например `paper.eg5or.ru`).
2. Выбираете устройство и отправляете текст/картинку.
3. ESP8266 опрашивает `/api/device/pull` и получает команду.
4. Команда отображается на дисплее.

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
- web fallback настройки:
  - `WEB_FALLBACK_ENABLED`
  - `WEB_API_BASE_URL` (например `https://paper.eg5or.ru`)
  - `WEB_DEVICE_ID`
  - `WEB_DEVICE_API_KEY`

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

## 4. Docker сервисы

В `docker-compose.yml` поднимаются три сервиса:
- `postgres` — основная БД для web-панели (устройства + очередь команд).
- `epaper-processor` — Telegram image processor (Python).
- `paper-web` — Web fallback панель и API для устройств (TypeScript).

Подготовка:

```bash
git clone <REPO_URL>
cd e-paper-tg
cp .env.example .env
cp .env.web.example .env.web
mkdir -p sessions postgres-data
```

Запуск:

```bash
docker compose up -d --build
```

Логи:

```bash
docker compose logs -f epaper-processor
docker compose logs -f paper-web
```

## 5. Telegram канал (как раньше)

### 5.1 Заполнение `.env`

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

### 5.2 Первая авторизация Telethon в Docker

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

## 6. Web fallback (новый канал)

### 6.1 Настройка `.env.web`

```env
BACKEND_PORT=3028
WEB_USERNAME=admin
WEB_PASSWORD=change_me
WEB_SESSION_SECRET=change_this_secret
DATABASE_URL=postgresql://paper:change_me@postgres:5432/paper
WEB_TARGET_WIDTH=400
WEB_TARGET_HEIGHT=300
WEB_MAX_OUTPUT_BYTES=20000
```

### 6.2 Первый запуск web-панели

После `docker compose up -d --build` сервис доступен на:
- `http://SERVER_IP:3028`

Дальше через reverse proxy можно привязать к поддомену `paper.eg5or.ru`.

### 6.2.1 Локальный запуск web-сервиса (для разработки UI)

```bash
cd web-panel
npm install
npm run dev
```

Локально будет:
- Frontend (React): `http://localhost:5173`
- Backend API: `http://localhost:3001`
- DB для локалки можно поднять отдельным Postgres или использовать контейнер `postgres`.

### 6.3 Настройка устройства в web-панели

1. Откройте `/settings`.
2. Добавьте устройство:
   - `Device ID` (например `device-001`)
   - `API Key` (случайная строка)
   - `Enabled = true`
3. Те же `Device ID` и `API Key` пропишите в `e-paper-tg.ino`
   (`WEB_DEVICE_ID`, `WEB_DEVICE_API_KEY`).

## 7. Эксплуатация и обновления

Обновить сервер после изменений в GitHub:

```bash
git pull
docker compose up -d --build
```

Проверить, что бот работает:

```bash
docker compose ps
docker compose logs -f epaper-processor
docker compose logs -f paper-web
```

### 7.1 CLI helper `eg5or`

В репозитории есть скрипт `tools/eg5or` для быстрого управления стеками:
- `paper` (`~/e-paper-tg`)
- `site` (`~/eg5or-site`)
- `caddy` (`~/caddy-eg5or`)
- `all` (все по очереди)

Установка:

```bash
cd ~/e-paper-tg
sudo ln -sf "$PWD/tools/eg5or" /usr/local/bin/eg5or
```

Примеры:

```bash
eg5or help
eg5or status all
eg5or restart paper
eg5or rebuild paper
eg5or deploy all
eg5or logs paper 300
```

## 8. Частые проблемы

- `Current status is: DISABLED` у бота — в `@BotFather`: `/setprivacy` -> `Disable`.
- Бот в группе не видит сообщение от другого бота — используйте режим Telethon (`USE_USERBOT_FOR_GROUP=true`).
- Telethon просит логин на каждом старте — проверьте volume `./sessions:/app/sessions` и `USERBOT_SESSION=sessions/...`.
- Контейнер не стартует — проверьте, что в `.env` задан `BOT_TOKEN`.
- ESP не получает web-команды — проверьте `WEB_API_BASE_URL`, `WEB_DEVICE_ID`, `WEB_DEVICE_API_KEY` в прошивке и устройство в `/settings`.

## Безопасность

- Не коммитьте `.env`.
- Не коммитьте `sessions/` и `*.session`.
- Не храните токены и API hash в публичном коде.

