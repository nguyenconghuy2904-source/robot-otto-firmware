#include "wake_word_manager.h"
#include "settings.h"
#include <esp_log.h>
#include <algorithm>
#include <cctype>
#include <regex>

#define TAG "WakeWordManager"

bool WakeWordManager::DetectAddWakeWordCommand(const char* content, std::string& wake_word) {
    if (!content) return false;
    
    std::string text(content);
    
    // Chuyển về chữ thường để dễ so sánh
    std::transform(text.begin(), text.end(), text.begin(), ::tolower);
    
    // Các pattern phát hiện lệnh thêm wake word
    std::vector<std::regex> patterns = {
        std::regex(R"(thêm\s+từ\s+đánh\s+thức\s+(.+))"),
        std::regex(R"(them\s+tu\s+danh\s+thuc\s+(.+))"),
        std::regex(R"(add\s+wake\s+word\s+(.+))"),
        std::regex(R"(đặt\s+từ\s+kích\s+hoạt\s+(.+))"),
        std::regex(R"(dat\s+tu\s+kich\s+hoat\s+(.+))")
    };
    
    std::smatch matches;
    for (const auto& pattern : patterns) {
        if (std::regex_search(text, matches, pattern)) {
            if (matches.size() > 1) {
                wake_word = matches[1].str();
                
                // Loại bỏ khoảng trắng đầu cuối
                wake_word.erase(0, wake_word.find_first_not_of(" \t"));
                wake_word.erase(wake_word.find_last_not_of(" \t") + 1);
                
                // Loại bỏ dấu câu cuối
                while (!wake_word.empty() && 
                       (wake_word.back() == '.' || wake_word.back() == ',' || 
                        wake_word.back() == '!' || wake_word.back() == '?')) {
                    wake_word.pop_back();
                }
                
                if (!wake_word.empty()) {
                    ESP_LOGI(TAG, "Detected add wake word command: '%s'", wake_word.c_str());
                    return true;
                }
            }
        }
    }
    
    return false;
}

void WakeWordManager::AddWakeWord(const std::string& wake_word) {
    // Kiểm tra xem đã tồn tại chưa
    auto it = std::find(wake_words_.begin(), wake_words_.end(), wake_word);
    if (it == wake_words_.end()) {
        wake_words_.push_back(wake_word);
        SaveWakeWords();
        ESP_LOGI(TAG, "Added new wake word: '%s'", wake_word.c_str());
    } else {
        ESP_LOGI(TAG, "Wake word '%s' already exists", wake_word.c_str());
    }
    
    // Set làm wake word hiện tại
    SetCurrentWakeWord(wake_word);
}

std::vector<std::string> WakeWordManager::GetWakeWords() {
    LoadWakeWords();
    return wake_words_;
}

void WakeWordManager::SetCurrentWakeWord(const std::string& wake_word) {
    current_wake_word_ = wake_word;
    
    Settings settings("wake_word", true);
    settings.SetString("current", wake_word);
    
    ESP_LOGI(TAG, "Set current wake word to: '%s'", wake_word.c_str());
}

std::string WakeWordManager::GetCurrentWakeWord() {
    if (current_wake_word_.empty()) {
        Settings settings("wake_word");
        current_wake_word_ = settings.GetString("current", "xiaozhi");
    }
    return current_wake_word_;
}

void WakeWordManager::SaveWakeWords() {
    Settings settings("wake_word", true);
    
    // Lưu số lượng wake word
    settings.SetInt("count", wake_words_.size());
    
    // Lưu từng wake word với key: word_0, word_1, ...
    for (size_t i = 0; i < wake_words_.size(); i++) {
        std::string key = "word_" + std::to_string(i);
        settings.SetString(key, wake_words_[i]);
    }
    
    ESP_LOGI(TAG, "Saved %zu wake words to NVS", wake_words_.size());
}

void WakeWordManager::LoadWakeWords() {
    Settings settings("wake_word");
    
    int count = settings.GetInt("count", 0);
    wake_words_.clear();
    
    for (int i = 0; i < count; i++) {
        std::string key = "word_" + std::to_string(i);
        std::string word = settings.GetString(key, "");
        if (!word.empty()) {
            wake_words_.push_back(word);
        }
    }
    
    // Nếu chưa có wake word nào, thêm mặc định
    if (wake_words_.empty()) {
        wake_words_.push_back("xiaozhi");
        wake_words_.push_back("hello");
        SaveWakeWords();
    }
    
    ESP_LOGI(TAG, "Loaded %zu wake words from NVS", wake_words_.size());
}