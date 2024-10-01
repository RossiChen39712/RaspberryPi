#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <unistd.h>
#include "ros_robot_controller_sdk.h" // 假設有對應的 SDK 支援

bool start = true;

// 關閉前處理函數
void Stop(int signum)
{
    start = false;
    printf("關閉中...\n");
}

int main()
{
    printf("----------------------------------------------------------\n
    Tips:按下Ctrl+C可關閉此程式運行，若失敗請多次嘗試！\n
    ----------------------------------------------------------\n");

    // 設定信號處理
    signal(SIGINT, Stop);

    // 建立 Board 類別
    rrc_Board board;

    // 初始化 Board，假設有對應的函數
    rrc_board_init(&board);

    // 先將所有燈關閉
    int rgb_off[2][4] = {{1, 0, 0, 0}, {2, 0, 0, 0}};
    rrc_set_rgb(&board, rgb_off, 2);

    while (1) {
        // 設置兩個燈為紅色
        int rgb_red[2][4] = {{1, 255, 0, 0}, {2, 255, 0, 0}};
        rrc_set_rgb(&board, rgb_red, 2);
        sleep(1);

        // 設置兩個燈為綠色
        int rgb_green[2][4] = {{1, 0, 255, 0}, {2, 0, 255, 0}};
        rrc_set_rgb(&board, rgb_green, 2);
        sleep(1);

        // 設置兩個燈為藍色
        int rgb_blue[2][4] = {{1, 0, 0, 255}, {2, 0, 0, 255}};
        rrc_set_rgb(&board, rgb_blue, 2);
        sleep(1);

        // 設置兩個燈為黃色
        int rgb_yellow[2][4] = {{1, 255, 255, 0}, {2, 255, 255, 0}};
        rrc_set_rgb(&board, rgb_yellow, 2);
        sleep(1);

        if (!start)
        {
            // 關閉所有燈
            rrc_set_rgb(&board, rgb_off, 2);
            printf("已關閉\n");
            break;
        }
    }

    return 0;
}
