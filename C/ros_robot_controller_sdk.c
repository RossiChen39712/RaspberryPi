#include "ros_robot_controller_sdk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>  // for sleep function
#include <fcntl.h>   // for file control options
#include <termios.h> // for POSIX terminal control definitions
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define BAUDRATE B1000000     // 波特率
#define DEVICE "/dev/ttyAMA0" // 設備名稱
#define BUF_SIZE 256          // 緩衝區大小

// 配置串列埠
int configure_serial(const char *device, int baudrate)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        perror("Failed to open the device");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    cfsetispeed(&options, baudrate);
    cfsetospeed(&options, baudrate);

    options.c_cflag |= (CLOCAL | CREAD); // 本地連接，允許讀取
    options.c_cflag &= ~CSIZE;           // 清除資料大小位元
    options.c_cflag |= CS8;              // 8位元數據
    options.c_cflag &= ~PARENB;          // 無奇偶校驗
    options.c_cflag &= ~CSTOPB;          // 一個停止位
    options.c_cflag &= ~CRTSCTS;         // 不使用硬體流控

    tcsetattr(fd, TCSANOW, &options); // 設置立即生效

    return fd;
}

// 接收數據的執行緒函數
void *recv_task(void *arg)
{
    PacketController *controller = (PacketController *)arg;
    uint8_t buf[BUF_SIZE];

    while (controller->enable_recv)
    {
        // 從串列埠讀取數據
        int n = read(controller->fd, buf, sizeof(buf));
        if (n > 0)
        {
            // 打印接收到的數據
            printf("Received data: ");
            for (int i = 0; i < n; i++)
            {
                printf("%02x ", buf[i]);
            }
            printf("\n");

            controller->state = PACKET_CONTROLLER_STATE_RECV_DATA;
        }
        usleep(1000); // 避免過高 CPU 負載
    }

    return NULL;
}

// 初始化控制器
PacketController *init_controller()
{
    PacketController *controller = (PacketController *)malloc(sizeof(PacketController));

    // 初始化串列埠
    controller->fd = configure_serial(DEVICE, BAUDRATE);
    if (controller->fd < 0)
    {
        free(controller);
        return NULL;
    }

    controller->state = PACKET_CONTROLLER_STATE_STARTBYTE1;
    controller->enable_recv = 1;

    // 啟動接收執行緒
    pthread_create(&controller->recv_thread, NULL, recv_task, (void *)controller);
    usleep(100000); // 稍作延遲等待執行緒啟動

    return controller;
}

// 結束控制器，清理資源
void cleanup_controller(PacketController *controller)
{
    controller->enable_recv = 0;                 // 停止接收
    pthread_join(controller->recv_thread, NULL); // 等待執行緒結束
    close(controller->fd);                       // 關閉串列埠
    free(controller);                            // 釋放內存
}

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

// 傳輸數據到串口
void buf_write(Board *board, PacketFunction func, const uint8_t *data, int len)
{
    uint8_t buf[256];
    buf[0] = 0xAA;
    buf[1] = 0x55;
    buf[2] = func;
    buf[3] = len;
    memcpy(&buf[4], data, len);
    buf[4 + len] = checksum_crc8(&buf[2], len + 2);
    // 打印發送的緩衝區
    printf("Sending buffer: ");
    for (int i = 0; i < len + 5; i++)
    {
        printf("%02x ", buf[i]);
    }
    printf("\n");

    write(board->fd, buf, len + 5);
}

// 開啟接收
void board_enable_reception(Board *board, int enable)
{
    board->enable_recv = enable;
}

// 設置 RGB LED 的函數
void board_set_rgb(Board *board, int pixels[][4], int count)
{
    // 創建數據緩衝區
    uint8_t data[2 + count * 4]; // 2個字節頭信息 + 每個LED 4個字節(1個ID + 3個RGB)
    data[0] = 0x01;              // 命令的子命令
    data[1] = (uint8_t)count;    // LED 數量

    // 將每個LED的信息打包到數據中
    for (int i = 0; i < count; i++)
    {
        int index = pixels[i][0];          // LED的ID
        uint8_t r = (uint8_t)pixels[i][1]; // 紅色
        uint8_t g = (uint8_t)pixels[i][2]; // 綠色
        uint8_t b = (uint8_t)pixels[i][3]; // 藍色

        // 按照Python的數據順序打包 (ID-1, R, G, B)
        data[2 + i * 4] = (uint8_t)(index - 1); // LED ID 減 1
        data[3 + i * 4] = r;
        data[4 + i * 4] = g;
        data[5 + i * 4] = b;
    }

    // for (int i = 0; i < sizeof(data); i++)
    // {
    //     printf("%02x ", data[i]); // 打印每個字節
    // }
    // printf("\n");

    // 將打包好的數據寫入
    buf_write(board, PACKET_FUNC_RGB, data, sizeof(data));
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
