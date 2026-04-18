#include <algorithm>
#include <atomic>
#include <cctype>
#include <esp_log.h>
#include <esp_timer.h>
#include <string>

#include "config.h"
#include "device_state.h"
#include "device_state_event.h"
#include "mcp_server.h"
#include "ricky_actions.h"
#include "ricky_bark.h"
#include "ricky_dance_music.h"
#include "ricky_motor.h"
#include "ricky_speed_settings.h"
#include "settings.h"
#include "wifi_station.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#define TAG "RickyController"

static bool RickyLoadEyeEmotionModeEnabled() {
    Settings s("ricky", false);
    return s.GetInt("eye_mode", 1) != 0;
}

static void RickyStoreEyeEmotionModeEnabled(bool enabled) {
    Settings s("ricky", true);
    s.SetInt("eye_mode", enabled ? 1 : 0);
}

static std::string RickyToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

/**
 * pace: slow|fast|normal(default)|custom — tiến/lùi dùng bộ % riêng (config.h).
 * custom: tham số speed 45–100 (tốc tiến hoặc tốc lùi tùy tool).
 */
static int RickyResolvePaceSpeed(const PropertyList& pl, bool forward) {
    std::string pace = RickyToLower(pl["pace"].value<std::string>());
    if (pace == "slow" || pace == "cham") {
        return forward ? RICKY_FORWARD_SPEED_SLOW : RICKY_BACKWARD_SPEED_SLOW;
    }
    if (pace == "fast" || pace == "nhanh") {
        return forward ? RICKY_FORWARD_SPEED_FAST : RICKY_BACKWARD_SPEED_FAST;
    }
    if (pace == "custom" || pace == "manual") {
        int v = pl["speed"].value<int>();
        if (v < RICKY_SPEED_MIN_EFFECTIVE) {
            return 0;
        }
        if (v > 100) {
            v = 100;
        }
        return v;
    }
    /* normal / default / mac dinh → % đã lưu NVS hoặc RICKY_SPEED_DEFAULT (config.h) */
    return forward ? RickySpeedGetStoredForwardNormal() : RickySpeedGetStoredBackwardNormal();
}

enum class RickyCmdType : uint8_t {
    Stop = 0,
    ForwardMs,
    BackwardMs,
    RunPreset,
    RunDanceBase,
    RunEyeAction,
};

struct RickyCmd {
    RickyCmdType type;
    int duration_ms;
    int speed;
    int preset_index;
};

class RickyController {
private:
    RickyMotor motor_;
    QueueHandle_t queue_ = nullptr;
    TaskHandle_t task_handle_ = nullptr;
    volatile bool busy_ = false;
    std::atomic<bool> eye_emotion_mode_enabled_{true};
    std::atomic<int64_t> auto_eye_suppress_until_us_{0};

    bool Enqueue(const RickyCmd& cmd) {
        if (queue_ == nullptr) {
            return false;
        }
        return xQueueSend(queue_, &cmd, pdMS_TO_TICKS(500)) == pdTRUE;
    }

    void SuppressAutoEyeForMs(uint32_t ms) {
        const int64_t now = esp_timer_get_time();
        auto_eye_suppress_until_us_.store(now + static_cast<int64_t>(ms) * 1000, std::memory_order_release);
    }

    void HandleDeviceStateChanged(DeviceState previous, DeviceState current) {
        /* Nếu rời idle sang tương tác (thường do wake word), dừng nhạc dance ngay để trả mic về listening. */
        if (previous == kDeviceStateIdle &&
            (current == kDeviceStateConnecting || current == kDeviceStateListening || current == kDeviceStateSpeaking)) {
            RickyStopDanceMusicNow();
        }
        /* Không tự chớp mắt theo trạng thái hội thoại; chỉ gọi self.ricky.eye_action khi cần. */
    }

    static void TaskEntry(void* arg) {
        auto* self = static_cast<RickyController*>(arg);
        RickyCmd cmd;
        while (1) {
            if (xQueueReceive(self->queue_, &cmd, portMAX_DELAY) != pdTRUE) {
                continue;
            }
            self->busy_ = true;
            switch (cmd.type) {
                case RickyCmdType::Stop:
                    self->motor_.Stop();
                    break;
                case RickyCmdType::ForwardMs:
                    self->motor_.Forward(cmd.speed);
                    vTaskDelay(pdMS_TO_TICKS(cmd.duration_ms));
                    self->motor_.Stop();
                    break;
                case RickyCmdType::BackwardMs:
                    self->motor_.Backward(cmd.speed);
                    vTaskDelay(pdMS_TO_TICKS(cmd.duration_ms));
                    self->motor_.Stop();
                    break;
                case RickyCmdType::RunPreset:
                    RickyRunPreset(self->motor_, cmd.preset_index);
                    break;
                case RickyCmdType::RunDanceBase:
                    RickyRunDanceBase(self->motor_, cmd.preset_index);
                    break;
                case RickyCmdType::RunEyeAction:
                    RickyRunEyeAction(self->motor_, cmd.preset_index);
                    break;
            }
            self->busy_ = false;
        }
    }

