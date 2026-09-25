// 系统标准头文件（优先引入，C语言规范）
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <stdarg.h>
// 自定义头文件
#include "common.h"
#include "safe_utils.h"
#include "log.h"

static int logLocaleInited = 0;
#define INIT_LOG_LOCALE() do { if(!logLocaleInited) { setlocale(LC_ALL, "zh_CN.GBK"); logLocaleInited=1; } } while(0)

// 日志配置宏定义
#define LOG_FILE_NAME     "system_log.txt"    // 修复文件名笔误
#define MAX_LOG_LINE_LEN  1024                // 单条日志最大长度

// 初始化日志模块
void InitLog() {
    INIT_LOG_LOCALE(); // 新增：调用编码初始化
    // ==================== 修改1：删除 ,ccs=GBK ====================
    FILE* fp = fopen(LOG_FILE_NAME, "a");
    if (fp == NULL) {
        printf("Warning: Log file creation failed, subsequent operations will not be recorded!\n");
        return;
    }

    fprintf(fp, "\n=======================System Start[%s]========================\n", GetCurrentFullTimeStr());
    fclose(fp);
}

// 写入操作日志函数
void WriteLog(const char* level, const char* operater, const char* action, const char* result) {
    INIT_LOG_LOCALE();
    if (!CheckNullPtr(4, level, operater, action, result)) {
        return;
    }

    // ==================== 修改2：删除 ,ccs=GBK ====================
    FILE* fp = fopen(LOG_FILE_NAME, "a");
    if (fp == NULL) {
        printf("Warning: Log write failed!\n");
        return;
    }

    char logLine[MAX_LOG_LINE_LEN] = { 0 };
    snprintf(logLine, MAX_LOG_LINE_LEN, "[%s] [%s] Operator: %s | Action: %s | Result: %s\n",
        GetCurrentFullTimeStr(), level, operater, action, result);

    fputs(logLine, fp);
    fclose(fp);
}

// 打印操作日志函数
void PrintAllLog() {
    INIT_LOG_LOCALE();
    // ==================== 修改3：删除 ,ccs=GBK ====================
    FILE* fp = fopen(LOG_FILE_NAME, "r");
    if (fp == NULL) {
        printf("No system operation logs!\n");
        WriteLog(LOG_LEVEL_ERROR, "System", "Print Log", "Failed, log file does not exist");
        return;
    }

    char buffer[MAX_LOG_LINE_LEN] = { 0 };
    printf("\n==================System Operation Logs======================\n");
    while (fgets(buffer, MAX_LOG_LINE_LEN, fp) != NULL) {
        printf("%s", buffer);
    }
    printf("============================================================\n");

    fclose(fp);
    WriteLog(LOG_LEVEL_INFO, "Admin", "Print Log", "Success");
}

// 清空日志函数
void ClearAllLog() {
    INIT_LOG_LOCALE();
    // ==================== 修改4：删除 ,ccs=GBK ====================
    FILE* fp = fopen(LOG_FILE_NAME, "w");
    if (fp == NULL) {
        printf("Log clear failed!\n");
        WriteLog(LOG_LEVEL_ERROR, "Admin", "Clear Log", "Failed, file open failed");
        return;
    }

    fprintf(fp, "======================File Cleared[%s]=====================\n", GetCurrentFullTimeStr());
    fclose(fp);

    printf("Logs have been cleared!\n");
    WriteLog(LOG_LEVEL_INFO, "Admin", "Clear Log", "Success");
}

