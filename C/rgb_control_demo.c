#include <stdio.h>
#include <unistd.h>
#include "ros_robot_controller_sdk.h" // 假設有對應的 SDK 支援

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