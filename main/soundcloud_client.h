#ifndef SOUNDCLOUD_CLIENT_H
#define SOUNDCLOUD_CLIENT_H

#include <string>
#include <vector>
#include <functional>
#include <esp_http_client.h>

struct SoundCloudTrack {
    std::string id;
    std::string title;
    std::string artist;
    std::string duration;
    std::string artwork_url;
    std::string stream_url;
};

class SoundCloudClient {
public:
    SoundCloudClient();
    ~SoundCloudClient();
    
    // Khởi tạo với PHP API server URL
    bool Initialize(const std::string& api_server_url);
    
    // Tìm kiếm tracks trên SoundCloud
    // query: từ khóa tìm kiếm
    // callback: hàm gọi khi có kết quả
    bool SearchTracks(const std::string& query, 
                      std::function<void(const std::vector<SoundCloudTrack>&)> callback);
    
    // Lấy URL download/stream cho track
    bool GetDownloadUrl(const std::string& track_url, std::string& download_url);
    
    // Stream audio trực tiếp từ URL
    // download_url: URL lấy từ GetDownloadUrl
    // on_data: callback nhận data chunk (PCM 16-bit, 16kHz mono)
    bool StreamAudio(const std::string& download_url,
                     std::function<void(const uint8_t* data, size_t len)> on_data);
    
    // Dừng streaming hiện tại
    void StopStreaming();
    
    // Kiểm tra trạng thái
    bool IsStreaming() const { return is_streaming_; }
    
    // Lấy thông tin track hiện tại
    const SoundCloudTrack& GetCurrentTrack() const { return current_track_; }

private:
    std::string api_server_url_;
    SoundCloudTrack current_track_;
    bool is_streaming_;
    esp_http_client_handle_t http_client_;
    
    // HTTP event handler
    static esp_err_t HttpEventHandler(esp_http_client_event_t *evt);
    
    // Parse JSON response từ PHP API
    bool ParseSearchResponse(const std::string& json, 
                            std::vector<SoundCloudTrack>& tracks);
    bool ParseDownloadResponse(const std::string& json, std::string& url);
};

#endif // SOUNDCLOUD_CLIENT_H
