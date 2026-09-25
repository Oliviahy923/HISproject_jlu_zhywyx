#include <stdio.h>
#include "common.h"
#include <string.h>
#include <ctype.h>

// 判断字符串是否为纯数字
int IsNumber(char* str)
{
    if (!str || *str == '\0')
        return 0;

    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] < '0' || str[i] > '9')
            return 0;
    }
    return 1;
}

// 判断字符串是否包含数字
int HasNumber(char* str)
{
    if (!str || *str == '\0')
        return 0;

    for (int i = 0; str[i] != '\0'; i++)
    {
        if (str[i] >= '0' && str[i] <= '9')
            return 1;
    }
    return 0;
}

// 判断字符串是否仅包含字母和数字
int IsAlphaNumber(char* str)
{
    if (!str || *str == '\0')
        return 0;

    for (int i = 0; str[i] != '\0'; i++)
    {
        if (!((str[i] >= '0' && str[i] <= '9') ||
            (str[i] >= 'a' && str[i] <= 'z') ||
            (str[i] >= 'A' && str[i] <= 'Z')))
            return 0;
    }
    return 1;
}

// 新增：全系统通用回车等待函数
void WaitEnter(void) {
    printf("\n");
    printf("【提示】按回车键继续...\n");
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}