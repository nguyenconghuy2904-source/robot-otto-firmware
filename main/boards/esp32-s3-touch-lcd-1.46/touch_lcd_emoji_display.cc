#include "touch_lcd_emoji_display.h"
#include "lvgl_theme.h"

#include <esp_log.h>
#include <font_awesome.h>
#include <lvgl.h>

#include <algorithm>
#include <cstring>
#include <string>

#define TAG "TouchLcdEmojiDisplay"

// 表情映射表 - sử dụng GIF resource
const TouchLcdEmojiDisplay::EmotionMap TouchLcdEmojiDisplay::emotion_maps_[] = {
    // Neutral
    {"neutral", &staticstate},
    {"relaxed", &staticstate},
    {"sleepy", &staticstate},

    // Happy
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

    // Add more if needed
    {nullptr, nullptr}  // 结束标记
};

TouchLcdEmojiDisplay::TouchLcdEmojiDisplay(esp_lcd_panel_io_handle_t panel_io,
                                           esp_lcd_panel_handle_t panel, int width, int height,
                                           int offset_x, int offset_y, bool mirror_x, bool mirror_y,
                                           bool swap_xy)
    : SpiLcdDisplay(panel_io, panel, width, height, offset_x, offset_y, mirror_x, mirror_y, swap_xy),
      emotion_gif_(nullptr) {
    DisplayLockGuard lock(this);
    lv_display_add_event_cb(display_, rounder_event_cb, LV_EVENT_INVALIDATE_AREA, NULL);
    SetupGifContainer();
}

void TouchLcdEmojiDisplay::SetupGifContainer() {
    DisplayLockGuard lock(this);

    if (emoji_label_) {
        lv_obj_del(emoji_label_);
    }
    if (chat_message_label_) {
        lv_obj_del(chat_message_label_);
    }
    if (content_) {
        lv_obj_del(content_);
    }

    content_ = lv_obj_create(container_);
    lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_size(content_, LV_HOR_RES, LV_HOR_RES);
    lv_obj_set_style_bg_opa(content_, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content_, 0, 0);
    lv_obj_set_flex_grow(content_, 1);
    lv_obj_center(content_);

    emoji_label_ = lv_label_create(content_);
    lv_label_set_text(emoji_label_, "");
    lv_obj_set_width(emoji_label_, 0);
    lv_obj_set_style_border_width(emoji_label_, 0, 0);
    lv_obj_add_flag(emoji_label_, LV_OBJ_FLAG_HIDDEN);

    // Sử dụng class LvglGif cho emoji GIF
    if (emotion_gif_) {
        delete emotion_gif_;
        emotion_gif_ = nullptr;
    }
    emotion_gif_ = new LvglGif(&staticstate);
    emotion_gif_->Start();

    chat_message_label_ = lv_label_create(content_);
    lv_label_set_text(chat_message_label_, "");
    lv_obj_set_width(chat_message_label_, LV_HOR_RES * 0.9);
    lv_label_set_long_mode(chat_message_label_, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_style_text_align(chat_message_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(chat_message_label_, lv_color_white(), 0);
    lv_obj_set_style_border_width(chat_message_label_, 0, 0);

    lv_obj_set_style_bg_opa(chat_message_label_, LV_OPA_70, 0);
    lv_obj_set_style_bg_color(chat_message_label_, lv_color_black(), 0);
    lv_obj_set_style_pad_ver(chat_message_label_, 5, 0);

    lv_obj_align(chat_message_label_, LV_ALIGN_BOTTOM_MID, 0, 0);

    auto& theme_manager = LvglThemeManager::GetInstance();
    auto theme = theme_manager.GetTheme("dark");
    if (theme != nullptr) {
        LcdDisplay::SetTheme(theme);
    }
}

void TouchLcdEmojiDisplay::SetEmotion(const char* emotion) {
    if (!emotion || !emotion_gif_) {
        return;
    }

    DisplayLockGuard lock(this);

    for (const auto& map : emotion_maps_) {
        if (map.name && strcmp(map.name, emotion) == 0) {
            if (emotion_gif_) {
                delete emotion_gif_;
            }
            emotion_gif_ = new LvglGif(map.gif);
            emotion_gif_->Start();
            ESP_LOGI(TAG, "设置表情: %s", emotion);
            return;
        }
    }

    if (emotion_gif_) {
        delete emotion_gif_;
    }
    emotion_gif_ = new LvglGif(&staticstate);
    emotion_gif_->Start();
    ESP_LOGI(TAG, "未知表情'%s'，使用默认", emotion);
}

void TouchLcdEmojiDisplay::SetChatMessage(const char* role, const char* content) {
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