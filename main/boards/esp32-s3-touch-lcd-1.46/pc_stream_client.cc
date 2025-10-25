#include "pc_stream_client.h"
#include <esp_log.h>
#include <string.h>
#include <esp_websocket_client.h>
#include <opus.h>

#define TAG "PCStreamClient"
#define OPUS_FRAME_SIZE 320  // 20ms @ 16kHz
#define OPUS_MAX_PACKET 4000

PCStreamClient::PCStreamClient() {
    // Khởi tạo Opus decoder
    int err;
    opus_decoder_ = opus_decoder_create(opus_sample_rate_, opus_channels_, &err);
    if (err != OPUS_OK) {
        ESP_LOGE(TAG, "Failed to create Opus decoder: %d", err);
    }
    
    // Khởi tạo Opus encoder
    opus_encoder_ = opus_encoder_create(opus_sample_rate_, opus_channels_, OPUS_APPLICATION_VOIP, &err);
    if (err != OPUS_OK) {
        ESP_LOGE(TAG, "Failed to create Opus encoder: %d", err);
    } else {
        opus_encoder_ctl(opus_encoder_, OPUS_SET_BITRATE(opus_bitrate_));
    }
}

PCStreamClient::~PCStreamClient() {
    Disconnect();
    if (opus_decoder_) {
        opus_decoder_destroy(opus_decoder_);
        opus_decoder_ = nullptr;
    }
    if (opus_encoder_) {
        opus_encoder_destroy(opus_encoder_);
        opus_encoder_ = nullptr;
    }
}

bool PCStreamClient::Connect(const std::string& url) {
    if (ws_client_) {
        ESP_LOGW(TAG, "Already connected");
        return true;
    }

    esp_websocket_client_config_t ws_cfg = {};
    ws_cfg.uri = url.c_str();
    ws_cfg.reconnect_timeout_ms = 5000;
    ws_cfg.network_timeout_ms = 10000;
    
    ws_client_ = esp_websocket_client_init(&ws_cfg);
    if (!ws_client_) {
        ESP_LOGE(TAG, "Failed to initialize WebSocket client");
        return false;
    }

    esp_websocket_register_events(ws_client_, WEBSOCKET_EVENT_ANY, 
                                  WebSocketEventHandler, this);
    
    esp_err_t ret = esp_websocket_client_start(ws_client_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WebSocket client: %d", ret);
        esp_websocket_client_destroy(ws_client_);
        ws_client_ = nullptr;
        return false;
    }

    ESP_LOGI(TAG, "WebSocket connecting to %s", url.c_str());
    return true;
}

void PCStreamClient::Disconnect() {
    if (ws_client_) {
        esp_websocket_client_stop(ws_client_);
        esp_websocket_client_destroy(ws_client_);
        ws_client_ = nullptr;
        ESP_LOGI(TAG, "WebSocket disconnected");
    }
}

bool PCStreamClient::IsConnected() const {
    return ws_client_ && esp_websocket_client_is_connected(ws_client_);
}

bool PCStreamClient::SendAudioPCM(const std::vector<int16_t>& pcm_data) {
    if (!IsConnected() || !opus_encoder_) {
        return false;
    }

    // Encode PCM to Opus
    std::vector<uint8_t> opus_packet(OPUS_MAX_PACKET);
    int encoded_bytes = opus_encode(opus_encoder_, pcm_data.data(), 
                                    pcm_data.size(), opus_packet.data(), 
                                    opus_packet.size());
    
    if (encoded_bytes < 0) {
        ESP_LOGE(TAG, "Opus encode failed: %d", encoded_bytes);
        return false;
    }

    // Gửi header "AUDIO:" + Opus data
    std::vector<uint8_t> packet;
    const char* header = "AUDIO:";
    packet.insert(packet.end(), header, header + 6);
    packet.insert(packet.end(), opus_packet.begin(), opus_packet.begin() + encoded_bytes);

    int ret = esp_websocket_client_send_bin(ws_client_, (const char*)packet.data(), 
                                            packet.size(), portMAX_DELAY);
    return ret > 0;
}

void PCStreamClient::OnAudioReceived(std::function<void(const std::vector<int16_t>& pcm_data)> cb) {
    audio_cb_ = cb;
}

bool PCStreamClient::SendImageJPEG(const std::vector<uint8_t>& jpeg_data) {
    if (!IsConnected()) {
        return false;
    }

    // Gửi header "VIDEO:" + JPEG data
    std::vector<uint8_t> packet;
    const char* header = "VIDEO:";
    packet.insert(packet.end(), header, header + 6);
    packet.insert(packet.end(), jpeg_data.begin(), jpeg_data.end());

    int ret = esp_websocket_client_send_bin(ws_client_, (const char*)packet.data(), 
                                            packet.size(), portMAX_DELAY);
    return ret > 0;
}

void PCStreamClient::OnImageReceived(std::function<void(const std::vector<uint8_t>& jpeg_data)> cb) {
    image_cb_ = cb;
}

void PCStreamClient::WebSocketEventHandler(void* handler_args, esp_event_base_t base, 
                                          int32_t event_id, void* event_data) {
    auto* self = static_cast<PCStreamClient*>(handler_args);
    auto* data = static_cast<esp_websocket_event_data_t*>(event_data);
    
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WebSocket connected");
            break;
            
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WebSocket disconnected");
            break;
            
        case WEBSOCKET_EVENT_DATA:
            if (data->data_len > 0) {
                self->HandleWebSocketData((const uint8_t*)data->data_ptr, data->data_len);
            }
            break;
            
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WebSocket error");
            break;
            
        default:
            break;
    }
}

void PCStreamClient::HandleWebSocketData(const uint8_t* data, int len) {
    if (len < 6) return;

    // Kiểm tra header
    if (memcmp(data, "AUDIO:", 6) == 0) {
        // Nhận audio: Opus -> PCM
        if (audio_cb_ && opus_decoder_) {
            std::vector<int16_t> pcm(OPUS_FRAME_SIZE * 6); // Max frame size
            int frame_size = opus_decode(opus_decoder_, data + 6, len - 6, 
                                        pcm.data(), pcm.size(), 0);
            if (frame_size > 0) {
                pcm.resize(frame_size);
                audio_cb_(pcm);
            }
        }
    } else if (memcmp(data, "VIDEO:", 6) == 0) {
        // Nhận hình ảnh JPEG
        if (image_cb_) {
            std::vector<uint8_t> jpeg(data + 6, data + len);
            image_cb_(jpeg);
        }
    }
}
