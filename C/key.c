#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gpiod.h>
#include <signal.h>
#include "ros_robot_controller_sdk.h"

int start = 1;
Board board;

// 信號處理函數，用來捕捉 Ctrl+C 信號，並關閉 RGB 燈
void handle_sigint(int sig)
{
    start = 0;
    RgbPixel off_pixels[2] = {{1, 0, 0, 0}, {2, 0, 0, 0}};
    board_set_rgb(&board, off_pixels, 2); // 關閉所有燈
    printf("\n接收到 Ctrl + C 信號，關閉RGB燈並退出程序\n");
}

int main()
{
    printf("Tips:\n * 按下Ctrl+C可关闭此次程序运行，若失败请多次尝试！\n");

    // 註冊信號處理函數，捕捉 Ctrl+C 信號
    signal(SIGINT, handle_sigint);

    // 初始化 Board 結構
    int serial_fd = configure_serial("/dev/ttyAMA0");
    if (serial_fd == -1)
    {
        perror("Failed to open serial port");
        return -1;
    }
    board.fd = serial_fd;

    // GPIO 設置
    struct gpiod_chip *chip;
    struct gpiod_line *key1, *key2;

    // 設定 GPIO4 芯片中的第 13 和 23 引腳
    chip = gpiod_chip_open_by_name("gpiochip4");
    if (!chip)
    {
        perror("Failed to open GPIO chip");
        return -1;
    }

    key1 = gpiod_chip_get_line(chip, 13); // key1 綁定 GPIO13
    if (!key1)
    {
        perror("Failed to get key1 line");
        gpiod_chip_close(chip);
        return -1;
    }
    gpiod_line_request_input_flags(key1, "key1", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);

    key2 = gpiod_chip_get_line(chip, 23); // key2 綁定 GPIO23
    if (!key2)
    {
        perror("Failed to get key2 line");
        gpiod_chip_close(chip);
        return -1;
    }
    gpiod_line_request_input_flags(key2, "key2", GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);

    // 主循環，根據按鍵狀態設置 RGB 顏色
    while (start)
    {
        int key1_state = gpiod_line_get_value(key1);
        int key2_state = gpiod_line_get_value(key2);

        if (key1_state == 0)
        {
            // 按鍵1被按下，設置 RGB 為紅色
            RgbPixel red_pixels[2] = {{1, 255, 0, 0}, {2, 255, 0, 0}};
            board_set_rgb(&board, red_pixels, 2);
        }

        if (key2_state == 0)
        {
            // 按鍵2被按下，設置 RGB 為藍色
            RgbPixel blue_pixels[2] = {{1, 0, 0, 255}, {2, 0, 0, 255}};
            board_set_rgb(&board, blue_pixels, 2);
        }

        printf("\rkey1: %d key2: %d", key1_state, key2_state); // 打印按鍵狀態
        fflush(stdout);
        usleep(1000); // 睡眠 1 毫秒
    }

    // 清理並關閉 GPIO
    gpiod_chip_close(chip);
    close(serial_fd);

    return 0;
}
