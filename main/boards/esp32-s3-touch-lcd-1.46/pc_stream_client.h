#pragma once

#include <string>
#include <vector>
#include <functional>
#include <esp_event.h>

// Forward declarations for Opus
struct OpusDecoder;
struct OpusEncoder;

// Forward declaration for WebSocket
typedef struct esp_websocket_client* esp_websocket_client_handle_t;

class PCStreamClient {
public:
    PCStreamClient();
    ~PCStreamClient();

    // Khởi tạo kết nối WebSocket tới PC
    bool Connect(const std::string& url);
    void Disconnect();
    bool IsConnected() const;

    // Gửi âm thanh (PCM -> Opus -> WebSocket)
    bool SendAudioPCM(const std::vector<int16_t>& pcm_data);
    // Nhận âm thanh (WebSocket -> Opus -> PCM callback)
    void OnAudioReceived(std::function<void(const std::vector<int16_t>& pcm_data)> cb);

    // Gửi hình ảnh (JPEG)
    bool SendImageJPEG(const std::vector<uint8_t>& jpeg_data);
    // Nhận hình ảnh (JPEG callback)
    void OnImageReceived(std::function<void(const std::vector<uint8_t>& jpeg_data)> cb);

private:
    esp_websocket_client_handle_t ws_client_ = nullptr;
    std::function<void(const std::vector<int16_t>&)> audio_cb_;
    std::function<void(const std::vector<uint8_t>&)> image_cb_;
    
    OpusDecoder* opus_decoder_ = nullptr;
    OpusEncoder* opus_encoder_ = nullptr;
    int opus_sample_rate_ = 16000;
    int opus_channels_ = 1;
    int opus_bitrate_ = 24000;
    
    static void WebSocketEventHandler(void* handler_args, esp_event_base_t base, 
                                      int32_t event_id, void* event_data);
    void HandleWebSocketData(const uint8_t* data, int len);
};
