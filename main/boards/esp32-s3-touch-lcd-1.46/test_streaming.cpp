/**
 * Test file for PC Streaming functionality
 * Compile this to test WebSocket audio/video streaming
 */

#include "touch_lcd_bot_controller.h"
#include "../../application.h"
#include "../../boards/common/board.h"
#include <esp_log.h>

#define TEST_TAG "StreamTest"

// Test configuration
const char* WS_SERVER_URL = "ws://192.168.1.100:8765";  // Thay bằng IP PC của bạn

void test_streaming_initialization() {
    ESP_LOGI(TEST_TAG, "=== Test 1: Streaming Initialization ===");
    
    auto& board = Board::GetInstance();
    auto* audio_codec = board.GetAudioCodec();
    auto* lcd_display = board.GetDisplay();
    
    // Tạo controller (giả sử có otto object)
    Otto* otto = nullptr; // TODO: Get actual otto instance
    TouchLcdBotController controller(otto);
    
    controller.SetAudioCodec(audio_codec);
    controller.SetLcdDisplay(lcd_display);
    
    ESP_LOGI(TEST_TAG, "✅ Controller initialized");
    
    // Kết nối WebSocket
    controller.InitPCStream(WS_SERVER_URL);
    ESP_LOGI(TEST_TAG, "✅ WebSocket connection initiated");
}

void test_send_audio() {
    ESP_LOGI(TEST_TAG, "=== Test 2: Send Audio to PC ===");
    
    auto& board = Board::GetInstance();
    auto* audio_codec = board.GetAudioCodec();
    
    Otto* otto = nullptr;
    TouchLcdBotController controller(otto);
    controller.SetAudioCodec(audio_codec);
    controller.InitPCStream(WS_SERVER_URL);
    
    // Đợi kết nối
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Gửi test audio (sine wave 1kHz)
    std::vector<int16_t> test_audio(320); // 20ms @ 16kHz
    for (int i = 0; i < 320; i++) {
        test_audio[i] = (int16_t)(32767 * sin(2 * M_PI * 1000 * i / 16000));
    }
    
    ESP_LOGI(TEST_TAG, "Sending test audio (1kHz sine wave)...");
    controller.SendAudioToPC(test_audio);
    ESP_LOGI(TEST_TAG, "✅ Audio sent");
}

void test_send_video() {
    ESP_LOGI(TEST_TAG, "=== Test 3: Send Video Frame to PC ===");
    
    Otto* otto = nullptr;
    TouchLcdBotController controller(otto);
    controller.InitPCStream(WS_SERVER_URL);
    
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Tạo fake JPEG header (test only)
    std::vector<uint8_t> fake_jpeg = {
        0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46,
        0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
        0x00, 0x01, 0x00, 0x00, 0xFF, 0xD9  // Minimal JPEG
    };
    
    ESP_LOGI(TEST_TAG, "Sending test JPEG frame...");
    controller.SendImageToPC(fake_jpeg);
    ESP_LOGI(TEST_TAG, "✅ Video frame sent");
}

void test_receive_callbacks() {
    ESP_LOGI(TEST_TAG, "=== Test 4: Receive Callbacks ===");
    
    auto& board = Board::GetInstance();
    auto* audio_codec = board.GetAudioCodec();
    
    Otto* otto = nullptr;
    TouchLcdBotController controller(otto);
    controller.SetAudioCodec(audio_codec);
    controller.InitPCStream(WS_SERVER_URL);
    
    ESP_LOGI(TEST_TAG, "Waiting for data from PC (30 seconds)...");
    ESP_LOGI(TEST_TAG, "Start pc_server.py on PC and send audio/video");
    
    // Đợi 30 giây để nhận dữ liệu
    for (int i = 0; i < 30; i++) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ESP_LOGI(TEST_TAG, "Waiting... %d/30", i + 1);
    }
    
    ESP_LOGI(TEST_TAG, "✅ Callback test completed");
}

// Main test entry point
extern "C" void app_main_test_streaming() {
    ESP_LOGI(TEST_TAG, "╔═══════════════════════════════════════════════════╗");
    ESP_LOGI(TEST_TAG, "║  ESP32-S3 Touch LCD 1.46 - PC Streaming Test     ║");
    ESP_LOGI(TEST_TAG, "╚═══════════════════════════════════════════════════╝");
    
    // Đợi WiFi kết nối
    ESP_LOGI(TEST_TAG, "Waiting for WiFi connection...");
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    // Chạy các test
    test_streaming_initialization();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    test_send_audio();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    test_send_video();
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    test_receive_callbacks();
    
    ESP_LOGI(TEST_TAG, "╔═══════════════════════════════════════════════════╗");
    ESP_LOGI(TEST_TAG, "║           All Tests Completed!                    ║");
    ESP_LOGI(TEST_TAG, "╚═══════════════════════════════════════════════════╝");
}
