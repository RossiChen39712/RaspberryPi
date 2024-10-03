#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdint.h>

#define DEVICE "/dev/ttyAMA0" // 在樹莓派上，UART 通常映射到 /dev/ttyAMA0
#define BAUDRATE B1000000     // 設置波特率
#define PACKET_FUNC_RGB 11    // 假設 RGB 的功能代碼為 11

// 定義 Board 結構，包含串列文件描述符
typedef struct
{
    int fd;
} Board;

// 配置串列埠
int configure_serial(const char *device)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY); // 開啟串列埠設備
    if (fd == -1)
    {
        perror("Failed to open serial port");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options); // 獲取當前配置

    // 設置波特率
    cfsetispeed(&options, BAUDRATE); // 設置輸入波特率
    cfsetospeed(&options, BAUDRATE); // 設置輸出波特率

    options.c_cflag |= (CLOCAL | CREAD); // 啟用接收，忽略調制解調器線
    options.c_cflag &= ~CSIZE;           // 清除字節大小設定
    options.c_cflag |= CS8;              // 設置為 8 位元數據
    options.c_cflag &= ~PARENB;          // 無奇偶校驗
    options.c_cflag &= ~CSTOPB;          // 一個停止位
    options.c_cflag &= ~CRTSCTS;         // 不使用硬體流控

    // 清除輸入/輸出緩衝區
    tcflush(fd, TCIFLUSH);
    // 設置新的串列埠參數
    tcsetattr(fd, TCSANOW, &options);

    return fd;
}

// 串列發送函數
void buf_write(Board *board, uint8_t func, const uint8_t *data, int len)
{
    uint8_t buffer[256]; // 緩衝區，根據需求大小進行調整
    buffer[0] = func;    // 封包的功能代碼（假設第一個字節是功能代碼）

    // 複製數據到緩衝區
    memcpy(&buffer[1], data, len);

    // 發送數據
    write(board->fd, buffer, len + 1);
    printf("Sent %d bytes to serial port\n", len + 1);
}

// 設置 RGB LED 的函數
void board_set_rgb(Board *board, int pixels[][4], int count)
{
    // 創建數據緩衝區
    uint8_t data[2 + count * 4]; // 2個字節頭信息 + 每個LED 4個字節(1個ID + 3個RGB)
    data[0] = 0x01;              // 命令的子命令（0x01假設是設置RGB的命令）
    data[1] = (uint8_t)count;    // LED 數量

    // 將每個LED的信息打包到數據中
    for (int i = 0; i < count; i++)
    {
        int index = pixels[i][0];          // LED的ID
        uint8_t r = (uint8_t)pixels[i][1]; // 紅色
        uint8_t g = (uint8_t)pixels[i][2]; // 綠色
        uint8_t b = (uint8_t)pixels[i][3]; // 藍色

        // 打包數據 (ID-1, R, G, B)
        data[2 + i * 4] = (uint8_t)(index - 1); // LED ID 減 1
        data[3 + i * 4] = r;
        data[4 + i * 4] = g;
        data[5 + i * 4] = b;
    }

    // 將打包好的數據寫入串列埠
    buf_write(board, PACKET_FUNC_RGB, data, sizeof(data));
}

int main()
{
    // 配置串列埠
    int serial_fd = configure_serial(DEVICE);
    if (serial_fd == -1)
    {
        return -1;
    }

    // 初始化 Board 結構
    Board board;
    board.fd = serial_fd;

    // 設置兩個 RGB LED 的顏色
    int pixels[2][4] = {
        {1, 255, 0, 0}, // LED 1 設為紅色
        {2, 0, 255, 0}  // LED 2 設為綠色
    };

    // 發送RGB設置命令
    board_set_rgb(&board, pixels, 2);

    // 關閉串列埠
    close(serial_fd);

    return 0;
}
