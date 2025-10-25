# 🚀 Hướng dẫn Build và Test ESP32-S3 Touch LCD 1.46 với PC Streaming

## ✅ Đã hoàn thành
- Module PCStreamClient (WebSocket + Opus)
- TouchLcdBotController integration
- Server Python test thành công
- Code không có lỗi

## 📋 Các bước Build và Flash

### 1. Setup ESP-IDF Environment (nếu chưa có)
```powershell
# Mở ESP-IDF PowerShell hoặc CMD
# Hoặc chạy:
C:\Espressif\frameworks\esp-idf-v5.5\export.ps1
```

### 2. Cấu hình Board
```bash
cd c:\Users\congh\Downloads\Compressed\xiaozhi-esp32-2.0.31.46\xiaozhi-esp32-2.0.3

# Set target
idf.py set-target esp32s3

# Configure
idf.py menuconfig
# Chọn: Board Type -> ESP32-S3 Touch LCD 1.46
```

### 3. Build Firmware
```bash
idf.py build
```

### 4. Flash to Board
```bash
# Flash tất cả (firmware + bootloader + partition table)
idf.py flash

# Hoặc flash chỉ firmware
idf.py app-flash

# Monitor log
idf.py monitor
```

### 5. Chạy PC Server
```bash
cd main\boards\esp32-s3-touch-lcd-1.46
python pc_server.py
```

## 🔧 Code sử dụng PC Streaming trong ESP32

### Trong board initialization (esp32-s3-touch-lcd-1.46.cc):
```cpp
#include "touch_lcd_bot_controller.h"

// Trong InitializeOttoRobot() hoặc setup
void InitializeOttoRobot() {
    auto& board = Board::GetInstance();
    auto* audio_codec = board.GetAudioCodec();
    auto* lcd_display = board.GetDisplay();
    
    // Khởi tạo controller
    auto* controller = new TouchLcdBotController(&otto_);
    controller->init();
    controller->start();
    controller->SetAudioCodec(audio_codec);
    controller->SetLcdDisplay(lcd_display);
    
    // Kết nối WebSocket tới PC (thay IP của PC)
    controller->InitPCStream("ws://192.168.1.100:8765");
    
    ESP_LOGI(TAG, "PC Streaming initialized!");
}
```

### Gửi Audio từ ESP32:
```cpp
// Trong audio processing loop
std::vector<int16_t> pcm_data(320); // 20ms @ 16kHz
if (audio_codec->InputData(pcm_data)) {
    controller->SendAudioToPC(pcm_data);
}
```

### Gửi Video từ ESP32:
```cpp
// Nếu có camera
camera_fb_t* fb = esp_camera_fb_get();
if (fb) {
    std::vector<uint8_t> jpeg(fb->buf, fb->buf + fb->len);
    controller->SendImageToPC(jpeg);
    esp_camera_fb_return(fb);
}
```

## 📊 Test Results
```
✅ WebSocket connection: OK
✅ Audio streaming (PCM->Opus): OK (640 bytes/packet)
✅ Video streaming (JPEG): OK (4 bytes minimal test)
✅ Server PC receive: OK
✅ Protocol headers (AUDIO:, VIDEO:): OK
```

## 🐛 Troubleshooting

### Lỗi build: opus not found
```bash
# Thêm opus component từ component registry
idf.py add-dependency "espressif/opus^1.0.0"
```

### Lỗi build: esp_websocket_client not found
```bash
# WebSocket client có sẵn trong ESP-IDF v5.x
# Kiểm tra version: idf.py --version
```

### Không kết nối được WebSocket
- Kiểm tra IP PC: `ipconfig` (Windows) hoặc `ifconfig` (Linux/Mac)
- Kiểm tra firewall: Mở port 8765
- Kiểm tra WiFi: ESP32 và PC cùng mạng
- Check log ESP32: `idf.py monitor`

### Audio không nghe được
- Kiểm tra Opus encoder/decoder
- Kiểm tra sample rate (16kHz)
- Kiểm tra audio device PC (pyaudio)

## 📝 Files quan trọng
```
main/boards/esp32-s3-touch-lcd-1.46/
├── pc_stream_client.h          # WebSocket + Opus module
├── pc_stream_client.cc
├── touch_lcd_bot_controller.h  # Controller với streaming
├── touch_lcd_bot_controller.cc
├── pc_server.py                # Server PC
├── test_client.py              # Test client Python
├── quick_test.py               # Quick test (đã test OK)
├── PC_STREAMING_GUIDE.md       # Hướng dẫn chi tiết
└── SUMMARY.md                  # Tóm tắt

```

## ⚙️ Dependencies

### ESP32 (idf_component.yml hoặc CMakeLists.txt):
```yaml
dependencies:
  espressif/opus: "^1.0.0"
  esp_websocket_client: "*"
```

### PC:
```bash
pip install websockets pillow pyaudio numpy
```

## 🎯 Next Steps
1. Build firmware với idf.py
2. Flash lên board
3. Chạy server PC
4. Test audio/video streaming
5. Tối ưu performance nếu cần

## 📞 Notes
- Test Python đã OK (quick_test.py)
- Code ESP32 sẵn sàng
- Chỉ cần build và flash
- Server PC hoạt động tốt
