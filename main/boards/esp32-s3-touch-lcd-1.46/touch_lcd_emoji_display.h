#pragma once



#include "display/lcd_display.h"
#include <lvgl.h>
#include "touch_lcd_gif_assets.h" // GIF asset declarations
#include "display/lvgl_display/gif/lvgl_gif.h"

// Use GIF-based emojis (like electron-bot)

/**
 * @brief ESP32-S3 Touch LCD 1.46 GIF表情显示类
 * 继承CustomLcdDisplay，添加GIF表情支持
 */
class TouchLcdEmojiDisplay : public SpiLcdDisplay {
public:
    static void rounder_event_cb(lv_event_t * e) {
        lv_area_t * area = (lv_area_t *)lv_event_get_param(e);
        uint16_t x1 = area->x1;
        uint16_t x2 = area->x2;

        area->x1 = (x1 >> 2) << 2;          // round the start of coordinate down to the nearest 4M number
        area->x2 = ((x2 >> 2) << 2) + 3;    // round the end of coordinate up to the nearest 4N+3 number
    }

    /**
     * @brief 构造函数，参数与SpiLcdDisplay相同
     */
    TouchLcdEmojiDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel,
                         int width, int height, int offset_x, int offset_y, bool mirror_x,
                         bool mirror_y, bool swap_xy);

    virtual ~TouchLcdEmojiDisplay() = default;

    // 重写表情设置方法
    virtual void SetEmotion(const char* emotion) override;

    // 重写聊天消息设置方法
    virtual void SetChatMessage(const char* role, const char* content) override;

private:
    void SetupGifContainer();

    LvglGif* emotion_gif_;  ///< GIF表情 (LvglGif)

    // 表情映射 - sử dụng GIF resource
    struct EmotionMap {
        const char* name;
        const lv_img_dsc_t* gif;
    };

    static const EmotionMap emotion_maps_[];
};