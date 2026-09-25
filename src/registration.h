#ifndef REGISTRATION_H
#define REGISTRATION_H

#include "common.h"

typedef struct _Registration {
    char regId[MAX_ID_LEN];
    char patientId[MAX_ID_LEN];
    char doctorId[MAX_ID_LEN];
    char deptId[MAX_ID_LEN];
    char createTime[MAX_DATA_LEN];
    float fee;
    RegStatus status; // 直接复用common.h里的RegStatus枚举
    struct _Registration* next;
} RegistrationNode, * RegistrationList;

// 【核心修复】删掉重复的PrintDoctorRegList声明，仅保留const版本
void InitRegistrationList();
int AddRegistration(RegistrationNode reg);
int CheckRegExist(char* regId);
void UpdateRegStatus(char* regId, RegStatus status);
void SaveRegistrationToFile(char* filename);
int LoadRegistrationFromFile(char* filename);
void FreeRegistrationList();
RegistrationList FindRegByPatient(char* patientId);
RegistrationList FindRegById(char* regId);
// 【新增】按医生ID获取下一个待诊患者（按挂号时间排序，状态为Waiting）
RegistrationList GetNextWaitingRegByDoctorId(const char* doctorId);
// 【唯一声明】PrintDoctorRegList参数加const，和实现完全匹配
void PrintDoctorRegList(const char* doctorId);

#endif
