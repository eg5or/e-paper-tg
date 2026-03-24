#!/usr/bin/env python3
"""
Telegram бот для автоматической обработки изображений
При получении фото автоматически конвертирует его и отправляет обратно
"""

import asyncio
import io
import math
import os
from PIL import Image
from telegram import Update, Bot, InputFile
from telegram.ext import Application, MessageHandler, filters, ContextTypes

try:
    from telethon import TelegramClient
except Exception:
    TelegramClient = None

# Настройки
def env_bool(name: str, default: bool) -> bool:
    value = os.getenv(name)
    if value is None:
        return default
    return value.strip().lower() in ("1", "true", "yes", "on")


def env_int(name: str, default: int = 0) -> int:
    value = os.getenv(name)
    if value is None or value.strip() == "":
        return default
    return int(value.strip())


def env_optional_int(name: str):
    value = os.getenv(name)
    if value is None or value.strip() == "":
        return None
    return int(value.strip())


BOT_TOKEN = os.getenv("BOT_TOKEN", "").strip()
TARGET_WIDTH = env_int("TARGET_WIDTH", 400)
TARGET_HEIGHT = env_int("TARGET_HEIGHT", 300)

# ID чата для автоматической отправки обработанных изображений
# Если None - отправляется в тот же чат, откуда пришло фото
TARGET_CHAT_ID = env_optional_int("TARGET_CHAT_ID")

# Лимит размера итогового файла (в байтах) для ускорения чтения на ESP
# Формат файла: RAW|W|H|<binary>
MAX_OUTPUT_BYTES = env_int("MAX_OUTPUT_BYTES", 20000)

# Отправка в группу через Userbot (обходит ограничение Bot API: боты не видят других ботов в группах)
USE_USERBOT_FOR_GROUP = env_bool("USE_USERBOT_FOR_GROUP", False)
USERBOT_API_ID = env_int("USERBOT_API_ID", 0)
USERBOT_API_HASH = os.getenv("USERBOT_API_HASH", "").strip()
USERBOT_PHONE = os.getenv("USERBOT_PHONE", "").strip()
USERBOT_SESSION = os.getenv("USERBOT_SESSION", "sessions/userbot_session").strip()

user_client = None

def is_group_chat_id(chat_id):
    return isinstance(chat_id, int) and chat_id < 0


def mask_secret(value: str, keep_start: int = 2, keep_end: int = 2) -> str:
    if not value:
        return "<empty>"
    if len(value) <= keep_start + keep_end:
        return "*" * len(value)
    return f"{value[:keep_start]}...{value[-keep_end:]}"


def mask_phone(phone: str) -> str:
    if not phone:
        return "<empty>"
    if len(phone) <= 6:
        return "*" * len(phone)
    return f"{phone[:3]}***{phone[-2:]}"


def safe_config_log() -> str:
    token_id = BOT_TOKEN.split(":", 1)[0] if ":" in BOT_TOKEN else "<invalid>"
    return (
        f"BOT_TOKEN_ID={token_id}, "
        f"TARGET_CHAT_ID={TARGET_CHAT_ID}, "
        f"USE_USERBOT_FOR_GROUP={USE_USERBOT_FOR_GROUP}, "
        f"USERBOT_API_ID={USERBOT_API_ID}, "
        f"USERBOT_API_HASH={mask_secret(USERBOT_API_HASH, 4, 4)}, "
        f"USERBOT_PHONE={mask_phone(USERBOT_PHONE)}, "
        f"USERBOT_SESSION={USERBOT_SESSION}"
    )


async def init_userbot():
    global user_client
    if not USE_USERBOT_FOR_GROUP:
        print("Userbot отключен (USE_USERBOT_FOR_GROUP=false).")
        return
    if TelegramClient is None:
        print("Telethon не установлен. Установите telethon или отключите USE_USERBOT_FOR_GROUP.")
        return
    if USERBOT_API_ID == 0 or not USERBOT_API_HASH or not USERBOT_PHONE:
        print("Userbot не настроен. Заполните USERBOT_API_ID/USERBOT_API_HASH/USERBOT_PHONE.")
        return
    session_file = f"{USERBOT_SESSION}.session"
    print(
        f"Проверка session-файла: {session_file} "
        f"(exists={os.path.exists(session_file)})"
    )
    print("Инициализация userbot...")
    print(f"Конфиг userbot: {safe_config_log()}")
    user_client = TelegramClient(
        USERBOT_SESSION,
        USERBOT_API_ID,
        USERBOT_API_HASH,
        lang_code="ru",
        system_lang_code="ru-RU",
    )
    await user_client.start(phone=USERBOT_PHONE)
    print("Userbot подключен и готов отправлять файлы в группы.")

