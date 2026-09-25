/* 系统操作日志模块 */
#ifndef LOG_H
#define LOG_H

// 日志级别定义
#define LOG_LEVEL_INFO  "INFO"    // 正常操作
#define LOG_LEVEL_ERROR "ERROR"   // 错误异常

// 初始化日志模块（系统启动调用1次）
void InitLog();
// 写入操作日志（日志级别、操作人、操作内容、操作结果）
void WriteLog(const char* level, const char* operater, const char* action, const char* result);
// 打印所有操作日志
void PrintAllLog();
// 清空所有日志
void ClearAllLog();

#endif // LOG_H