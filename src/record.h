#ifndef RECORD_H
#define RECORD_H

#include "common.h"

typedef enum {
	REGISTER,
	CONSULT,
	EXAMINE,
	HOSPITALIZE,
	HOSPITAL_DEPOSIT,  // 住院押金缴费
	HOSPITAL_DISCHARGE // 出院结算
} RecordType;

typedef struct MedicalRecord {
	char recordId[MAX_ID_LEN];          // 记录唯一ID
	char patientId[MAX_ID_LEN];         // 关联患者ID
	char doctorId[MAX_ID_LEN];          // 关联医生ID
	char deptId[MAX_ID_LEN];            // 关联科室ID
	RecordType type;                    // 记录类型
	char createTime[MAX_DATE_LEN];      // 时间
	char detail[MAX_DETAIL_LEN];        // 详情
	float fee;                          // 费用（精确到分）
	char prescriptionId[MAX_ID_LEN];    // 关联处方ID
	struct MedicalRecord* next;
} RecordNode, * RecordList;

extern RecordList recordHead;
// 对外暴露链表头，用于统计
RecordList GetRecordListHead();

void InitRecordList();
int AddRecord(RecordNode newRecord);
int SaveRecordToFile(char* fileName);
int LoadRecordFromFile(char* fileName);
void FreeRecordList();
int CheckRecordValid(RecordNode* newRecord, int mode);
int AddRegisterRecord(RecordNode newRecord);
int AddConsultRecord(RecordNode newRecord);
int AddExamineRecord(RecordNode newRecord);
int AddHospitalizeRecord(RecordNode newRecord);
int ModifyRecord(const char* recordId, RecordNode newData);
int DeleteRecord(const char* recordId);
void QueryRecordByPatientId(const char* patientId);
void QueryRecordByDoctorId(const char* doctorId);
void QueryRecordByTimeRange(char* startTime, char* endTime);
void PrintAllRecord();
void SortRecordByTime(RecordList list);
int CompareTime(const char* time1, const char* time2);
// 医疗记录总费用(精确到分)
float GetTotalRecordFee();
// 打印所有记录简略信息（ID+患者ID+类型）
void PrintAllRecordBrief(void);

#endif