async def shutdown_userbot():
    if user_client and user_client.is_connected():
        await user_client.disconnect()

async def send_via_userbot(target_chat, file_bytes, file_name, caption):
    if user_client is None or not user_client.is_connected():
        return False
    file_data = io.BytesIO(file_bytes)
    file_data.name = file_name
    await user_client.send_file(target_chat, file_data, caption=caption)
    return True

async def process_image_to_bitmap(image_bytes):
    """
    Конвертирует изображение в монохромный битмап
    
    Returns:
        tuple: (width, height, raw_bitmap_bytes, preview_png_bytes)
    """
    # Рассчитываем максимально допустимое количество пикселей по лимиту файла
    # Формат: "RAW|W|H|<binary>", 1 байт = 8 пикселей
    # Оставляем запас под заголовок "RAW|W|H|"
    header_reserve = 64
    max_raw_bytes = max(0, MAX_OUTPUT_BYTES - header_reserve)
    max_pixels = max_raw_bytes * 8

    # Если лимит меньше полного кадра, уменьшаем рабочий размер
    max_target_pixels = TARGET_WIDTH * TARGET_HEIGHT
    if max_pixels > 0 and max_pixels < max_target_pixels:
        scale = math.sqrt(max_pixels / max_target_pixels)
        target_w = max(1, int(TARGET_WIDTH * scale))
        target_h = max(1, int(TARGET_HEIGHT * scale))
    else:
        target_w = TARGET_WIDTH
        target_h = TARGET_HEIGHT

    # Открываем изображение
    img = Image.open(io.BytesIO(image_bytes))
    
    # Конвертируем в RGB если нужно
    if img.mode != 'RGB':
        img = img.convert('RGB')
    
    # Изменяем размер с сохранением пропорций
    img.thumbnail((target_w, target_h), Image.Resampling.LANCZOS)
    
    # Создаем новое изображение нужного размера с белым фоном
    new_img = Image.new('RGB', (target_w, target_h), 'white')
    
    # Вставляем изображение по центру
    x_offset = (target_w - img.width) // 2
    y_offset = (target_h - img.height) // 2
    new_img.paste(img, (x_offset, y_offset))
    
    # Конвертируем в градации серого
    gray_img = new_img.convert('L')
    
    # Применяем дизеринг Floyd-Steinberg для лучшего качества
    mono_img = gray_img.convert('1', dither=Image.Dither.FLOYDSTEINBERG)
    
    # Конвертируем в массив байтов (1 бит на пиксель)
    width, height = mono_img.size
    bitmap_data = bytearray()
    
    for y in range(height):
        byte = 0
        bit_pos = 7
        for x in range(width):
            pixel = mono_img.getpixel((x, y))
            if pixel == 0:  # Черный пиксель
                byte |= (1 << bit_pos)
            
            bit_pos -= 1
            if bit_pos < 0:
                bitmap_data.append(byte)
                byte = 0
                bit_pos = 7
        
        # Если строка не кратна 8, дополняем последний байт
        if bit_pos != 7:
            bitmap_data.append(byte)
    
    # Готовим превью PNG для отправки в Telegram
    preview_buf = io.BytesIO()
    mono_img.convert("L").save(preview_buf, format="PNG")
    preview_bytes = preview_buf.getvalue()
    
    return width, height, bytes(bitmap_data), preview_bytes

