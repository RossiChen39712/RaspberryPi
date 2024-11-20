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

    // 開始 RGB Color Cycle，並傳入時間間隔參數
    rgb_color_cycle(&board, 50);

    // 當 Ctrl + C 被按下時，將所有燈關閉
    RgbPixel off_pixels[2] = {rgb1_off, rgb2_off};
    board_set_rgb(&board, off_pixels, 2);
    printf("RGB 已關閉\n");

    // 關閉串列埠
    close(serial_fd);

    return 0;
}
