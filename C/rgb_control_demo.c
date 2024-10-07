#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include "ros_robot_controller_sdk.h"

int start = 1; // 控制程式運行的全局變數

// 信號處理函數
void handle_sigint(int sig)
{
    start = 0; // 當接收到 Ctrl + C 信號時，將 start 設為 0，退出迴圈
    printf("\n接收到 Ctrl + C 信號，準備復歸並退出程式\n");
}

#define DEVICE "/dev/ttyAMA0" // 設備串列埠

int main()
{
    // 註冊 SIGINT 信號處理函數 (Ctrl + C)
    signal(SIGINT, handle_sigint);

    // 配置串列埠
    int serial_fd = configure_serial(DEVICE);
    if (serial_fd == -1)
    {
        return -1;
    }

    // 初始化 Board 結構
    Board board;
    board.fd = serial_fd;

    while (start)
    {
        // 設置 RGB LED 為紅色
        const RgbPixel red_pixels[2] = {rgb1_red, rgb2_red};
        board_set_rgb(&board, red_pixels, 2);
        sleep(1);

        // 設置 RGB LED 為綠色
        const RgbPixel green_pixels[2] = {rgb1_green, rgb2_green};
        board_set_rgb(&board, green_pixels, 2);
        sleep(1);

        // 設置 RGB LED 為藍色
        const RgbPixel blue_pixels[2] = {rgb1_blue, rgb2_blue};
        board_set_rgb(&board, blue_pixels, 2);
        sleep(1);

        // 設置 RGB LED 為黃色
        const RgbPixel yellow_pixels[2] = {rgb1_yellow, rgb2_yellow};
        board_set_rgb(&board, yellow_pixels, 2);
        sleep(1);
    }

    // 關閉 LED 並釋放資源
    const RgbPixel off_pixels[2] = {rgb1_off, rgb2_off};
    board_set_rgb(&board, off_pixels, 2);
    printf("RGB 已關閉\n");

    // 關閉串列埠
    close(serial_fd);

    return 0;
}
