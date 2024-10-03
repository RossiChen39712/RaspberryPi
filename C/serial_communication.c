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

uint8_t crc8_table[256] = {
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

// 計算 CRC8 的函數
uint8_t checksum_crc8(const uint8_t *data, size_t len)
{
    uint8_t check = 0; // 初始校驗碼
    for (size_t i = 0; i < len; i++)
    {
        check = crc8_table[check ^ data[i]]; // 查找表格並更新校驗碼
    }
    return check & 0x00FF; // 返回校驗碼
}

// 串列發送函數
void buf_write(Board *board, uint8_t func, const uint8_t *data, int len)
{
    uint8_t buffer[256]; // 緩衝區，根據需求大小進行調整
    int index = 0;

    // 添加頭部信息
    buffer[index++] = 0xAA; // 開始字節 1
    buffer[index++] = 0x55; // 開始字節 2
    buffer[index++] = func; // 功能代碼

    // 添加數據長度
    buffer[index++] = len;

    // 複製數據到緩衝區
    memcpy(&buffer[index], data, len);
    index += len;

    // 計算 CRC8 校驗和
    uint8_t crc = checksum_crc8(&buffer[2], len + 2); // 從功能碼開始計算
    buffer[index++] = crc;

    // 打印發送的緩衝區
    printf("Sending buffer: ");
    for (int i = 0; i < index; i++)
    {
        printf("%02x ", buffer[i]);
    }
    printf("\n");

    // 發送數據
    write(board->fd, buffer, index);
    printf("Sent %d bytes to serial port\n", index);
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

    for (int i = 0; i < sizeof(data); i++)
    {
        printf("%02x ", data[i]); // 打印每個字節
    }
    printf("\n");

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