async def handle_photo(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Обработчик фото из Telegram"""
    try:
        chat_id = update.effective_chat.id
        
        # Получаем самое большое фото
        photo = update.message.photo[-1]
        
        # Скачиваем файл
        file = await context.bot.get_file(photo.file_id)
        file_bytes = await file.download_as_bytearray()
        
        # Отправляем сообщение о начале обработки
        await update.message.reply_text("Обрабатываю изображение...")
        
        # Обрабатываем изображение
        width, height, raw_data, preview_png = await process_image_to_bitmap(file_bytes)
        
        # Формируем RAW файл для отправки
        # Формат: "RAW|WIDTH|HEIGHT|<binary>"
        header = f"RAW|{width}|{height}|".encode("ascii")
        result_bytes = header + raw_data
        if MAX_OUTPUT_BYTES > 0 and len(result_bytes) > MAX_OUTPUT_BYTES:
            print(
                f"ВНИМАНИЕ: размер {len(result_bytes)} байт превышает лимит {MAX_OUTPUT_BYTES}. "
                "Уменьшите MAX_OUTPUT_BYTES или TARGET_*."
            )
        
        # Определяем целевой чат для отправки
        target_chat = TARGET_CHAT_ID if TARGET_CHAT_ID is not None else chat_id
        
        file_name = f"epaper_img_{width}x{height}.epd"  # Расширение .epd для идентификации
        caption = f"Обработанное изображение {width}x{height}"
        
        # Если цель - группа, пытаемся отправить через userbot (обходит ограничения Bot API)
        sent_via_userbot = False
        if USE_USERBOT_FOR_GROUP and is_group_chat_id(target_chat):
            sent_via_userbot = await send_via_userbot(target_chat, result_bytes, file_name, caption)
            if sent_via_userbot:
                print(f"Файл отправлен в группу {target_chat} через userbot.")
            else:
                print("Не удалось отправить через userbot, пробуем Bot API.")
        
        # Отправляем файл через Bot API (личные чаты или fallback)
        if not sent_via_userbot:
            file_data = io.BytesIO(result_bytes)
            await context.bot.send_document(
                chat_id=target_chat,
                document=InputFile(file_data, filename=file_name),
                caption=caption
            )

        # Отправляем превью в тот же чат, откуда пришло фото (просмотр в Telegram)
        preview_file = io.BytesIO(preview_png)
        preview_file.name = f"preview_{width}x{height}.png"
        await context.bot.send_photo(
            chat_id=chat_id,
            photo=InputFile(preview_file, filename=preview_file.name),
            caption=f"Превью {width}x{height}"
        )
        
        # Отправляем подтверждение отправителю
        if target_chat != chat_id:
            await update.message.reply_text(
                f"✅ Изображение обработано и автоматически отправлено в личный чат с ESP8266 ботом (chat_id: {target_chat})!"
            )
        else:
            await update.message.reply_text(
                "✅ Изображение обработано! Оно уже отправлено в этот чат и будет отображено на устройстве."
            )
        
    except Exception as e:
        await update.message.reply_text(f"❌ Ошибка обработки: {str(e)}")
        print(f"Ошибка: {e}", flush=True)

async def handle_text(update: Update, context: ContextTypes.DEFAULT_TYPE):
    """Обработчик текстовых сообщений (для тестирования)"""
    text = update.message.text
    
    if text.startswith("/start"):
        target_info = f"в чат {TARGET_CHAT_ID}" if TARGET_CHAT_ID else "в этот же чат"
        await update.message.reply_text(
            f"Привет! Отправьте мне фото, и я автоматически:\n"
            f"1. Обработаю его для E-Paper дисплея\n"
            f"2. Отправлю результат {target_info}\n\n"
            f"Просто отправьте фото - всё остальное произойдет автоматически!"
        )
    elif text.startswith("/help"):
        target_info = f"в настроенный чат (ID: {TARGET_CHAT_ID})" if TARGET_CHAT_ID else "в этот же чат"
        await update.message.reply_text(
            "Этот бот автоматически обрабатывает изображения для E-Paper дисплея.\n\n"
            "Просто отправьте фото, и бот:\n"
            "1. Изменит размер до 400x300\n"
            "2. Конвертирует в монохромное\n"
            "3. Применит дизеринг для лучшего качества\n"
            f"4. Автоматически отправит результат {target_info}\n\n"
            "ESP8266 устройство автоматически распознает и отобразит изображение!"
        )
    elif text.startswith("/chatid"):
        # Команда для получения chat_id текущего чата
        await update.message.reply_text(
            f"ID этого чата: {update.effective_chat.id}\n\n"
            f"Используйте это значение для настройки TARGET_CHAT_ID в коде бота."
        )

async def post_init(application):
    await init_userbot()

async def post_shutdown(application):
    await shutdown_userbot()

def main():
    """Запуск бота"""
    print("Запуск Telegram бота для обработки изображений...")
    if not BOT_TOKEN:
        raise RuntimeError("Не задан BOT_TOKEN. Укажите его в переменных окружения.")
    print(f"Загруженная конфигурация: {safe_config_log()}")
    
    # Создаем приложение
    application = (
        Application.builder()
        .token(BOT_TOKEN)
        .post_init(post_init)
        .post_shutdown(post_shutdown)
        .build()
    )
    
    # Регистрируем обработчики
    application.add_handler(MessageHandler(filters.PHOTO, handle_photo))
    application.add_handler(MessageHandler(filters.TEXT & ~filters.COMMAND, handle_text))
    application.add_handler(MessageHandler(filters.COMMAND, handle_text))
    
    # Запускаем бота
    print("Бот запущен. Нажмите Ctrl+C для остановки.")
    application.run_polling(allowed_updates=Update.ALL_TYPES)

if __name__ == "__main__":
    main()
