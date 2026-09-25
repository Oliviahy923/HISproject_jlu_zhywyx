#ifndef PATIENT_H
#define PATIENT_H

#include "common.h"

typedef struct Patient
{
    char patientId[MAX_ID_LEN];    // 患者ID
    char name[MAX_NAME_LEN];       // 姓名
    char gender[10];               // 性别
    int age;                       // 年龄
    char phone[MAX_ID_LEN];        // 电话
    int patientType;               // 1=门诊，2=住院
    char bedId[MAX_ID_LEN];        // 关联床位
    int inHospitalStatus;          // 住院状态
    float depositFee;              // 住院押金
    char inTime[MAX_DATA_LEN];     // 入院时间
    char outTime[MAX_DATA_LEN];    // 出院时间
    struct Patient* next;
} PatientNode, * PatientList;

// ==================== 函数声明（只读入参统一加const，和实现完全匹配）====================
// 初始化
void InitPatientList(void);
// 保存到txt文件
int SavePatientToFile(const char* filename);
// 从txt加载患者数据
int LoadPatientFromFile(const char* filename);
// 释放链表内存
void FreePatientList(void);
// 合法性校验
int CheckPatientValid(const PatientNode* newPatient, int mode);
// 增加患者
int AddPatient(PatientNode newPatient);
// 修改患者
int ModifyPatient(const char* patientId, PatientNode newData);
// 删除患者
int DeletePatient(const char* patientId);
// 检查患者Id是否存在
int CheckPatientExist(const char* patientId);
// 按Id查患者详情
void QueryPatientById(const char* patientId);
// 姓名模糊查询
void QueryPatientByName(const char* name);
// 打印所有患者信息
void PrintAllPatient(void);
// 统计患者视角报表
void StatPatientFeeDetail(const char* input);
// 更新患者类型
int UpdatePatientType(const char* patientId, int newType, const char* bedId, const char* inTime);
// 交互式新增患者
int AddPatientInteractive(void);
// 根据姓名/ID获取患者的真实ID
char* GetPatientRealId(const char* input);
// 按姓名检查患者是否存在
int CheckPatientExistByName(const char* patientName);
// 【对外暴露】医生接诊校验函数，解决prescription.c链接错误
int IsPatientTreatedByDoctor(const char* patientId, const char* doctorId);
//根据ID查找患者
PatientList FindPatientById(const char* patientId);
// 患者自助修改个人信息（登录后调用）
int ModifyPatientSelfInfo(const char* patientId);
// ==================== 住院流程核心函数 ====================
// 住院登记（含床位智能匹配）
int InHospitalRegister(const char* patientId, const char* deptId, const char* doctorId);
// 住院押金缴费
int PayHospitalDeposit(const char* patientId);
// 办理入院（床位分配+患者状态联动）
int HandleInHospitalCheckIn(const char* patientId);
// 出院结算办理
int HandleHospitalDischarge(const char* patientId, float settleFee);
void PrintAllPatientBrief(void);

#endif // PATIENT_H