#ifndef WAKE_WORD_MANAGER_H
#define WAKE_WORD_MANAGER_H

#include <string>
#include <vector>
#include <regex>

class WakeWordManager {
public:
    static WakeWordManager& GetInstance() {
        static WakeWordManager instance;
        return instance;
    }

    // Phát hiện lệnh thêm wake word từ tin nhắn chat
    bool DetectAddWakeWordCommand(const char* content, std::string& wake_word);
    
    // Thêm wake word mới vào danh sách
    void AddWakeWord(const std::string& wake_word);
    
    // Lấy danh sách wake word
    std::vector<std::string> GetWakeWords();
    
    // Set wake word hiện tại
    void SetCurrentWakeWord(const std::string& wake_word);
    
    // Get wake word hiện tại
    std::string GetCurrentWakeWord();

private:
    WakeWordManager() = default;
    ~WakeWordManager() = default;
    
    // Lưu/đọc từ NVS
    void SaveWakeWords();
    void LoadWakeWords();
    
    std::vector<std::string> wake_words_;
    std::string current_wake_word_;
};

#endif // WAKE_WORD_MANAGER_H