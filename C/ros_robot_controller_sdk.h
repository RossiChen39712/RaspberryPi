#ifndef STM32_SDK_H
#define STM32_SDK_H

#include <stdint.h>
#include <stdbool.h>

// 定義通信協議狀態
typedef enum
{
    PACKET_CONTROLLER_STATE_STARTBYTE1 = 0,
    PACKET_CONTROLLER_STATE_STARTBYTE2,
    PACKET_CONTROLLER_STATE_LENGTH,
    PACKET_CONTROLLER_STATE_FUNCTION,
    PACKET_CONTROLLER_STATE_ID,
    PACKET_CONTROLLER_STATE_DATA,
    PACKET_CONTROLLER_STATE_CHECKSUM
} PacketControllerState;

// 定義功能的種類
typedef enum
{
    PACKET_FUNC_SYS = 0,
    PACKET_FUNC_LED,
    PACKET_FUNC_BUZZER,
    PACKET_FUNC_MOTOR,
    PACKET_FUNC_PWM_SERVO,
    PACKET_FUNC_BUS_SERVO,
    PACKET_FUNC_KEY,
    PACKET_FUNC_IMU,
    PACKET_FUNC_GAMEPAD,
    PACKET_FUNC_SBUS,
    PACKET_FUNC_OLED,
    PACKET_FUNC_RGB,
    PACKET_FUNC_NONE
} PacketFunction;

// 按鍵事件的狀態
typedef enum
{
    KEY_EVENT_PRESSED = 0x01,
    KEY_EVENT_LONGPRESS = 0x02,
    KEY_EVENT_LONGPRESS_REPEAT = 0x04,
    KEY_EVENT_RELEASE_FROM_LP = 0x08,
    KEY_EVENT_RELEASE_FROM_SP = 0x10,
    KEY_EVENT_CLICK = 0x20,
    KEY_EVENT_DOUBLE_CLICK = 0x40,
    KEY_EVENT_TRIPLE_CLICK = 0x80
} PacketReportKeyEvents;

// SBUS狀態結構體
typedef struct
{
    int16_t channels[16];
    bool channel_17;
    bool channel_18;
    bool signal_loss;
    bool fail_safe;
} SBusStatus;

// Board 結構體，用於保存狀態
typedef struct
{
    int enable_recv;
    int frame[256];
    int recv_count;
    // 添加其他必要的狀態和資源
} Board;

// 函數聲明

// 初始化 Board
void board_init(Board *board, const char *device, int baudrate, int timeout);

// 設置 LED 控制
void board_set_led(Board *board, float on_time, float off_time, int repeat, int led_id);

// 設置蜂鳴器
void board_set_buzzer(Board *board, int freq, float on_time, float off_time, int repeat);

// 設置電機速度
void board_set_motor_speed(Board *board, const float *speeds, int count);

// 設置 OLED 顯示內容
void board_set_oled_text(Board *board, int line, const char *text);

// 設置 RGB 顏色
void board_set_rgb(Board *board, const uint8_t *pixels, int count);

// 設置電機佔空比
void board_set_motor_duty(Board *board, const float *dutys, int count);

// 設置 PWM 舵機位置
void board_pwm_servo_set_position(Board *board, float duration, const int *positions, int count);

// 讀取按鍵狀態
int board_get_button(Board *board, int *key_id, int *key_event);

// 讀取 IMU 狀態
int board_get_imu(Board *board, float *imu_data);

// 讀取遊戲手柄狀態
int board_get_gamepad(Board *board, float *axes, int *buttons);

// 讀取 SBUS 狀態
int board_get_sbus(Board *board, SBusStatus *sbus_status);

// 開啟或關閉數據接收
void board_enable_reception(Board *board, int enable);

// 校驗函數
uint8_t checksum_crc8(const uint8_t *data, int len);

// 接收數據的執行緒
void *board_recv_task(void *arg);

// SBUS 測試
void bus_servo_test(Board *board);

// PWM 測試
void pwm_servo_test(Board *board);

#endif // STM32_SDK_H
