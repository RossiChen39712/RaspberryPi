#include "ros_robot_controller_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>  // for sleep function
#include <fcntl.h>   // for file control options
#include <termios.h> // for POSIX terminal control definitions

// CRC8 校驗表
static const uint8_t crc8_table[256] = {
    0, 94, 188, 226, 97, 63, 221, 131, 194, 156, 126, 32, 163, 253, 31, 65,
    157, 195, 33, 127, 252, 162, 64, 30, 95, 1, 227, 189, 62, 96, 130, 220,
    35, 125, 159, 193, 66, 28, 254, 160, 225, 191, 93, 3, 128, 222, 60, 98,
    190, 224, 2, 92, 223, 129, 99, 61, 124, 34, 192, 158, 29, 67, 161, 255,
    70, 24, 250, 164, 39, 121, 155, 197, 132, 218, 56, 102, 229, 187, 89, 7,
    219, 133, 103, 57, 186, 228, 6, 88, 25, 71, 165, 251, 120, 38, 196, 154,
    101, 59, 217, 135, 4, 90, 184, 230, 167, 249, 27, 69, 198, 152, 122, 36,
    248, 166, 68, 26, 153, 199, 37, 123, 58, 100, 134, 216, 91, 5, 231, 185,
    140, 210, 48, 110, 237, 179, 81, 15, 78, 16, 242, 172, 47, 113, 147, 205,
    17, 79, 173, 243, 112, 46, 204, 146, 211, 141, 111, 49, 178, 236, 14, 80,
    175, 241, 19, 77, 206, 144, 114, 44, 109, 51, 209, 143, 12, 82, 176, 238,
    50, 108, 142, 208, 83, 13, 239, 177, 240, 174, 76, 18, 145, 207, 45, 115,
    202, 148, 118, 40, 171, 245, 23, 73, 8, 86, 180, 234, 105, 55, 213, 139,
    87, 9, 235, 181, 54, 104, 138, 212, 149, 203, 41, 119, 244, 170, 72, 22,
    233, 183, 85, 11, 136, 214, 52, 106, 43, 117, 151, 201, 74, 20, 246, 168,
    116, 42, 200, 150, 21, 75, 169, 247, 182, 232, 10, 84, 215, 137, 107, 53};

// CRC8 校驗計算
uint8_t checksum_crc8(const uint8_t *data, int len)
{
    uint8_t check = 0;
    for (int i = 0; i < len; i++)
    {
        check = crc8_table[check ^ data[i]];
    }
    return check & 0xFF;
}

// 初始化 Board
void board_init(Board *board, const char *device, int baudrate, int timeout)
{
    board->enable_recv = 0;
    board->recv_count = 0;

    // 打開串口
    board->fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (board->fd == -1)
    {
        perror("串口打開失敗");
        return;
    }

    struct termios options;
    tcgetattr(board->fd, &options);
    cfsetispeed(&options, baudrate);
    cfsetospeed(&options, baudrate);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CRTSCTS;
    tcsetattr(board->fd, TCSANOW, &options);

    // 啟動接收執行緒
    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, board_recv_task, (void *)board);
}

// 設置 LED
void board_set_led(Board *board, float on_time, float off_time, int repeat, int led_id)
{
    uint16_t on_ms = (uint16_t)(on_time * 1000);
    uint16_t off_ms = (uint16_t)(off_time * 1000);
    uint8_t data[7] = {led_id, on_ms & 0xFF, on_ms >> 8, off_ms & 0xFF, off_ms >> 8, repeat & 0xFF, repeat >> 8};
    board_buf_write(board, PACKET_FUNC_LED, data, 7);
}

// 設置電機佔空比
void board_set_motor_duty(Board *board, const float *dutys, int count)
{
    uint8_t data[10]; // 假設最多支持4個電機
    data[0] = 0x05;
    data[1] = count;
    for (int i = 0; i < count; i++)
    {
        data[2 + i * 2] = (uint8_t)(dutys[i] * 100); // 簡單比例轉換
    }
    board_buf_write(board, PACKET_FUNC_MOTOR, data, 2 + count * 2);
}

// 讀取 IMU 數據
int board_get_imu(Board *board, float *imu_data)
{
    if (board->enable_recv)
    {
        // 假設有隊列系統來檢查 IMU 數據
        // 填充 imu_data 數據並返回 1 表示成功
        return 1;
    }
    return 0;
}

// 接收數據的執行緒
void *board_recv_task(void *arg)
{
    Board *board = (Board *)arg;
    uint8_t recv_data;

    while (1)
    {
        if (board->enable_recv)
        {
            int n = read(board->fd, &recv_data, 1);
            if (n > 0)
            {
                // 根據通信協議處理接收數據
                // 這裡需要添加接收邏輯
            }
        }
        else
        {
            usleep(10000); // 每次迴圈延遲 10 毫秒
        }
    }
    return NULL;
}

// 傳輸數據到串口
void board_buf_write(Board *board, PacketFunction func, const uint8_t *data, int len)
{
    uint8_t buf[256];
    buf[0] = 0xAA;
    buf[1] = 0x55;
    buf[2] = func;
    buf[3] = len;
    memcpy(&buf[4], data, len);
    buf[4 + len] = checksum_crc8(&buf[2], len + 2);
    write(board->fd, buf, len + 5);
}

// 開啟接收
void board_enable_reception(Board *board, int enable)
{
    board->enable_recv = enable;
}

// 測試 Bus 舵機
void bus_servo_test(Board *board)
{
    // 舵機測試代碼
}

// 測試 PWM 舵機
void pwm_servo_test(Board *board)
{
    // PWM 舵機測試代碼
}
