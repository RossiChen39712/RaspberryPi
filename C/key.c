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

    // 初始化 GPIO
    struct gpiod_chip *chip = gpio_init("gpiochip4");
    if (!chip)
    {
        return -1;
    }

    while (start)
    {
        // 讀取按鍵狀態
        int key1_state = gpio_read_key(chip, 13);
        int key2_state = gpio_read_key(chip, 23);

        if (key1_state == 0)
        {
            // 設置 RGB 燈為紅色
            RgbPixel red_pixels[2] = {{1, 255, 0, 0}, {2, 255, 0, 0}};
            board_set_rgb(&board, red_pixels, 2);
        }

        if (key2_state == 0)
        {
            // 設置 RGB 燈為藍色
            RgbPixel blue_pixels[2] = {{1, 0, 0, 255}, {2, 0, 0, 255}};
            board_set_rgb(&board, blue_pixels, 2);
        }

        printf("\rkey1: %d key2: %d", key1_state, key2_state);
        fflush(stdout);

        usleep(1000); // 延遲 1 毫秒
    }

    // 清理並關閉 GPIO
    gpiod_chip_close(chip);
    close(serial_fd);

    return 0;
}
