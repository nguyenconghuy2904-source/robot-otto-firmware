# Hướng dẫn Streaming Audio/Video giữa PC và ESP32-S3 Touch LCD 1.46

## 📋 Tổng quan
Board ESP32-S3 Touch LCD 1.46 hiện đã hỗ trợ:
- ✅ Truyền âm thanh hai chiều (PCM ↔ Opus ↔ WebSocket)
- ✅ Truyền hình ảnh hai chiều (JPEG ↔ WebSocket)
- ✅ Giao thức WebSocket với header phân loại (`AUDIO:`, `VIDEO:`)

## 🔧 Cấu hình

### 1. Khởi tạo trong code ESP32
```cpp
#include "touch_lcd_bot_controller.h"

// Trong hàm main hoặc board initialization
TouchLcdBotController* controller = new TouchLcdBotController(otto);
controller->init();
controller->start();

// Set audio codec và LCD display
controller->SetAudioCodec(audio_codec);
controller->SetLcdDisplay(lcd_display);

// Kết nối tới PC WebSocket server
controller->InitPCStream("ws://192.168.1.100:8765");
```

### 2. Gửi audio từ ESP32 lên PC
```cpp
// Lấy audio từ microphone (PCM 16kHz mono)
std::vector<int16_t> pcm_data;
audio_codec->InputData(pcm_data);

// Gửi lên PC (tự động encode Opus)
controller->SendAudioToPC(pcm_data);
```

### 3. Gửi hình ảnh từ ESP32 lên PC
```cpp
// Lấy JPEG từ camera
camera_fb_t* fb = esp_camera_fb_get();
std::vector<uint8_t> jpeg_data(fb->buf, fb->buf + fb->len);

// Gửi lên PC
controller->SendImageToPC(jpeg_data);
esp_camera_fb_return(fb);
```

### 4. Nhận audio/video từ PC
Audio và video từ PC sẽ tự động được xử lý:
- **Audio**: Tự động decode Opus → PCM → phát ra loa
- **Video**: Callback nhận JPEG (cần bổ sung JPEG decoder để hiển thị lên LCD)

## 🖥️ Server PC (Python WebSocket)

### Cài đặt
```bash
pip install websockets pillow pyaudio numpy
```

### Code mẫu server PC
```python
import asyncio
import websockets
import pyaudio
import numpy as np

CHUNK = 320  # 20ms @ 16kHz
FORMAT = pyaudio.paInt16
CHANNELS = 1
RATE = 16000

audio = pyaudio.PyAudio()
stream = audio.open(format=FORMAT, channels=CHANNELS,
                   rate=RATE, input=True, output=True,
                   frames_per_buffer=CHUNK)

async def handle_client(websocket, path):
    print("ESP32 connected!")
    try:
        async for message in websocket:
            if isinstance(message, bytes):
                if message[:6] == b'AUDIO:':
                    # Nhận audio từ ESP32, phát ra loa PC
                    opus_data = message[6:]
                    # TODO: Decode Opus và phát
                    print(f"Received audio: {len(opus_data)} bytes")
                    
                elif message[:6] == b'VIDEO:':
                    # Nhận JPEG từ ESP32, hiển thị trên PC
                    jpeg_data = message[6:]
                    print(f"Received video frame: {len(jpeg_data)} bytes")
                    # TODO: Decode JPEG và hiển thị
                    
    except websockets.exceptions.ConnectionClosed:
        print("ESP32 disconnected")

async def main():
    async with websockets.serve(handle_client, "0.0.0.0", 8765):
        print("WebSocket server running on ws://0.0.0.0:8765")
        await asyncio.Future()  # Run forever

asyncio.run(main())
```

## 📡 Giao thức WebSocket

### Format gói tin
- **Audio từ ESP32 → PC**: `"AUDIO:" + [Opus encoded data]`
- **Video từ ESP32 → PC**: `"VIDEO:" + [JPEG data]`
- **Audio từ PC → ESP32**: `"AUDIO:" + [Opus encoded data]`
- **Video từ PC → ESP32**: `"VIDEO:" + [JPEG data]`

### Cấu hình Opus
- Sample rate: 16kHz
- Channels: Mono (1)
- Bitrate: 24kbps
- Frame size: 320 samples (20ms)

## 🎯 Ví dụ sử dụng đầy đủ

### ESP32 Code
```cpp
void setup() {
    // Khởi tạo board, audio, camera, LCD
    Board& board = Board::GetInstance();
    auto* audio_codec = board.GetAudioCodec();
    auto* lcd = board.GetDisplay();
    
    // Khởi tạo controller
    auto* controller = new TouchLcdBotController(otto);
    controller->init();
    controller->start();
    controller->SetAudioCodec(audio_codec);
    controller->SetLcdDisplay(lcd);
    
    // Kết nối WebSocket
    controller->InitPCStream("ws://192.168.1.100:8765");
}

void loop() {
    // Gửi audio mỗi 20ms
    static uint32_t last_audio = 0;
    if (millis() - last_audio > 20) {
        std::vector<int16_t> pcm;
        if (audio_codec->InputData(pcm)) {
            controller->SendAudioToPC(pcm);
        }
        last_audio = millis();
    }
    
    // Gửi video mỗi 100ms (10 FPS)
    static uint32_t last_video = 0;
    if (millis() - last_video > 100) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (fb) {
            std::vector<uint8_t> jpeg(fb->buf, fb->buf + fb->len);
            controller->SendImageToPC(jpeg);
            esp_camera_fb_return(fb);
        }
        last_video = millis();
    }
}
```

## 🔍 Debug

### Kiểm tra kết nối
```cpp
if (pc_stream_client_.IsConnected()) {
    ESP_LOGI(TAG, "WebSocket connected to PC");
} else {
    ESP_LOGE(TAG, "WebSocket not connected");
}
```

### Bật log chi tiết
Trong `sdkconfig` hoặc `menuconfig`:
```
CONFIG_LOG_DEFAULT_LEVEL_DEBUG=y
```

## ⚠️ Lưu ý
1. **WiFi**: Đảm bảo ESP32 và PC cùng mạng WiFi.
2. **IP Server**: Sửa IP trong `InitPCStream()` thành IP của PC.
3. **Firewall**: Tắt firewall hoặc mở port 8765 trên PC.
4. **Opus Library**: Đảm bảo project đã link thư viện `opus` (thêm vào CMakeLists.txt).
5. **WebSocket Client**: Đảm bảo `esp_websocket_client` đã được enable trong ESP-IDF.

## 📦 Dependencies cần thêm vào CMakeLists.txt

```cmake
# main/boards/esp32-s3-touch-lcd-1.46/CMakeLists.txt
idf_component_register(
    SRCS "..."
    INCLUDE_DIRS "."
    REQUIRES 
        esp_websocket_client
        opus
        esp_camera  # nếu dùng camera
        # ... các component khác
)
```

## 🚀 Roadmap
- [ ] Bổ sung JPEG decoder để hiển thị video từ PC lên LCD
- [ ] Tối ưu bandwidth (giảm bitrate, frame rate)
- [ ] Hỗ trợ H.264 video streaming
- [ ] Audio echo cancellation (AEC)
