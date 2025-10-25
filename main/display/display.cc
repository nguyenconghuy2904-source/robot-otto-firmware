#include <esp_log.h>
#include <esp_err.h>
#include <string>
#include <cstdlib>
#include <cstring>
#include <font_awesome.h>

#include "display.h"
#include "board.h"
#include "application.h"
#include "audio_codec.h"
#include "settings.h"
#include "assets/lang_config.h"
#include "wake_word_manager.h"

#define TAG "Display"

Display::Display() {
}

Display::~Display() {
}

void Display::SetStatus(const char* status) {
    ESP_LOGW(TAG, "SetStatus: %s", status);
}

void Display::ShowNotification(const std::string &notification, int duration_ms) {
    ShowNotification(notification.c_str(), duration_ms);
}

void Display::ShowNotification(const char* notification, int duration_ms) {
    ESP_LOGW(TAG, "ShowNotification: %s", notification);
}

void Display::UpdateStatusBar(bool update_all) {
}


void Display::SetEmotion(const char* emotion) {
    ESP_LOGW(TAG, "SetEmotion: %s", emotion);
}

void Display::SetChatMessage(const char* role, const char* content) {
    ESP_LOGW(TAG, "Role:%s", role);
    ESP_LOGW(TAG, "     %s", content);
    
    // Xử lý lệnh wake word nếu role là "assistant"
    if (role && content && strcmp(role, "assistant") == 0) {
        ProcessWakeWordCommand(content);
    }
}

void Display::ProcessWakeWordCommand(const char* content) {
    auto& wake_word_manager = WakeWordManager::GetInstance();
    std::string wake_word;
    
    if (wake_word_manager.DetectAddWakeWordCommand(content, wake_word)) {
        wake_word_manager.AddWakeWord(wake_word);
        
        // Hiển thị thông báo thành công
        std::string notification = "Đã thêm từ đánh thức: " + wake_word;
        ShowNotification(notification, 3000);
        
        ESP_LOGI(TAG, "Successfully added wake word: %s", wake_word.c_str());
    }
}

void Display::SetTheme(Theme* theme) {
    current_theme_ = theme;
    Settings settings("display", true);
    settings.SetString("theme", theme->name());
}

void Display::SetPowerSaveMode(bool on) {
    ESP_LOGW(TAG, "SetPowerSaveMode: %d", on);
}
