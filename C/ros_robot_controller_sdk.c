#include "ros_robot_controller_sdk.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <gpiod.h>
#include <pthread.h>

// CRC8 查表數據
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

// 配置串列埠
int configure_serial(const char *device)
{
    int fd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1)
    {
        perror("Failed to open serial port");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    cfsetispeed(&options, B1000000); // 設置輸入波特率
    cfsetospeed(&options, B1000000); // 設置輸出波特率

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CRTSCTS;

    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &options);

    return fd;
}

// 計算 CRC8
uint8_t checksum_crc8(const uint8_t *data, size_t len)
{
    uint8_t check = 0;
    for (size_t i = 0; i < len; i++)
    {
        check = crc8_table[check ^ data[i]];
    }
    return check & 0x00FF;
}

// 串列發送
void buf_write(Board *board, uint8_t func, const uint8_t *data, int len)
{
    uint8_t buffer[256];
    int index = 0;

    buffer[index++] = 0xAA;
    buffer[index++] = 0x55;
    buffer[index++] = func;
    buffer[index++] = len;

    memcpy(&buffer[index], data, len);
    index += len;

    uint8_t crc = checksum_crc8(&buffer[2], len + 2);
    buffer[index++] = crc;

    write(board->fd, buffer, index);
}

// 設置 RGB LED
void board_set_rgb(Board *board, const RgbPixel *pixels, int count)
{
    uint8_t data[2 + count * 4];
    data[0] = 0x01;
    data[1] = (uint8_t)count;

    for (int i = 0; i < count; i++)
    {
        data[2 + i * 4] = (uint8_t)(pixels[i].id - 1);
        data[3 + i * 4] = pixels[i].r;
        data[4 + i * 4] = pixels[i].g;
        data[5 + i * 4] = pixels[i].b;
    }

    buf_write(board, PACKET_FUNC_RGB, data, 2 + count * 4);
}

// 封裝 RGB 色彩循環函數，增加時間間隔參數
void rgb_color_cycle(Board *board, int time_interval)
{
    int r = 255, g = 0, b = 0; // 從紅色開始
    int step = 5;              // 設置每次變化的步長

    while (1)
    {
        // 設置兩個 LED 為當前顏色
        RgbPixel color_pixels[2] = {
            {1, r, g, b},
            {2, r, g, b}};
        board_set_rgb(board, color_pixels, 2);

        usleep(time_interval * 1000); // 根據參數設置延遲，將毫秒轉換成微秒

        // RGB 色彩循環：逐步增減 R、G、B 值來實現平滑過渡
        if (r == 255 && g < 255 && b == 0)
        {
            g += step; // 紅 -> 黃 -> 綠
        }
        else if (g == 255 && r > 0 && b == 0)
        {
            r -= step; // 綠 -> 青
        }
        else if (g == 255 && b < 255 && r == 0)
        {
            b += step; // 青 -> 藍
        }
        else if (b == 255 && g > 0 && r == 0)
        {
            g -= step; // 藍 -> 紫
        }
        else if (b == 255 && r < 255 && g == 0)
        {
            r += step; // 紫 -> 紅
        }
        else if (r == 255 && b > 0 && g == 0)
        {
            b -= step; // 回到紅色
        }

        // 如果外部信號要退出，則退出循環（用全局變量控制）
        extern int start;
        if (!start)
        {
            break;
        }
    }
}

// 初始化 GPIO
struct gpiod_chip *gpio_init(const char *chip_name)
{
    struct gpiod_chip *chip = gpiod_chip_open_by_name(chip_name);
    if (!chip)
    {
        perror("Failed to open GPIO chip");
    }
    return chip;
}

// 設置 GPIO 中斷監聽
struct gpiod_line *gpio_setup_interrupt(struct gpiod_chip *chip, int pin, void (*callback)(int))
{
    struct gpiod_line *line = gpiod_chip_get_line(chip, pin);
    if (!line)
    {
        perror("Failed to get GPIO line");
        return NULL;
    }

    if (gpiod_line_request_both_edges_events(line, "gpio_interrupt") < 0)
    {
        perror("Failed to request line events");
        return NULL;
    }

    return line;
}

void *gpio_wait_for_interrupt(void *arg)
{
    struct gpiod_line *line = (struct gpiod_line *)arg;
    struct gpiod_line_event event;

    while (1)
    {
        int ret = gpiod_line_event_wait(line, NULL); // 等待中斷事件
        if (ret == 1)
        {
            gpiod_line_event_read(line, &event);
            if (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
            {
                printf("Falling edge detected on line %d!\n", gpiod_line_offset(line));

                // 呼叫對應的回調函數
                if (gpiod_line_offset(line) == 13) // 假設 13 為按鈕1
                {
                    button1_callback(GPIOD_LINE_EVENT_FALLING_EDGE);
                }
                else if (gpiod_line_offset(line) == 23) // 假設 23 為按鈕2
                {
                    button2_callback(GPIOD_LINE_EVENT_FALLING_EDGE);
                }
            }
        }
        else if (ret < 0)
        {
            perror("gpiod_line_event_wait");
        }

        // 若外部信號控制結束，則結束循環
        extern int start;
        if (!start)
        {
            break;
        }
    }

    return NULL;
}

// 按鈕1回調函數
void button1_callback(int event_type, Board *board)
{
    if (event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
    {
        const RgbPixel red_pixels[2] = {{1, 255, 0, 0}, {2, 255, 0, 0}}; // 使用定義好的顏色
        board_set_rgb(board, red_pixels, 2);
        printf("\n按下按鈕1，RGB 燈變為紅色\n");
    }
}

// 按鈕2回調函數
void button2_callback(int event_type, Board *board)
{
    if (event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
    {
        const RgbPixel blue_pixels[2] = {{1, 0, 0, 255}, {2, 0, 0, 255}}; // 使用定義好的顏色
        board_set_rgb(board, blue_pixels, 2);
        printf("\n按下按鈕2，RGB 燈變為藍色\n");
    }
}