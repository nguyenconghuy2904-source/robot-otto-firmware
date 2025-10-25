#include "otto_emoji_display.h"
#include "lvgl_theme.h"

#include <esp_log.h>
#include <font_awesome.h>

#include <algorithm>
#include <cstring>
#include <string>
#include <chrono>

#include "display/lcd_display.h"
#include "application.h"
#include "board.h"

#define TAG "OttoEmojiDisplay"

// 表情映射表 - 将原版21种表情映射到现有6个GIF
const OttoEmojiDisplay::EmotionMap OttoEmojiDisplay::emotion_maps_[] = {
    // 中性/平静类表情 -> staticstate
    {"neutral", &staticstate},
    {"relaxed", &staticstate},
    {"sleepy", &staticstate},

    // 积极/开心类表情 -> happy
    {"happy", &happy},
    {"laughing", &happy},
    {"funny", &happy},
    {"loving", &happy},
    {"confident", &happy},
    {"winking", &happy},
    {"cool", &happy},
    {"delicious", &happy},
    {"kissy", &happy},
    {"silly", &happy},

    // 悲伤类表情 -> sad
    {"sad", &sad},
    {"crying", &sad},

    // 愤怒类表情 -> anger
    {"angry", &anger},

    // 惊讶类表情 -> scare
    {"surprised", &scare},
    {"shocked", &scare},

    // 思考/困惑类表情 -> buxue
    {"thinking", &buxue},
    {"confused", &buxue},
    {"embarrassed", &buxue},

    {nullptr, nullptr}  // 结束标记
};

OttoEmojiDisplay::OttoEmojiDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                                                                     int width, int height, int offset_x, int offset_y, bool mirror_x,
                                                                     bool mirror_y, bool swap_xy)
        : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy) {
        // Tạm comment để tránh crash
        // SetupEyeContainer();
        ESP_LOGI(TAG, "OttoEmojiDisplay constructed, wake word manager available");
}

void OttoEmojiDisplay::SetupEyeContainer() {
    DisplayLockGuard lock(this);

    // Kiểm tra container_ trước khi dùng
    if (container_ == nullptr) {
        ESP_LOGE(TAG, "container_ is NULL, cannot setup eye");
        return;
    }

    if (eye_rect_) {
        lv_obj_del(eye_rect_);
    }
    if (content_) {
        lv_obj_del(content_);
    }

    content_ = lv_obj_create(container_);
    if (content_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create content_");
        return;
    }

    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(content_, LV_HOR_RES, LV_HOR_RES);
    lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_, 0, 0);
    lv_obj_set_flex_grow(content_, 1);
    lv_obj_center(content_);

    // Tạo mắt vuông bo tròn góc
    int eye_size = LV_HOR_RES * 0.5;
    int eye_radius = eye_size / 5; // Bo góc 20%
    eye_rect_ = lv_obj_create(content_);
    if (eye_rect_ == nullptr) {
        ESP_LOGE(TAG, "Failed to create eye_rect_");
        return;
    }

    lv_obj_set_size(eye_rect_, eye_size, eye_size);
    lv_obj_set_style_radius(eye_rect_, eye_radius, 0);
    lv_obj_set_style_bg_color(eye_rect_, eye_color_, 0);
    lv_obj_set_style_border_width(eye_rect_, 4, 0);
    lv_obj_set_style_border_color(eye_rect_, lv_color_hex(0x444444), 0);
    lv_obj_center(eye_rect_);
}

void OttoEmojiDisplay::SetEmotion(const char* emotion) {
    // Đổi màu mắt theo trạng thái nếu muốn
    DisplayLockGuard lock(this);
    if (!eye_rect_) return;

    // Ví dụ: đổi màu theo trạng thái
    if (emotion && strcmp(emotion, "happy") == 0) {
        eye_color_ = lv_color_hex(0x00C853); // Xanh lá
    } else if (emotion && strcmp(emotion, "sad") == 0) {
        eye_color_ = lv_color_hex(0x2979FF); // Xanh dương
    } else if (emotion && strcmp(emotion, "angry") == 0) {
        eye_color_ = lv_color_hex(0xD50000); // Đỏ
    } else {
        eye_color_ = lv_color_hex(0x222222); // Mặc định
    }
    lv_obj_set_style_bg_color(eye_rect_, eye_color_, 0);
}