    void RegisterMcpTools() {
        auto& mcp = McpServer::GetInstance();

        mcp.AddTool("self.ricky.stop",
                    "Ricky_boy: dừng motor và tắt PWM (đèn mắt tắt theo phần cứng nếu chung với driver).",
                    PropertyList(), [this](const PropertyList&) -> ReturnValue {
                        SuppressAutoEyeForMs(3000);
                        RickyCmd c{RickyCmdType::Stop, 0, 0, 0};
                        if (!Enqueue(c)) {
                            return std::string("queue full");
                        }
                        return true;
                    });

        mcp.AddTool("self.ricky.forward",
                    "Ricky_boy: bước tới. pace=normal(mặc định 55%)|slow|fast|custom. "
                    "Tiến: PWM tuyến tính theo % (45–100, dưới 45 không chạy). "
                    "Tham số speed chỉ dùng khi pace=custom (tốc độ tiến). "
                    "Về hội thoại, ưu tiên trả lời theo thì tương lai trước khi chạy lệnh, ví dụ: "
                    "\"Tôi sẽ tiến lên 1 bước\"; tránh dùng dạng \"đã tiến...\".",
                    PropertyList({
                        Property("pace", kPropertyTypeString, "normal"),
                        Property("duration_ms", kPropertyTypeInteger, 2000, 200, 60000),
                        Property("speed", kPropertyTypeInteger, RICKY_SPEED_DEFAULT, RICKY_SPEED_MIN_EFFECTIVE, 100),
                    }),
                    [this](const PropertyList& properties) -> ReturnValue {
                        SuppressAutoEyeForMs(6000);
                        RickyCmd c{RickyCmdType::ForwardMs, properties["duration_ms"].value<int>(),
                                   RickyResolvePaceSpeed(properties, true), 0};
                        if (!Enqueue(c)) {
                            return std::string("queue full");
                        }
                        return true;
                    });

        mcp.AddTool("self.ricky.backward",
                    "Ricky_boy: bước lùi. pace giống forward nhưng bộ slow/fast riêng (config.h). "
                    "Lùi: cùng thang % nhưng firmware ánh xạ phi tuyến (trượt chân). "
                    "speed chỉ khi pace=custom (tốc độ lùi). "
                    "Về hội thoại, ưu tiên trả lời theo thì tương lai trước khi chạy lệnh, ví dụ: "
                    "\"Tôi sẽ lùi 1 bước\"; tránh dùng dạng \"đã lùi...\".",
                    PropertyList({
                        Property("pace", kPropertyTypeString, "normal"),
                        Property("duration_ms", kPropertyTypeInteger, 2000, 200, 60000),
                        Property("speed", kPropertyTypeInteger, RICKY_SPEED_DEFAULT, RICKY_SPEED_MIN_EFFECTIVE, 100),
                    }),
                    [this](const PropertyList& properties) -> ReturnValue {
                        SuppressAutoEyeForMs(6000);
                        RickyCmd c{RickyCmdType::BackwardMs, properties["duration_ms"].value<int>(),
                                   RickyResolvePaceSpeed(properties, false), 0};
                        if (!Enqueue(c)) {
                            return std::string("queue full");
                        }
                        return true;
                    });

        mcp.AddTool(
            "self.ricky.motor_speed_config",
            "Ricky_boy: tốc độ khi pace=normal (lưu NVS vĩnh viễn). action=get|set_forward|set_backward|"
            "set_both|reset_default. forward/backward: 45–100. reset_default = xóa NVS, dùng mặc định firmware "
            "(factory_default trong JSON). Ví dụ: đặt tiến 60%, lùi 58%, đưa về mặc định.",
            PropertyList({
                Property("action", kPropertyTypeString, "get"),
                Property("forward", kPropertyTypeInteger, RICKY_SPEED_DEFAULT, 0, 100),
                Property("backward", kPropertyTypeInteger, RICKY_SPEED_DEFAULT, 0, 100),
            }),
            [](const PropertyList& props) -> ReturnValue {
                std::string act = RickyToLower(props["action"].value<std::string>());
                auto wrap_ok = [](const char* action) {
                    return std::string("{\"ok\":true,\"action\":\"") + action + "\"," +
                           RickySpeedGetStatusJson().substr(1);
                };
                if (act == "get" || act == "status" || act == "xem") {
                    return wrap_ok("get");
                }
                if (act == "reset_default" || act == "factory" || act == "mac_dinh" || act == "ve_mac_dinh" ||
                    act == "default") {
                    RickySpeedResetToFactoryDefaults();
                    return wrap_ok("reset_default");
                }
                if (act == "set_forward" || act == "tien") {
                    int v = props["forward"].value<int>();
                    if (v < RICKY_SPEED_MIN_EFFECTIVE || v > 100) {
                        return std::string(
                            "{\"ok\":false,\"error\":\"forward must be 45-100 (pace=normal tiến)\"}");
                    }
                    if (!RickySpeedStoreForwardNormal(v)) {
                        return std::string("{\"ok\":false,\"error\":\"store forward failed\"}");
                    }
                    return wrap_ok("set_forward");
                }
                if (act == "set_backward" || act == "lui") {
                    int v = props["backward"].value<int>();
                    if (v < RICKY_SPEED_MIN_EFFECTIVE || v > 100) {
                        return std::string(
                            "{\"ok\":false,\"error\":\"backward must be 45-100 (pace=normal lùi)\"}");
                    }
                    if (!RickySpeedStoreBackwardNormal(v)) {
                        return std::string("{\"ok\":false,\"error\":\"store backward failed\"}");
                    }
                    return wrap_ok("set_backward");
                }
                if (act == "set_both" || act == "ca_hai") {
                    int vf = props["forward"].value<int>();
                    int vb = props["backward"].value<int>();
                    if (vf < RICKY_SPEED_MIN_EFFECTIVE || vf > 100 || vb < RICKY_SPEED_MIN_EFFECTIVE ||
                        vb > 100) {
                        return std::string("{\"ok\":false,\"error\":\"forward and backward must each be 45-100\"}");
                    }
                    if (!RickySpeedStoreForwardNormal(vf) || !RickySpeedStoreBackwardNormal(vb)) {
                        return std::string("{\"ok\":false,\"error\":\"store failed\"}");
                    }
                    return wrap_ok("set_both");
                }
                return std::string(
                    "{\"ok\":false,\"error\":\"unknown action: get, set_forward, set_backward, set_both, "
                    "reset_default\"}");
            });

        mcp.AddTool(
            "self.ricky.run_preset",
            "Chạy một trong 13 động tác: preset 1=Super Shake, 2=Sneaky Steps, "
            "3=Scaredy Cat, 4=Force Move, 5=Double Hop, 6=Elastic, 7=Engine Start, 8=Confused, "
            "9=Charge & Brake, 10=Heartbeat, 11=dog_dance1+nhạc dog_dance2 (~14s), 12=dog_dance5+nhạc dog_dance2 (30s), "
            "13=bài 13a Intro blend shake+elastic (~24s, test_rb case 12).",
            PropertyList({Property("preset", kPropertyTypeInteger, 1, 1, 13)}),
            [this](const PropertyList& properties) -> ReturnValue {
                int p = properties["preset"].value<int>();
                SuppressAutoEyeForMs(p == 13 ? 26000 : 12000);
                RickyCmd c{RickyCmdType::RunPreset, 0, 0, p - 1};
                if (!Enqueue(c)) {
                    return std::string("queue full");
                }
                return true;
            });

        mcp.AddTool(
            "self.ricky.run_dance",
            "Chạy bộ động tác cơ sở không âm thanh nền để ghép nhạc ngoài: "
            "dance 1=Super Shake(preset 1), 2=Sneaky Steps(2), 3=Scaredy Cat(3), 4=Force Move(4), "
            "5=Elastic(6), 6=Confused(8), 7=Heartbeat(10).",
            PropertyList({Property("dance", kPropertyTypeInteger, 1, 1, 7)}),
            [this](const PropertyList& properties) -> ReturnValue {
                const int dance = properties["dance"].value<int>();
                SuppressAutoEyeForMs(12000);
                RickyCmd c{RickyCmdType::RunDanceBase, 0, 0, dance - 1};
                if (!Enqueue(c)) {
                    return std::string("queue full");
                }
                return true;
            });

        mcp.AddTool(
            "self.ricky.eye_action",
            "Chạy động tác mắt đã test ổn định (đèn mắt dùng chung motor, PWM rất nhẹ để hạn chế quay và giảm bíp): "
            "1=CALM_DOUBLE, 2=HAPPY_TRIPLE, 3=SURPRISED_HOLD, 4=SAD_SPARSE, 5=SLEEPY_LONG_OFF, "
            "6=ALERT_STROBE, 7=HEADLIGHT_ON. Tự nghỉ 3000ms sau mỗi động tác.",
            PropertyList({Property("action", kPropertyTypeInteger, 1, 1, 7)}),
            [this](const PropertyList& properties) -> ReturnValue {
                const int action = properties["action"].value<int>();
                RickyCmd c{RickyCmdType::RunEyeAction, 0, 0, action - 1};
                if (!Enqueue(c)) {
                    return std::string("queue full");
                }
                return true;
            });

        mcp.AddTool(
            "self.ricky.eye_emotion_mode",
            "Luu co bay tat (NVS key eye_mode) cho tuong lai; firmware hien khong tu chop mat theo state. "
            "De chop mat, dung self.ricky.eye_action. action=get|enable|disable|set, enabled khi action=set.",
            PropertyList({
                Property("action", kPropertyTypeString, "get"),
                Property("enabled", kPropertyTypeBoolean, true),
            }),
            [this](const PropertyList& props) -> ReturnValue {
                const std::string action = RickyToLower(props["action"].value<std::string>());
                bool enabled = eye_emotion_mode_enabled_.load(std::memory_order_acquire);

                if (action == "enable" || action == "on" || action == "bat") {
                    enabled = true;
                } else if (action == "disable" || action == "off" || action == "tat") {
                    enabled = false;
                } else if (action == "set") {
                    enabled = props["enabled"].value<bool>();
                } else if (action != "get" && action != "status") {
                    return std::string(
                        "{\"ok\":false,\"error\":\"unknown action: get, enable, disable, set\"}");
                }

                if (action != "get" && action != "status") {
                    eye_emotion_mode_enabled_.store(enabled, std::memory_order_release);
                    RickyStoreEyeEmotionModeEnabled(enabled);
                    ESP_LOGI(TAG, "eye emotion mode: %s", enabled ? "enabled" : "disabled");
                }

                return std::string("{\"ok\":true,\"enabled\":") + (enabled ? "true" : "false") + "}";
            });

        mcp.AddTool("self.ricky.bark_short",
                    "Phát tiếng chó sủa ngắn (dữ liệu chosua1s_short, PCM 24 kHz).",
                    PropertyList(), [](const PropertyList&) -> ReturnValue {
                        RickyPlayBarkShortAsync();
                        return true;
                    });

        mcp.AddTool("self.ricky.bark_long",
                    "Phát tiếng chó sủa dài (dữ liệu chosua1s_long, PCM 24 kHz).",
                    PropertyList(), [](const PropertyList&) -> ReturnValue {
                        RickyPlayBarkLongAsync();
                        return true;
                    });

        mcp.AddTool("self.ricky.get_status", "Trạng thái motor: moving hoặc idle.", PropertyList(),
                    [this](const PropertyList&) -> ReturnValue { return busy_ ? "moving" : "idle"; });

        mcp.AddTool("self.ricky.get_ip", "WiFi IP của Ricky_boy.", PropertyList(),
                    [](const PropertyList&) -> ReturnValue {
                        auto& ws = WifiStation::GetInstance();
                        std::string ip = ws.GetIpAddress();
                        if (ip.empty()) {
                            return std::string("{\"ip\":\"\",\"connected\":false}");
                        }
                        return std::string("{\"ip\":\"" + ip + "\",\"connected\":true}");
                    });

        ESP_LOGI(TAG, "MCP Ricky_boy registered");
    }

public:
    RickyController() {
        eye_emotion_mode_enabled_.store(RickyLoadEyeEmotionModeEnabled(), std::memory_order_release);
        motor_.Init(RICKY_MOTOR_IN1_GPIO, RICKY_MOTOR_IN2_GPIO);
        queue_ = xQueueCreate(6, sizeof(RickyCmd));
        if (queue_ == nullptr) {
            ESP_LOGE(TAG, "queue create failed");
        }
        if (xTaskCreate(TaskEntry, "ricky_motor", 4096, this, 5, &task_handle_) != pdPASS) {
            ESP_LOGE(TAG, "task create failed");
        }
        DeviceStateEventManager::GetInstance().RegisterStateChangeCallback(
            [this](DeviceState previous, DeviceState current) { HandleDeviceStateChanged(previous, current); });
        RegisterMcpTools();
        ESP_LOGI(TAG, "eye auto by conversation state: off (manual via self.ricky.eye_action); eye_mode NVS flag: %s",
                 eye_emotion_mode_enabled_.load(std::memory_order_acquire) ? "on" : "off");
    }
};

static RickyController* g_ricky_controller = nullptr;

void InitializeRickyController() {
    if (g_ricky_controller == nullptr) {
        g_ricky_controller = new RickyController();
        ESP_LOGI(TAG, "Ricky_boy controller ready");
    }
}
