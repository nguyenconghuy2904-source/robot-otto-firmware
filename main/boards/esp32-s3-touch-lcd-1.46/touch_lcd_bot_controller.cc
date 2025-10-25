#include "touch_lcd_bot_controller.h"
#include "../../application.h"
#include "../../mcp_server.h"
#include "../../settings.h"
#include "pc_stream_client.h"

#include <esp_log.h>

#define CONTROLLER_TAG "TouchLcdBotController"

// 静态变量声明
static TouchLcdBotController* g_controller = nullptr;

// TouchLcdBotController class implementations
TouchLcdBotController::TouchLcdBotController(Otto* otto) 
    : otto_(otto), action_queue_(nullptr), controller_task_(nullptr),
      running_(false), action_in_progress_(false) {
}

TouchLcdBotController::~TouchLcdBotController() {
    stop();
    if (action_queue_) {
        vQueueDelete(action_queue_);
    }
}

bool TouchLcdBotController::init() {
    action_queue_ = xQueueCreate(MAX_QUEUE_SIZE, sizeof(RobotAction));
    if (action_queue_ == nullptr) {
        ESP_LOGE(CONTROLLER_TAG, "Failed to create action queue");
        return false;
    }
    return true;
}

void TouchLcdBotController::start() {
    if (!running_) {
        running_ = true;
        xTaskCreate(controllerTask, "bot_controller", STACK_SIZE, this, TASK_PRIORITY, &controller_task_);
    }
}

void TouchLcdBotController::stop() {
    running_ = false;
    if (controller_task_) {
        vTaskDelete(controller_task_);
        controller_task_ = nullptr;
    }
}

void TouchLcdBotController::queueAction(ActionType type, int action_id, uint32_t duration_ms) {
    RobotAction action = {type, action_id, duration_ms};
    if (xQueueSend(action_queue_, &action, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(CONTROLLER_TAG, "Action queue is full, dropping action");
    }
}

bool TouchLcdBotController::isActionInProgress() {
    return action_in_progress_;
}

void TouchLcdBotController::controllerTask(void* pvParameters) {
    TouchLcdBotController* controller = static_cast<TouchLcdBotController*>(pvParameters);
    controller->processActionQueue();
}

void TouchLcdBotController::processActionQueue() {
    RobotAction action;
    
    while (running_) {
        if (xQueueReceive(action_queue_, &action, pdMS_TO_TICKS(100)) == pdTRUE) {
            action_in_progress_ = true;
            
            switch (action.type) {
                case ActionType::HAND_ACTION:
                    if (action.action_id >= 1 && action.action_id <= 12) {
                        otto_->HandAction(action.action_id);
                    }
                    break;
                case ActionType::BODY_ACTION:
                    if (action.action_id >= 1 && action.action_id <= 2) {
                        otto_->BodyAction(action.action_id);
                    }
                    break;
                case ActionType::HEAD_ACTION:
                    if (action.action_id >= 1 && action.action_id <= 5) {
                        otto_->HeadAction(action.action_id);
                    }
                    break;
                case ActionType::POWER_ACTION:
                    // Handle power actions if needed
                    break;
            }
            
            // Wait for the specified duration
            if (action.duration_ms > 0) {
                vTaskDelay(pdMS_TO_TICKS(action.duration_ms));
            }
            
            action_in_progress_ = false;
        }
    }
}

// 全局函数实现
void InitializeTouchLcdBotController(Otto* otto) {
    if (g_controller == nullptr) {
        g_controller = new TouchLcdBotController(otto);
        ESP_LOGI(CONTROLLER_TAG, "Touch LCD Bot控制器已初始化并注册MCP工具");
    }
}

// PCStreamClient functions

// PCStreamClient functions
void TouchLcdBotController::InitPCStream(const std::string& ws_url) {
    ESP_LOGI(CONTROLLER_TAG, "Initializing PC Stream: %s", ws_url.c_str());
    
    pc_stream_client_.Connect(ws_url);
    
    // Callback nhận audio từ PC -> phát ra loa
    pc_stream_client_.OnAudioReceived([this](const std::vector<int16_t>& pcm_data) {
        if (audio_codec_) {
            std::vector<int16_t> data_copy = pcm_data;
            audio_codec_->OutputData(data_copy);
            ESP_LOGD(CONTROLLER_TAG, "Received audio from PC: %d samples", pcm_data.size());
        }
    });
    
    // Callback nhận hình ảnh từ PC -> hiển thị lên LCD
    pc_stream_client_.OnImageReceived([this](const std::vector<uint8_t>& jpeg_data) {
        if (lcd_display_) {
            // TODO: Giải mã JPEG và hiển thị lên LCD
            // Cần module JPEG decoder (esp_jpg_decode hoặc tương tự)
            ESP_LOGD(CONTROLLER_TAG, "Received image from PC: %d bytes", jpeg_data.size());
        }
    });
}

void TouchLcdBotController::SendAudioToPC(const std::vector<int16_t>& pcm_data) {
    pc_stream_client_.SendAudioPCM(pcm_data);
}

void TouchLcdBotController::SendImageToPC(const std::vector<uint8_t>& jpeg_data) {
    pc_stream_client_.SendImageJPEG(jpeg_data);
}

void TouchLcdBotController::SetAudioCodec(AudioCodec* codec) {
    audio_codec_ = codec;
}

void TouchLcdBotController::SetLcdDisplay(LcdDisplay* display) {
    lcd_display_ = display;
}