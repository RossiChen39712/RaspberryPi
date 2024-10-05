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

void rgb_color_cycle(Board *board)
{
    int r = 255, g = 0, b = 0; // 從紅色開始
    int step = 5;              // 設置每次變化的步長

    while (start)
    {
        // 設置兩個 LED 為當前顏色
        RgbPixel color_pixels[2] = {
            {1, r, g, b},
            {2, r, g, b}};
        board_set_rgb(board, color_pixels, 2);
        usleep(100000); // 每 100 毫秒更新一次顏色

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
    }
}

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

    // 開始 RGB Color Cycle
    rgb_color_cycle(&board);

    // 當 Ctrl + C 被按下時，將所有燈關閉
    RgbPixel off_pixels[2] = {rgb1_off, rgb2_off};
    board_set_rgb(&board, off_pixels, 2);
    printf("RGB 已關閉\n");

    // 關閉串列埠
    close(serial_fd);

    return 0;
}
