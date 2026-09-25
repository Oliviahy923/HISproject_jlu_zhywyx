#ifndef BED_H
#define BED_H



////////
// 系统标准头文件 (优先引入)
#include <stdio.h>
// 自定义公共头文件（确保common.h里定义了MAX_ID_LEN/MAX_NAME_LEN/MAX_DATA_LEN/MAX_DETAIL_LEN）
#include "common.h"

// ==============================================
//                床位状态宏定义（解决未定义报错）
// ==============================================
#define BED_FREE        0   // 床位空闲
#define BED_OCCUPIED    1   // 床位已占用

// ==============================================
//                床位数据结构定义（和.c文件100%匹配）
// ==============================================
typedef struct BedNode {
    char bedId[MAX_ID_LEN];
    char wardType[MAX_NAME_LEN];  // 修正：和.c文件统一，替换原roomNo
    char deptId[MAX_ID_LEN];
    int status; // 0=空闲，1=已占用
    char patientId[MAX_ID_LEN];
    char inTime[MAX_DATA_LEN];
    struct BedNode* next;
} BedNode, * BedList;

// ==============================================
//                床位模块函数声明
// ==============================================

// 1. 初始化与内存管理
void InitBedList(void);
void FreeBedList(void);

// 2. 核心校验函数
int CheckBedExist(const char* bedId);
int CheckBedAvailable(const char* bedId);

// 3. 增删改核心操作
// 带参函数：供业务模块（如patient.c）调用
int AssignBedToPatient(const char* bedId, const char* patientId, const char* inTime);
int ReleaseBed(const char* bedId);
// 交互式函数：供菜单模块调用
int AddBed(void);
int AssignBedToPatientInteractive(void);
int ReleaseBedInteractive(void);
int ModifyBed(void);
int DeleteBed(void);

// 4. 床位查询功能
void QueryBedByWardType(void);
void QueryFreeBedByDept(const char* deptId);
void PrintAllBed(void);
void PrintFreeBed(void);

// 5. 数据持久化
int SaveBedToFile(const char* fileName);
int LoadBedFromFile(const char* fileName);

// 6. 统计与报表
void GenerateBedUsageReport(FILE* fp);
void StatHospitalBedUsageRate(FILE* fp);

// 打印所有床位简略信息（ID+类型+状态）
void PrintAllBedBrief(void);

// 4. 床位查询功能
void QueryBedByWardType(void);
void QueryFreeBedByDept(const char* deptId);
void PrintAllBed(void);
void PrintFreeBed(void);
// 床位智能匹配：按科室匹配空闲床位，返回匹配的床位ID，无匹配返回NULL
char* AutoMatchFreeBedByDept(const char* deptId);

#endif // BED_H