#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>

#define DEVICE "/dev/serial0" // 在樹莓派上，UART 通常映射到 /dev/serial0
#define BAUDRATE B9600        // 波特率設定，根據設備需求調整

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

// 發送資料到串列埠
void serial_send(int fd, const char *data)
{
    int len = strlen(data);
    int n = write(fd, data, len);
    if (n < 0)
    {
        perror("Error writing to serial port");
    }
    else
    {
        printf("Sent: %s\n", data);
    }
}

// 從串列埠接收資料
void serial_receive(int fd)
{
    char buffer[256];
    int n = read(fd, buffer, sizeof(buffer) - 1);
    if (n < 0)
    {
        perror("Error reading from serial port");
    }
    else if (n == 0)
    {
        printf("No data available to read\n");
    }
    else
    {
        buffer[n] = '\0'; // 添加字符串終止符
        printf("Received: %s\n", buffer);
    }
}

int main()
{
    // 配置串列埠
    int serial_fd = configure_serial(DEVICE);
    if (serial_fd == -1)
    {
        return -1;
    }

    // 發送資料到外部設備
    serial_send(serial_fd, "Hello from Raspberry Pi!");

    // 等待 1 秒，給外部設備時間回應
    sleep(1);

    // 接收來自外部設備的資料
    serial_receive(serial_fd);

    // 關閉串列埠
    close(serial_fd);

    return 0;
}
