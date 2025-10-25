#pragma once

#include "display/lcd_display.h"

/**
 * @brief Otto机器人GIF表情显示类
 * 继承LcdDisplay，添加GIF表情支持
 */
class OttoEmojiDisplay : public SpiLcdDisplay {
public:
    OttoEmojiDisplay(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_handle_t panel, int width,
                     int height, int offset_x, int offset_y, bool mirror_x, bool mirror_y,
                     bool swap_xy);
    virtual ~OttoEmojiDisplay() = default;

    // Vẽ mắt vuông bo tròn
    virtual void SetEmotion(const char* emotion) override;
    virtual void SetChatMessage(const char* role, const char* content) override;
    virtual void UpdateStatusBar(bool update_all = false) override;

private:
    void SetupEyeContainer();
    lv_obj_t* eye_rect_ = nullptr; // Mắt vuông bo tròn
    lv_color_t eye_color_ = lv_color_hex(0x222222); // Màu mắt mặc định
};