#include "soundcloud_client.h"
#include <esp_log.h>
#include <cJSON.h>
#include <cstring>

#define TAG "SoundCloudClient"
#define MAX_HTTP_RECV_BUFFER 4096

SoundCloudClient::SoundCloudClient() 
    : is_streaming_(false), http_client_(nullptr) {
}

SoundCloudClient::~SoundCloudClient() {
    StopStreaming();
}

bool SoundCloudClient::Initialize(const std::string& api_server_url) {
    api_server_url_ = api_server_url;
    ESP_LOGI(TAG, "Initialized with API server: %s", api_server_url_.c_str());
    return true;
}

bool SoundCloudClient::SearchTracks(const std::string& query, 
                                    std::function<void(const std::vector<SoundCloudTrack>&)> callback) {
    if (api_server_url_.empty()) {
        ESP_LOGE(TAG, "API server URL not set");
        return false;
    }
    
    // Tạo URL request: http://server/api.php?action=search&query=...
    std::string url = api_server_url_ + "/api.php?action=search&query=" + query;
    
    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 10000;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return false;
    }
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d", status, content_length);
        
        if (status == 200 && content_length > 0) {
            char* buffer = (char*)malloc(content_length + 1);
            if (buffer) {
                int read_len = esp_http_client_read(client, buffer, content_length);
                buffer[read_len] = '\0';
                
                // Parse JSON response
                std::vector<SoundCloudTrack> tracks;
                if (ParseSearchResponse(buffer, tracks)) {
                    callback(tracks);
                }
                
                free(buffer);
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err == ESP_OK;
}

bool SoundCloudClient::GetDownloadUrl(const std::string& track_url, std::string& download_url) {
    if (api_server_url_.empty()) {
        ESP_LOGE(TAG, "API server URL not set");
        return false;
    }
    
    // Request: http://server/api.php?action=download&url=...
    std::string url = api_server_url_ + "/api.php?action=download&url=" + track_url;
    
    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.timeout_ms = 15000;
    
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return false;
    }
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int content_length = esp_http_client_get_content_length(client);
        
        if (status == 200 && content_length > 0) {
            char* buffer = (char*)malloc(content_length + 1);
            if (buffer) {
                int read_len = esp_http_client_read(client, buffer, content_length);
                buffer[read_len] = '\0';
                
                // Parse JSON response
                if (ParseDownloadResponse(buffer, download_url)) {
                    ESP_LOGI(TAG, "Got download URL: %s", download_url.c_str());
                }
                
                free(buffer);
            }
        }
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return err == ESP_OK && !download_url.empty();
}

esp_err_t SoundCloudClient::HttpEventHandler(esp_http_client_event_t *evt) {
    // SoundCloudClient* client = (SoundCloudClient*)evt->user_data;
    
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // Nhận audio data chunk
                ESP_LOGD(TAG, "Received %d bytes", evt->data_len);
                // TODO: Decode và gửi tới AudioCodec
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

bool SoundCloudClient::StreamAudio(const std::string& download_url,
                                   std::function<void(const uint8_t* data, size_t len)> on_data) {
    if (is_streaming_) {
        ESP_LOGW(TAG, "Already streaming");
        return false;
    }
    
    esp_http_client_config_t config = {};
    config.url = download_url.c_str();
    config.method = HTTP_METHOD_GET;
    config.event_handler = HttpEventHandler;
    config.user_data = this;
    config.buffer_size = MAX_HTTP_RECV_BUFFER;
    
    http_client_ = esp_http_client_init(&config);
    if (!http_client_) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client for streaming");
        return false;
    }
    
    is_streaming_ = true;
    
    esp_err_t err = esp_http_client_open(http_client_, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        esp_http_client_cleanup(http_client_);
        http_client_ = nullptr;
        is_streaming_ = false;
        return false;
    }
    
    int content_length = esp_http_client_fetch_headers(http_client_);
    ESP_LOGI(TAG, "Streaming audio, content_length = %d", content_length);
    
    // Read và stream data chunks
    char buffer[MAX_HTTP_RECV_BUFFER];
    while (is_streaming_) {
        int read_len = esp_http_client_read(http_client_, buffer, sizeof(buffer));
        if (read_len > 0) {
            on_data((uint8_t*)buffer, read_len);
        } else if (read_len == 0) {
            ESP_LOGI(TAG, "Stream finished");
            break;
        } else {
            ESP_LOGE(TAG, "Stream error");
            break;
        }
    }
    
    esp_http_client_close(http_client_);
    esp_http_client_cleanup(http_client_);
    http_client_ = nullptr;
    is_streaming_ = false;
    
    return true;
}

void SoundCloudClient::StopStreaming() {
    if (is_streaming_) {
        is_streaming_ = false;
        if (http_client_) {
            esp_http_client_close(http_client_);
            esp_http_client_cleanup(http_client_);
            http_client_ = nullptr;
        }
        ESP_LOGI(TAG, "Streaming stopped");
    }
}

bool SoundCloudClient::ParseSearchResponse(const std::string& json, 
                                           std::vector<SoundCloudTrack>& tracks) {
    cJSON* root = cJSON_Parse(json.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return false;
    }
    
    cJSON* success = cJSON_GetObjectItem(root, "success");
    if (!cJSON_IsTrue(success)) {
        ESP_LOGE(TAG, "API returned error");
        cJSON_Delete(root);
        return false;
    }
    
    cJSON* data = cJSON_GetObjectItem(root, "data");
    if (!cJSON_IsArray(data)) {
        ESP_LOGE(TAG, "No tracks in response");
        cJSON_Delete(root);
        return false;
    }
    
    cJSON* track = nullptr;
    cJSON_ArrayForEach(track, data) {
        SoundCloudTrack t;
        
        cJSON* id = cJSON_GetObjectItem(track, "id");
        if (cJSON_IsString(id)) t.id = id->valuestring;
        
        cJSON* title = cJSON_GetObjectItem(track, "title");
        if (cJSON_IsString(title)) t.title = title->valuestring;
        
        cJSON* artist = cJSON_GetObjectItem(track, "artist");
        if (cJSON_IsString(artist)) t.artist = artist->valuestring;
        
        cJSON* duration = cJSON_GetObjectItem(track, "duration");
        if (cJSON_IsString(duration)) t.duration = duration->valuestring;
        
        cJSON* artwork = cJSON_GetObjectItem(track, "artwork_url");
        if (cJSON_IsString(artwork)) t.artwork_url = artwork->valuestring;
        
        cJSON* stream = cJSON_GetObjectItem(track, "stream_url");
        if (cJSON_IsString(stream)) t.stream_url = stream->valuestring;
        
        tracks.push_back(t);
        ESP_LOGI(TAG, "Track: %s - %s", t.artist.c_str(), t.title.c_str());
    }
    
    cJSON_Delete(root);
    return !tracks.empty();
}

bool SoundCloudClient::ParseDownloadResponse(const std::string& json, std::string& url) {
    cJSON* root = cJSON_Parse(json.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return false;
    }
    
    cJSON* success = cJSON_GetObjectItem(root, "success");
    if (!cJSON_IsTrue(success)) {
        ESP_LOGE(TAG, "API returned error");
        cJSON_Delete(root);
        return false;
    }
    
    cJSON* download_url = cJSON_GetObjectItem(root, "download_url");
    if (cJSON_IsString(download_url)) {
        url = download_url->valuestring;
    }
    
    cJSON_Delete(root);
    return !url.empty();
}
