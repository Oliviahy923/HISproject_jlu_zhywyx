#ifndef PRESCRIPTION_H
#define PRESCRIPTION_H

// 包含通用定义头文件，获取PrescStatus、MAX_ID_LEN等定义
#include "common.h"

// 处方结构体
typedef struct Prescription {
    char prescId[MAX_ID_LEN];             //处方ID
    char patientId[MAX_ID_LEN];           //关联患者ID
    char doctorId[MAX_ID_LEN];            //开方医生ID
    char medicineId[MAX_ID_LEN];          //关联药品ID
    int quantity;                         //数量
    float totalFee;                       //总费用
    PrescStatus status;                   //处方状态（直接用common.h里的枚举）
    char createTime[MAX_DATA_LEN];        //开方时间
    char dispenseTime[MAX_DATA_LEN];      //发药时间
    struct Prescription* next;            //下一节点
} PrescNode, * PrescList;


// 【后面的函数声明完全保留，不需要修改】
//初始化&内存管理
void InitPrescriptionList();
void FreePrescriptionList();

//核心校验函数
int CheckPrescriptionExist(const char* prescId);
int CheckPrescriptionUndispensed(const char* prescId);
int PrescCheckMedicineHasUndispensedPresc(const char* medicineId);

//增删改核心函数
int AddPrescription();
int ModifyPrescription();
int DeletePrescription();
int DispensePrescription();

//查询功能
void QueryPrescriptionByPatientId(const char* patientId);
void QueryPrescriptionByMedicineId(const char* medicineId);
void QueryPrescriptionByTimeRange();
void PrintAllPrescription();
// 查询患者待缴费处方
int QueryUnpaidPrescriptionByPatientId(const char* patientId);
//持久化储存
int SavePrescriptionToFile(const char* fileName);
int LoadPrescriptionFromFile(const char* fileName);

// 获取所有处方总费用
float GetTotalPrescriptionFee();
// 更新处方状态为已缴费
int UpdatePrescriptionToPaid(const char* prescId);
// 外部链表头声明
extern PrescList prescHead;
// 打印所有处方简略信息（ID+患者ID+状态）
void PrintAllPrescriptionBrief(void);
//增删改核心函数
int AddPrescription();
int ModifyPrescription();
// 仅管理员可删除处方，医生仅可作废处方
int DeletePrescription();
// 处方作废（含冲账回退逻辑，医生/管理员均可调用）
int CancelPrescription(const char* prescId);
int DispensePrescription();

#endif
