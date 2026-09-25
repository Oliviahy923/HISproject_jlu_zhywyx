#include "safe_utils.h"
#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <conio.h>

//安全字符串拷贝
int SafeStrCopy(char* dest, const char* src, int maxDestLen) {
	if (dest == NULL || src == NULL || maxDestLen <= 0) {
		return 0;
	}

	strncpy(dest, src, maxDestLen - 1);
	dest[maxDestLen - 1] = '\0';
	return 1;
}

//安全整数输入
int SafeIntInput(const char* prompt, int* inputNum, int min, int max) {
	if (prompt == NULL || inputNum == NULL || min > max) {
		return 0;
	}

	int inputResult;

	while (1) {
		printf("%s（请输入%d-%d之间的整数）：", prompt, min, max);
		inputResult = scanf("%d", inputNum);
		// 修复明显BUG：仅输入失败时清空缓冲区，避免正常输入吞数据
		if (inputResult != 1) {
			while (getchar() != '\n');
		}
		if (inputResult == 1 && *inputNum >= min && *inputNum <= max) {
			// 清空剩余缓冲区
			while (getchar() != '\n');
			return 1;
		}
		else {
			printf("您输入的内容不正确！请输入%d-%d之间的整数，不要输入字母、符号！\n", min, max);
		}
	}
}

//安全字符输入（溢出处理，首尾空格删除）
int SafeStrInput(const char* prompt, char* inputStr, int maxLen) {
	if (prompt == NULL || inputStr == NULL || maxLen <= 0) {
		return 0;
	}

	while (1) {
		printf("%s（最多%d个字符）：", prompt, maxLen - 1);
		if (fgets(inputStr, maxLen, stdin) != NULL) {
			int len = (int)strlen(inputStr);
			// 去掉fgets自带的换行符
			if (len > 0 && inputStr[len - 1] == '\n') {
				inputStr[len - 1] = '\0';
				len--;
			}
			// 过滤首尾空格
			char* start = inputStr;
			while (isspace((unsigned char)*start)) start++;
			// 空内容校验
			if (*start == '\0') {
				printf("您输入的内容不能为空，请重新输入！\n");
				continue;
			}
			// 处理尾部空格
			char* end = inputStr + strlen(inputStr) - 1;
			while (end > start && isspace((unsigned char)*end)) end--;
			end[1] = '\0';
			// 把处理后的内容移到字符串开头
			memmove(inputStr, start, strlen(start) + 1);

			// 输入成功，退出循环，返回1
			return 1;
		}
		else {
			// 输入失败，清空缓冲区，重新输入
			while (getchar() != '\n');
			printf("输入失败，请重新输入！\n");
			continue;
		}
	}
}

//校验字符串是否全为空格
int IsStrAllSpace(const char* str) {
	if (str == NULL) {
		return 0;
	}
	while (*str != '\0') {
		if (!isspace((unsigned char)*str)) {
			return 0;           // 存在有非空格
		}
		str++;
	}
	return 1;
}

//空指针批量校验
int CheckNullPtr(int ptrCount, ...) {
	va_list args;
	va_start(args, ptrCount);
	for (int i = 0; i < ptrCount; i++) {
		void* ptr = va_arg(args, void*);
		if (ptr == NULL) {
			va_end(args);
			return 0; // 有一个空指针，返回0
		}
	}
	va_end(args);
	return 1; // 全部非空，返回1
}

//金额合法性校验
int CheckFeeValid(float fee) {
	if (fee < 0 || fee>100000) {
		return 0;
	}
	float feeCent = fee * 100;
	if (fabs(feeCent - round(feeCent)) > 0.0001) {
		return 0;
	}
	return 1;
}

//时间格式校验（YYYY-MM-DD）
int CheckTimeFormatValid(const char* timeStr) {
	if (timeStr == NULL || strlen(timeStr) != 10) {
		return 0;
	}

	if (timeStr[4] != '-' || timeStr[7] != '-') {    // 校验符号位
		return 0;
	}

	for (int i = 0; i < 10; i++) {
		if (i == 4 || i == 7) continue;
		if (!isdigit((unsigned char)timeStr[i])) {
			return 0;
		}
	}

	// 修复跨编译器BUG：删除GCC专属__attribute__，未使用变量直接注释
	// int year = atoi(timeStr);
	int month = atoi(timeStr + 5);
	int day = atoi(timeStr + 8);

	if (month < 1 || month>12) {
		return 0;
	}

	int maxDay[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	// 闰年判断
	if ((month % 4 == 0 && month % 100 != 0) || month % 400 == 0) {
		maxDay[1] = 29;
	}
	if (day<1 || day>maxDay[month - 1]) {
		return 0;
	}
	return 1;
}

//获取时间函数(格式：YYYY-MM-DD HH:MM:SS)
char* GetCurrentFullTimeStr() {
	static char timeStr[30] = { 0 };
	time_t now = time(NULL);
	struct tm* t = localtime(&now);
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", t);
	return timeStr;
}

//获取时间函数(格式：YYYY-MM-DD)
char* GetCurrentDate() {
	static char timeStr[20] = { 0 };
	time_t now = time(NULL);
	struct tm* t = localtime(&now);
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d", t);
	return timeStr;
}
// 获取时间函数（格式：MM-DD HH:MM）
char* GetCurrentTimeMMDDHHMM() {
	static char timeStr[30] = { 0 };
	time_t now = time(NULL);
	struct tm* t = localtime(&now);
	strftime(timeStr, sizeof(timeStr), "%m-%d %H:%M", t);
	return timeStr;
}

#include <windows.h>
// 带预填充的字符串输入（原地编辑，支持修改原有内容）
int SafeStrInputEdit(const char* tip, char* buffer, int bufferLen, const char* originContent) {
	if (tip == NULL || buffer == NULL || bufferLen <= 0 || originContent == NULL) {
		return 0;
	}
	memset(buffer, 0, bufferLen);

	// 打印提示语
	printf("%s：\n", tip);
	printf("原有内容：%s", originContent);

	// 获取控制台句柄与当前光标位置
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	COORD startPos = csbi.dwCursorPosition;

	// 预填充原有内容到输入缓冲区
	int originLen = (int)strlen(originContent);
	if (originLen > 0 && originLen < bufferLen - 1) {
		SafeStrCopy(buffer, originContent, bufferLen);
	}

	// 原地编辑核心逻辑
	int idx = originLen;
	int ch;
	while (1) {
		ch = _getch();
		// 回车键：结束输入
		if (ch == '\r' || ch == '\n') {
			printf("\n");
			break;
		}
		// 退格键：删除字符
		else if (ch == '\b' || ch == 0x7F) {
			if (idx > 0) {
				idx--;
				buffer[idx] = '\0';
				// 回退光标，删除字符
				COORD currPos = startPos;
				currPos.X += idx;
				SetConsoleCursorPosition(hConsole, currPos);
				printf(" ");
				SetConsoleCursorPosition(hConsole, currPos);
			}
		}
		// 可打印字符：输入内容
		else if (isprint(ch) && idx < bufferLen - 1) {
			buffer[idx] = (char)ch;
			printf("%c", ch);
			idx++;
		}
	}

	buffer[idx] = '\0';
	return 1;
}
