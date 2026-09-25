#ifndef SAFE_UTILS_H
#define SAFE_UTILS_H

//安全字符串拷贝（溢出内容自动截断）(返回1成功，返回0失败)
int SafeStrCopy(char* dest, const char* src, int maxDestLen);
//安全整数输入（prompt提示语）
int SafeIntInput(const char* prompt, int* inputNum, int min, int max);
//安全字符串输入
int SafeStrInput(const char* prompt, char* inputStr, int maxLen);
//空指针校验（全部非空返回1，有空返回0）
int CheckNullPtr(int ptrCount, ...);
//字符串是否全为空格
int IsStrAllSpace(const char* str);
//金额合法性校验
int CheckFeeValid(float fee);
//时间格式校验
int CheckTimeFormatValid(const char* timeStr);
//获取当前时间字符串（格式：YYYY-MM-DD_HH:MM:SS）
char* GetCurrentFullTimeStr();
//获取当前时间字符串（格式：YYYY-MM-DD）
char* GetCurrentDate();
//获取当前时间字符串（格式：MM-DD HH:MM）
char* GetCurrentTimeMMDDHHMM();
// 带预填充的字符串输入（原地编辑病历专用）
int SafeStrInputEdit(const char* tip, char* buffer, int bufferLen, const char* originContent);

#endif
