#ifndef DOCTOR_H
#define DOCTOR_H

#include "common.h"

// 医生信息结构体（两个版本完全一致，无字段冲突）
typedef struct Doctor {
    char doctorId[MAX_ID_LEN];       // 医生唯一ID
    char name[MAX_NAME_LEN];         // 医生姓名
    char deptId[MAX_ID_LEN];         // 所属科室ID
    char title[MAX_NAME_LEN];        // 职称
    char phone[MAX_ID_LEN];          // 联系方式
    int registerLimit;               // 每日挂号限额
    int todayRegisterCount;          // 当日已挂号数
    struct Doctor* next;
} DoctorNode, * DoctorList;

// ==================== 模块初始化与内存管理 ====================
// 初始化医生链表
void InitDoctorList(void);
// 释放链表内存
void FreeDoctorList(void);

// ==================== 数据持久化 ====================
// 医生数据保存到txt文件
int SaveDoctorTofile(const char* filename);
// 从txt文件加载医生数据
int LoadDoctorFromFile(const char* fileName);

// ==================== 数据校验与增删改 ====================
// 医生数据合法性校验
int CheckDoctorValid(const DoctorNode* newDoctor, int mode);
// 新增医生（自动校验合法性，isLoad标记是否为文件加载）
int AddDoctor(DoctorNode newDoctor, int isLoad);
// 修改医生信息
int ModifyDoctor(const char* doctorId, DoctorNode newData);
// 删除医生
int DeleteDoctor(const char* doctorId);

// ==================== 存在性校验函数 ====================
// 按ID校验医生是否存在（1=存在，0=不存在）
int CheckDoctorExist(const char* doctorId);
// 按姓名精确校验医生是否存在（1=存在，0=不存在）
int CheckDoctorExistByName(const char* name);

// ==================== 查询与打印函数 ====================
// 按ID查询医生详情并打印
void QueryDoctorById(const char* doctorId);
// 按科室ID查询该科室所有医生并打印
void QueryDoctorByDept(const char* deptId);
// 按姓名模糊查询医生并打印
void QueryDoctorByName(const char* name);
// 打印所有医生信息
void PrintAllDoctor(void);
// 按姓名精确匹配医生，返回对应的医生ID（找不到返回NULL）
char* GetDoctorIdByName(const char* name);

// ==================== 统计与业务函数 ====================
// 统计指定时间范围内医生接诊量
void StatDoctorVisitCount(const char* doctorId, const char* startTime, const char* endTime);
// 统计指定科室患者分布情况
void StatDeptPatientDistribution(const char* deptId);
// 医生当日挂号计数+1，自动校验限额（1=成功，0=已满）
int AddDoctorRegisterCount(const char* doctorId);
// 医生当日挂号计数-1，自动校验下限（1=成功，0=异常）
int SubDoctorRegisterCount(const char* doctorId);



// ==================== 【仅新增！缺失函数声明】根治所有报错  ====================
// 完全对应你doctor.c内部的 FindDoctorById 实现，返回值、参数const全部严格对齐
DoctorList GetDoctorById(const char* doctorId);
// 按科室ID+姓名关键词模糊查询医生（仅当前科室），统一格式打印，返回匹配数量，唯一匹配时输出医生ID到outDoctorId
int QueryDoctorByDeptAndName(const char* deptId, const char* nameKeyword, char* outDoctorId, int outIdLen);

// 打印所有医生简略信息（ID+姓名+科室）
void PrintAllDoctorBrief(void);

#endif
