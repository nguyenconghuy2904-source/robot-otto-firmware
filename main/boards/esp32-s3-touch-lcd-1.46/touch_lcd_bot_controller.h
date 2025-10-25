#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "movements.h"
#include "pc_stream_client.h"
#include <string>

// Forward declarations
class AudioCodec;
class LcdDisplay;

enum class ActionType : int {
    HAND_ACTION = 0,
    BODY_ACTION = 1,
    HEAD_ACTION = 2,
    POWER_ACTION = 3
};

struct RobotAction {
    ActionType type;
    int action_id;
    uint32_t duration_ms;
};

class TouchLcdBotController {
public:
    TouchLcdBotController(Otto* otto);
    ~TouchLcdBotController();
    
    bool init();
    void start();
    void stop();
    void queueAction(ActionType type, int action_id, uint32_t duration_ms = 1000);
    bool isActionInProgress();

    // PC Streaming functions
    void InitPCStream(const std::string& ws_url);
    void SendAudioToPC(const std::vector<int16_t>& pcm_data);
    void SendImageToPC(const std::vector<uint8_t>& jpeg_data);
    void SetAudioCodec(AudioCodec* codec);
    void SetLcdDisplay(LcdDisplay* display);

private:
    static void controllerTask(void* pvParameters);
    void processActionQueue();
    
    Otto* otto_;
    QueueHandle_t action_queue_;
    TaskHandle_t controller_task_;
    bool running_;
    bool action_in_progress_;
    
    // PC Streaming
    PCStreamClient pc_stream_client_;
    AudioCodec* audio_codec_ = nullptr;
    LcdDisplay* lcd_display_ = nullptr;
    
    static const int MAX_QUEUE_SIZE = 10;
    static const int STACK_SIZE = 4096;
    static const int TASK_PRIORITY = 5;
};

// Global function declarations
void InitializeTouchLcdBotController(Otto* otto);