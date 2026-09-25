#include <stdio.h>
#include <stdlib.h>
#include "system.h"


int main() {

    system("chcp 936 > nul");
    // 【必须加】关闭输出缓冲区，避免打印延迟
    setvbuf(stdout, NULL, _IONBF, 0);

    // 系统全模块初始化
    SystemInit();

    // 系统主循环（仅用户主动确认退出时才会返回）
    SystemLoop();

    return 0;

}
