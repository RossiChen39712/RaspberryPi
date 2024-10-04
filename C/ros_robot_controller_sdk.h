#ifndef SDK_H
#define SDK_H

#include <stdint.h>
#include <stdbool.h>

// 定義通信協議狀態
typedef enum
{
    // 0xAA 0x55 Length Function ID Data Checksum
    PACKET_CONTROLLER_STATE_STARTBYTE1 = 0,
    PACKET_CONTROLLER_STATE_STARTBYTE2 = 1,
    PACKET_CONTROLLER_STATE_LENGTH = 2,
    PACKET_CONTROLLER_STATE_FUNCTION = 3,
    PACKET_CONTROLLER_STATE_ID = 4,
    PACKET_CONTROLLER_STATE_DATA = 5,
    PACKET_CONTROLLER_STATE_CHECKSUM = 6
} PacketControllerState;

// 可通過串列埠實現的控制功能
typedef enum
{
    PACKET_FUNC_SYS = 0,       // 系統功能
    PACKET_FUNC_LED = 1,       // LED 控制
    PACKET_FUNC_BUZZER = 2,    // 蜂鳴器控制
    PACKET_FUNC_MOTOR = 3,     // 馬達控制
    PACKET_FUNC_PWM_SERVO = 4, // PWM 舵機控制，板子上從裡到外依次為1-4
    PACKET_FUNC_BUS_SERVO = 5, // 總線舵機控制
    PACKET_FUNC_KEY = 6,       // 按鍵狀態獲取
    PACKET_FUNC_IMU = 7,       // IMU 獲取
    PACKET_FUNC_GAMEPAD = 8,   // 手柄狀態獲取
    PACKET_FUNC_SBUS = 9,      // 航模遙控數據獲取
    PACKET_FUNC_OLED = 10,     // OLED 顯示內容設定
    PACKET_FUNC_RGB = 11,      // 設置 RGB 顏色
    PACKET_FUNC_NONE = 12      // 無效功能
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

// Board 結構體，用於保存狀態
typedef struct
{
    int fd;
    // 添加其他必要的狀態和資源
} Board;

// 函數聲明
// 配置串列埠
int configure_serial(const char *device);

// 計算 CRC8 的函數
uint8_t checksum_crc8(const uint8_t *data, size_t len);

// 串列發送函數
void buf_write(Board *board, uint8_t func, const uint8_t *data, int len);

// 設置 RGB LED 的函數
void board_set_rgb(Board *board, int pixels[][4], int count);

#endif // SDK_H