void OttoEmojiDisplay::SetChatMessage(const char* role, const char* content) {
    // Gọi parent class để xử lý wake word command trước
    Display::SetChatMessage(role, content);
    
    DisplayLockGuard lock(this);
    if (chat_message_label_ == nullptr) {
        return;
    }

    if (content == nullptr || strlen(content) == 0) {
        lv_obj_add_flag(chat_message_label_, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    lv_label_set_text(chat_message_label_, content);
    lv_obj_remove_flag(chat_message_label_, LV_OBJ_FLAG_HIDDEN);

    ESP_LOGI(TAG, "设置聊天消息 [%s]: %s", role, content);
}

void OttoEmojiDisplay::UpdateStatusBar(bool update_all) {
    auto& app = Application::GetInstance();
    auto& board = Board::GetInstance();
    auto codec = board.GetAudioCodec();

    // Update mute icon
    {
        DisplayLockGuard lock(this);
        if (mute_label_ == nullptr) {
            return;
        }

        // 如果静音状态改变，则更新图标
        if (codec->output_volume() == 0 && !muted_) {
            muted_ = true;
            lv_label_set_text(mute_label_, FONT_AWESOME_VOLUME_XMARK);
        } else if (codec->output_volume() > 0 && muted_) {
            muted_ = false;
            lv_label_set_text(mute_label_, "");
        }
    }

    // Update time
    if (app.GetDeviceState() == kDeviceStateIdle) {
        if (last_status_update_time_ + std::chrono::seconds(10) < std::chrono::system_clock::now()) {
            // Set status to clock "HH:MM"
            time_t now = time(NULL);
            struct tm* tm = localtime(&now);
            // Check if the we have already set the time
            if (tm->tm_year >= 2025 - 1900) {
                char time_str[16];
                strftime(time_str, sizeof(time_str), "%H:%M  ", tm);
                SetStatus(time_str);
            } else {
                ESP_LOGW(TAG, "System time is not set, tm_year: %d", tm->tm_year);
            }
        }
    }

    esp_pm_lock_acquire(pm_lock_);
    // 更新电池图标 (không có cảnh báo âm thanh)
    int battery_level;
    bool charging, discharging;
    const char* icon = nullptr;
    if (board.GetBatteryLevel(battery_level, charging, discharging)) {
        if (charging) {
            icon = FONT_AWESOME_BATTERY_BOLT;
        } else {
            const char* levels[] = {
                FONT_AWESOME_BATTERY_EMPTY, // 0-19%
                FONT_AWESOME_BATTERY_QUARTER,    // 20-39%
                FONT_AWESOME_BATTERY_HALF,    // 40-59%
                FONT_AWESOME_BATTERY_THREE_QUARTERS,    // 60-79%
                FONT_AWESOME_BATTERY_FULL, // 80-99%
                FONT_AWESOME_BATTERY_FULL, // 100%
            };
            icon = levels[battery_level / 20];
        }
        DisplayLockGuard lock(this);
        if (battery_label_ != nullptr && battery_icon_ != icon) {
            battery_icon_ = icon;
            lv_label_set_text(battery_label_, battery_icon_);
        }

        // HOÀN TOÀN LOẠI BỎ: Không có bất kỳ xử lý nào cho low_battery_popup_
        // Otto robot sẽ không hiển thị popup cảnh báo pin yếu
    }

    // 每 10 秒更新一次网络图标
    static int seconds_counter = 0;
    if (update_all || seconds_counter++ % 10 == 0) {
        // 升级固件时，不读取 4G 网络状态，避免占用 UART 资源
        auto device_state = Application::GetInstance().GetDeviceState();
        static const std::vector<DeviceState> allowed_states = {
            kDeviceStateIdle,
            kDeviceStateStarting,
            kDeviceStateWifiConfiguring,
            kDeviceStateListening,
            kDeviceStateActivating,
        };
        if (std::find(allowed_states.begin(), allowed_states.end(), device_state) != allowed_states.end()) {
            const char* icon = board.GetNetworkStateIcon();
            if (network_label_ != nullptr && icon != nullptr && network_icon_ != icon) {
                DisplayLockGuard lock(this);
                network_icon_ = icon;
                lv_label_set_text(network_label_, network_icon_);
            }
        }
    }
    
    ESP_LOGD(TAG, "状态栏更新完成 (已禁用电池警告音)");
}
