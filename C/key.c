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

    // 設置按鍵中斷
    struct gpiod_line *button1_line = gpio_setup_interrupt(chip, 13, button1_callback);
    struct gpiod_line *button2_line = gpio_setup_interrupt(chip, 23, button2_callback);

    if (!button1_line || !button2_line)
    {
        gpiod_chip_close(chip);
        return -1;
    }

    // 創建兩個線程來等待按鍵中斷
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, (void *)gpio_wait_for_interrupt, button1_line);
    pthread_create(&thread2, NULL, (void *)gpio_wait_for_interrupt, button2_line);

    // 等待信號觸發退出
    while (start)
    {
        sleep(1); // 主線程保持運行
    }

    // 清理資源
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    gpiod_chip_close(chip);
    close(board.fd);

    return 0;
}
