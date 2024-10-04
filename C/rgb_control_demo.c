#include <stdio.h>
#include <unistd.h>
#include "ros_robot_controller_sdk.h"

#define DEVICE "/dev/ttyAMA0" // 設備串列埠

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

    int start = 1; // 假設開始時為 1，代表需要執行

    while (true)
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

        if (!start)
        {
            // 關閉 LED
            const RgbPixel off_pixels[2] = {rgb1_off, rgb2_off};
            board_set_rgb(&board, off_pixels, 2);
            printf("已關閉\n");
            break;
        }
    }

    // 關閉串列埠
    close(serial_fd);

    return 0;
}