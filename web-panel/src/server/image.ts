import sharp from "sharp";
import { config } from "./config";

export async function processImageToRaw(imageBuffer: Buffer): Promise<{
  epd: Buffer;
  width: number;
  height: number;
  previewPng: Buffer;
}> {
  // Keep the same sizing logic as telegram_bot_processor.py
  const headerReserve = 64;
  const maxRawBytes = Math.max(0, config.webMaxOutputBytes - headerReserve);
  const maxPixels = maxRawBytes * 8;
  const fullTargetPixels = config.webTargetWidth * config.webTargetHeight;

  let targetW = config.webTargetWidth;
  let targetH = config.webTargetHeight;
  if (maxPixels > 0 && maxPixels < fullTargetPixels) {
    const scale = Math.sqrt(maxPixels / fullTargetPixels);
    targetW = Math.max(1, Math.floor(config.webTargetWidth * scale));
    targetH = Math.max(1, Math.floor(config.webTargetHeight * scale));
  }

  const { data, info } = await sharp(imageBuffer)
    .resize(targetW, targetH, {
      fit: "contain",
      background: { r: 255, g: 255, b: 255, alpha: 1 },
      kernel: sharp.kernel.lanczos3
    })
    .grayscale()
    .raw()
    .toBuffer({ resolveWithObject: true });

  const width = info.width;
  const height = info.height;
  const channels = info.channels;
  const gray = new Float32Array(width * height);
  for (let i = 0; i < width * height; i++) {
    gray[i] = data[i * channels];
  }

  // Floyd-Steinberg dithering to match Python path quality.
  for (let y = 0; y < height; y++) {
    for (let x = 0; x < width; x++) {
      const i = y * width + x;
      const oldPixel = gray[i];
      const newPixel = oldPixel < 128 ? 0 : 255;
      const err = oldPixel - newPixel;
      gray[i] = newPixel;

      if (x + 1 < width) gray[i + 1] += (err * 7) / 16;
      if (y + 1 < height) {
        if (x > 0) gray[i + width - 1] += (err * 3) / 16;
        gray[i + width] += (err * 5) / 16;
        if (x + 1 < width) gray[i + width + 1] += err / 16;
      }
    }
  }

  const bitmap = new Uint8Array(Math.ceil(width / 8) * height);
  const monoPixels = new Uint8Array(width * height);
  let bi = 0;
  let pi = 0;

  for (let y = 0; y < height; y++) {
    let byte = 0;
    let bitPos = 7;
    for (let x = 0; x < width; x++) {
      const value = gray[y * width + x] < 128 ? 0 : 255;
      const isBlack = value === 0;
      if (isBlack) byte |= 1 << bitPos;
      monoPixels[pi++] = value;
      bitPos--;
      if (bitPos < 0) {
        bitmap[bi++] = byte;
        byte = 0;
        bitPos = 7;
      }
    }
    if (bitPos !== 7) bitmap[bi++] = byte;
  }

  const header = Buffer.from(`RAW|${width}|${height}|`, "ascii");
  const epd = Buffer.concat([header, Buffer.from(bitmap)]);
  if (config.webMaxOutputBytes > 0 && epd.length > config.webMaxOutputBytes) {
    throw new Error("Не удалось уложить изображение в лимит WEB_MAX_OUTPUT_BYTES.");
  }

  const previewPng = await sharp(Buffer.from(monoPixels), {
    raw: { width, height, channels: 1 }
  })
    .png()
    .toBuffer();

  return { epd, width, height, previewPng };
}
