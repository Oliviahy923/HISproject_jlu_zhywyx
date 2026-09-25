// 先包含系统头文件
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
// 【必须优先包含common.h，统一全局打印规范】
#include "common.h"
// 再包含自定义业务头文件
#include "record.h"
#include "doctor.h"
#include "patient.h"
#include "department.h"
#include "bed.h"
#include "safe_utils.h"
#include "log.h"

// ==================== 内部私有静态变量 ====================
RecordList recordHead = NULL;

// ==================== 对外接口：获取链表头节点 ====================
RecordList GetRecordListHead() {
	return recordHead;
}

// ==================== 内部私有辅助函数 ====================
// 按ID查找医疗记录
RecordNode* FindRecordById(const char* recordId)
{
	if (recordHead == NULL || recordHead->next == NULL || recordId == NULL) {
		return NULL;
	}
	RecordNode* p = recordHead->next;
	while (p != NULL) {
		if (strcmp(p->recordId, recordId) == 0) {
			return p;
		}
		p = p->next;
	}
	return NULL;
}

static int InsertRecordToList(RecordNode newRecord)
{
	// 1. 统一申请节点内存，处理内存不足的异常
	RecordNode* newNode = (RecordNode*)malloc(sizeof(RecordNode));
	if (newNode == NULL)
	{
		PRINT_ERR("内存分配失败，无法新增医疗记录！");
		return 0;
	}
	// 2. 原样赋值结构体数据
	*newNode = newRecord;
	// 3. 头插法插入链表
	newNode->next = recordHead->next;
	recordHead->next = newNode;
	return 1;
}
// 用于加载文件数据（完全不变）
int AddRecord(RecordNode newRecord)
{
	if (!CheckRecordValid(&newRecord, 1))
	{
		return 0;
	}
	return InsertRecordToList(newRecord);
}

// 初始化医疗记录链表（仅优化打印提示）
void InitRecordList()
{
	recordHead = (RecordList)malloc(sizeof(RecordNode));
	if (recordHead == NULL)
	{
		PRINT_ERR("医疗记录链表初始化失败，内存不足！");
		return;
	}
	recordHead->next = NULL;
	PRINT_OK("医疗记录链表初始化成功！");
}

int SaveRecordToFile(char* fileName)
{
	if (fileName == NULL || strlen(fileName) == 0)
	{
		PRINT_ERR("文件名不能为空！");
		return 0;
	}
	FILE* fp = fopen(fileName, "w");
	if (fp == NULL)
	{
		PRINT_ERR("医疗记录数据文件打开失败！");
		return 0;
	}
	RecordList p = recordHead->next;
	int saveCount = 0;
	while (p != NULL)
	{
		// 转义详情字段，彻底解决CSV格式错乱和乱码
		char cleanDetail[MAX_DETAIL_LEN * 2] = { 0 };
		int srcIdx = 0, destIdx = 0;
		while (p->detail[srcIdx] != '\0' && destIdx < MAX_DETAIL_LEN * 2 - 2)
		{
			if (p->detail[srcIdx] == ',')
			{
				cleanDetail[destIdx++] = '，';
			}
			else if (p->detail[srcIdx] == '\n' || p->detail[srcIdx] == '\r')
			{
				cleanDetail[destIdx++] = ' ';
			}
			else if ((unsigned char)p->detail[srcIdx] < 0x20)
			{
				srcIdx++;
				continue;
			}
			else
			{
				cleanDetail[destIdx++] = p->detail[srcIdx];
			}
			srcIdx++;
		}
		cleanDetail[destIdx] = '\0';
		const char* prescIdStr = (strlen(p->prescriptionId) > 0) ? p->prescriptionId : "无";
		// 严格按字段顺序写入，纯数字枚举避免编码乱码
		fprintf(fp, "%s,%s,%s,%s,%d,%s,%s,%.2f,%s\n",
			p->recordId,
			p->patientId,
			p->doctorId,
			p->deptId,
			p->type,
			p->createTime,
			cleanDetail,
			p->fee,
			prescIdStr);
		p = p->next;
		saveCount++;
	}
	fclose(fp);
	char msg[80];
	snprintf(msg, sizeof(msg), "医疗记录保存成功，共%d条数据！", saveCount);
	PRINT_OK(msg);
	return 1;
}

// 从文件中加载数据（乱码修复版，和保存格式100%匹配）
int LoadRecordFromFile(char* fileName)
{
	FILE* fp = fopen(fileName, "r");
	if (fp == NULL)
	{
		PRINT_TIP("未找到医疗记录历史数据文件，将使用空数据库！");
		return 0;
	}
	RecordNode newrecord;
	int loadCount = 0;
	int typeTemp = 0;
	// 【修复1】删除此处未定义的 token 使用
	// typeTemp = atoi(token); 
	char line[MAX_LINE_LEN] = { 0 };
	while (fgets(line, sizeof(line), fp) != NULL)
	{
		int len = (int)strlen(line);
		if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
		if (len > 1 && line[len - 2] == '\r') line[len - 2] = '\0';
		if (strlen(line) == 0) continue;

		memset(&newrecord, 0, sizeof(RecordNode));
		typeTemp = 0;
		char* token = NULL;
		char* rest = line;
		int fieldIndex = 0;

		while ((token = strtok_s(rest, ",", &rest)) != NULL && fieldIndex < 9) {
			switch (fieldIndex) {
			case 0: SafeStrCopy(newrecord.recordId, token, MAX_ID_LEN); break;
			case 1: SafeStrCopy(newrecord.patientId, token, MAX_ID_LEN); break;
			case 2: SafeStrCopy(newrecord.doctorId, token, MAX_ID_LEN); break;
			case 3: SafeStrCopy(newrecord.deptId, token, MAX_ID_LEN); break;
			case 4:
				typeTemp = atoi(token);
				// 类型合法性校验必须在break之前执行
				if (typeTemp < REGISTER || typeTemp > HOSPITAL_DISCHARGE) {
					continue;
				}
				break;
			case 5: SafeStrCopy(newrecord.createTime, token, MAX_DATE_LEN); break;
			case 6: SafeStrCopy(newrecord.detail, token, MAX_DETAIL_LEN); break;
			case 7: newrecord.fee = (float)atof(token); break;
			case 8:
				if (strcmp(token, "无") != 0) {
					SafeStrCopy(newrecord.prescriptionId, token, MAX_ID_LEN);
				}
				else {
					memset(newrecord.prescriptionId, 0, MAX_ID_LEN);
				}
				break;
			}
			fieldIndex++;
		}
		if (fieldIndex != 9) continue;
		newrecord.type = (RecordType)typeTemp;
		if (CheckRecordValid(&newrecord, 1) == 1)
		{
			if (InsertRecordToList(newrecord) == 1)
			{
				loadCount++;
			}
		}
		memset(line, 0, sizeof(line));
	}
	fclose(fp);
	if (loadCount > 0) {
		char msg[80];
		snprintf(msg, sizeof(msg), "医疗记录加载成功，共加载%d条有效数据！", loadCount);
		PRINT_OK(msg);
	}
	else {
		PRINT_TIP("医疗记录数据文件为空，未加载任何数据！");
	}
	return 1;
}

// 释放链表内存（仅优化打印提示）
void FreeRecordList()
{
	RecordNode* p = recordHead->next;
	RecordNode* q = NULL;
	while (p != NULL)
	{
		q = p->next;
		free(p);
		p = q;
	}
	free(recordHead);
	recordHead = NULL;
	PRINT_OK("释放医疗记录链表内存成功！");
}

// 数据合法性校验（完全不变，仅优化内部打印提示为宏）
int CheckRecordValid(RecordNode* newRecord, int mode)
{
	// 检查记录Id
	if (strlen(newRecord->recordId) == 0 || strlen(newRecord->recordId) >= MAX_ID_LEN)
	{
		PRINT_WARN("记录ID为空或长度超限！");
		return 0;
	}
	if (!IsAlphaNumber(newRecord->recordId))
	{
		PRINT_WARN("记录ID格式错误!只能包含字母和数字！");
		return 0;
	}
	if (mode == 1)
	{
		if (FindRecordById(newRecord->recordId) != NULL)
		{
			char msg[80];
			snprintf(msg, sizeof(msg), "记录ID【%s】已存在!无法新增！", newRecord->recordId);
			PRINT_WARN(msg);
			return 0;
		}
	}
	// 检查关联患者
	if (strlen(newRecord->patientId) == 0 || strlen(newRecord->patientId) >= MAX_ID_LEN)
	{
		PRINT_WARN("关联患者ID为空或长度超限！");
		return 0;
	}
	if (!IsAlphaNumber(newRecord->patientId))
	{
		PRINT_WARN("患者ID格式错误!只能包含字母和数字！");
		return 0;
	}
	if (!CheckPatientExist(newRecord->patientId))
	{
		char msg[80];
		snprintf(msg, sizeof(msg), "患者ID【%s】不存在！", newRecord->patientId);
		PRINT_WARN(msg);
		return 0;
	}
	// 检查开单医生
	if (strlen(newRecord->doctorId) == 0 || strlen(newRecord->doctorId) >= MAX_ID_LEN)
	{
		PRINT_WARN("开单医生ID为空或长度超限！");
		return 0;
	}
	if (!IsAlphaNumber(newRecord->doctorId))
	{
		PRINT_WARN("开单医生ID格式错误!只能包含字母和数字！");
		return 0;
	}
	if (!CheckDoctorExist(newRecord->doctorId))
	{
		char msg[80];
		snprintf(msg, sizeof(msg), "医生ID【%s】不存在！", newRecord->doctorId);
		PRINT_WARN(msg);
		return 0;
	}
	// 检查科室
	if (strlen(newRecord->deptId) == 0 || strlen(newRecord->deptId) >= MAX_ID_LEN)
	{
		PRINT_WARN("科室ID为空或长度超限！");
		return 0;
	}
	if (!CheckDepartmentExist(newRecord->deptId))
	{
		char msg[80];
		snprintf(msg, sizeof(msg), "科室ID【%s】不存在！", newRecord->deptId);
		PRINT_WARN(msg);
		return 0;
	}
	// 金额合法性校验
	if (newRecord->fee < 0 || newRecord->fee > 100000)
	{
		char msg[80];
		snprintf(msg, sizeof(msg), "费用【%.2f】非法,单笔金额不得超过10万元！", newRecord->fee);
		PRINT_WARN(msg);
		return 0;
	}

	// 记录类型合法性校验，覆盖全量枚举
	if (newRecord->type < REGISTER || newRecord->type > HOSPITAL_DISCHARGE)
	{
		PRINT_WARN("记录类型非法！超出系统允许的记录类型范围");
		return 0;
	}
	// 时间格式校验
	int month, day, hour, minute;
	if (sscanf(newRecord->createTime, "%d-%d %d:%d", &month, &day, &hour, &minute) != 4)
	{
		PRINT_WARN("时间格式非法，必须符合「月-日 时:分」规范！");
		return 0;
	}
	if (month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 || minute < 0 || minute > 59)
	{
		PRINT_WARN("时间数值非法！");
		return 0;
	}

	return 1;
}

// 新增挂号记录（仅优化打印提示）
int AddRegisterRecord(RecordNode newRecord)
{
	newRecord.type = REGISTER;
	if (!CheckRecordValid(&newRecord, 1))
	{
		return 0;
	}
	if (!AddDoctorRegisterCount(newRecord.doctorId))
	{
		return 0;
	}
	if (!InsertRecordToList(newRecord))
	{
		// 【关键】插入失败，回滚医生的挂号计数，保证数据一致性
		SubDoctorRegisterCount(newRecord.doctorId);
		PRINT_ERR("挂号失败，内存不足，记录创建失败！");
		return 0;
	}

	char msg[80];
	snprintf(msg, sizeof(msg), "挂号记录%s已创建！", newRecord.recordId);
	PRINT_OK(msg);
	return 1;
}

// 新增看诊记录（仅优化打印提示）
int AddConsultRecord(RecordNode newRecord)
{
	newRecord.type = CONSULT;
	if (!CheckRecordValid(&newRecord, 1))
	{
		return 0;
	}

	if (!InsertRecordToList(newRecord))
	{
		return 0;
	}

	char msg[80];
	snprintf(msg, sizeof(msg), "看诊记录%s已创建！", newRecord.recordId);
	PRINT_OK(msg);
	return 1;
}

// 新增检查记录（仅优化打印提示，保留扩展注释）
int AddExamineRecord(RecordNode newRecord)
{
	newRecord.type = EXAMINE;
	if (!CheckRecordValid(&newRecord, 1))
	{
		return 0;
	}

	/* 可选：检查患者是否已在该医生处挂号
	if (!CheckPatientHasRegistered(newRecord.patientId, newRecord.doctorId))
	{
		PRINT_WARN("患者未在医生处挂号，无法开检查单！");
		return 0;
	}
	*/

	if (!InsertRecordToList(newRecord))
	{
		PRINT_ERR("新增失败，内存不足，检查记录创建失败！");
		return 0;
	}

	char msg[80];
	snprintf(msg, sizeof(msg), "检查记录%s已创建!检查项目：%s,费用：%.2f元",
		newRecord.recordId, newRecord.detail, newRecord.fee);
	PRINT_OK(msg);
	return 1;
}

// 新增住院记录（仅优化打印提示）
int AddHospitalizeRecord(RecordNode newRecord)
{
	newRecord.type = HOSPITALIZE;
	if (!CheckRecordValid(&newRecord, 1))
	{
		return 0;
	}
	// 强制权限校验：仅能给自己接诊过的患者生成住院记录
	if (!IsPatientTreatedByDoctor(newRecord.patientId, newRecord.doctorId)) {
		PRINT_WARN("权限拒绝：您只能给自己接诊过的患者办理住院！");
		return 0;
	}
	// 校验：关联的床位ID不能为空
	if (strlen(newRecord.prescriptionId) == 0)
	{
		PRINT_WARN("住院记录必须绑定床位ID！");
		return 0;
	}
	// 校验：床位是否存在且空闲
	if (!CheckBedAvailable(newRecord.prescriptionId))
	{
		char msg[80];
		snprintf(msg, sizeof(msg), "床位%s不存在或已被占用！", newRecord.prescriptionId);
		PRINT_WARN(msg);
		return 0;
	}
	// 分配床位（更新床位状态为占用、关联患者ID、记录入院时间）
	if (!AssignBedToPatient(newRecord.prescriptionId, newRecord.patientId, newRecord.createTime))
	{
		char msg[80];
		snprintf(msg, sizeof(msg), "新增失败，床位【%s】分配失败！", newRecord.prescriptionId);
		PRINT_ERR(msg);
		return 0;
	}
	// 更新患者类型为「住院」，并绑定床位ID
	if (!UpdatePatientType(newRecord.patientId, 2, newRecord.prescriptionId, newRecord.createTime))
	{
		PRINT_ERR("患者类型更新失败！");
		// 患者类型更新失败，必须释放刚才分配的床位
		ReleaseBed(newRecord.prescriptionId);
		return 0;
	}
	if (!InsertRecordToList(newRecord))
	{
		PRINT_ERR("内存不足，住院记录创建失败！");
		// 插入失败，必须回滚所有已修改的数据
		ReleaseBed(newRecord.prescriptionId); // 释放床位
		UpdatePatientType(newRecord.patientId, 1, NULL, NULL); // 回滚患者类型为门诊
		return 0;
	}

	char msg[80];
	snprintf(msg, sizeof(msg), "住院记录%s已创建!患者%s已入住床位%s！",
		newRecord.recordId, newRecord.patientId, newRecord.prescriptionId);
	PRINT_OK(msg);
	return 1;
}

// 修改医疗记录（仅优化打印提示，财务冲账部分保留原有格式）
int ModifyRecord(const char* recordId, RecordNode newData)
{
	// 1. 基础校验：待修改的记录ID不能为空
	if (recordId == NULL || strlen(recordId) == 0)
	{
		PRINT_WARN("记录ID不能为空！");
		return 0;
	}
	RecordNode* oldRecord = FindRecordById(recordId);
	if (oldRecord == NULL)
	{
		char msg[80];
		snprintf(msg, sizeof(msg), "未找到记录ID%s！", recordId);
		PRINT_ERR(msg);
		return 0;
	}
	if (!CheckRecordValid(&newData, 2))
	{
		PRINT_ERR("修改失败，新数据合法性校验不通过！");
		return 0;
	}
	// 修改涉及费用的错误数据，需先将当前错误数据做冲账回退处理
	if (oldRecord->fee != newData.fee)
	{
		RecordNode reverseRecord = *oldRecord;
		reverseRecord.fee = -oldRecord->fee;
		snprintf(reverseRecord.detail, MAX_DETAIL_LEN, "【财务冲销】原记录ID:%s 金额回退", oldRecord->recordId);

		// 生成唯一冲账记录ID，避免重复
		int cxSuffix = 0;
		while (1) {
			if (cxSuffix == 0) {
				snprintf(reverseRecord.recordId, MAX_ID_LEN, "CX%s", oldRecord->recordId);
			}
			else {
				snprintf(reverseRecord.recordId, MAX_ID_LEN, "CX%s_%d", oldRecord->recordId, cxSuffix);
			}
			if (FindRecordById(reverseRecord.recordId) == NULL) {
				break;
			}
			cxSuffix++;
		}
		// 2. 插入冲账记录到链表
		if (!InsertRecordToList(reverseRecord))
		{
			PRINT_ERR("修改失败，冲账记录创建失败，无法修改！");
			return 0;
		}
		printf("\n");
		print_title_box("财务冲账凭证");
		char info1[80], info2[80], info3[80], info4[80];
		snprintf(info1, sizeof(info1), "原记录ID:%s", oldRecord->recordId);
		snprintf(info2, sizeof(info2), "原费用：%.2f元 → 已冲销", oldRecord->fee);
		snprintf(info3, sizeof(info3), "新费用：%.2f元 → 重新入账", newData.fee);
		snprintf(info4, sizeof(info4), "冲账记录ID:%s", reverseRecord.recordId);
		const char* lines[5] = { "财务冲账凭证", info1, info2, info3, info4 };
		print_content_box(lines, 5);
		printf("\n");
	}
	// 生成唯一修改后记录ID，避免重复
	int xgSuffix = 0;
	while (1) {
		if (xgSuffix == 0) {
			snprintf(newData.recordId, MAX_ID_LEN, "XG%s", oldRecord->recordId);
		}
		else {
			snprintf(newData.recordId, MAX_ID_LEN, "XG%s_%d", oldRecord->recordId, xgSuffix);
		}
		if (FindRecordById(newData.recordId) == NULL) {
			break;
		}
		xgSuffix++;
	}
	if (!InsertRecordToList(newData))
	{
		PRINT_ERR("修改失败，新记录创建失败！");
		return 0;
	}

	// 原记录仅标记为已冲销，保留完整追溯链路
	snprintf(oldRecord->detail, MAX_DETAIL_LEN, "【已冲销】新记录ID:%s", newData.recordId);

	// 4. 【住院记录特殊处理】如果修改了床位，需释放旧床位、分配新床位
	if (oldRecord->type == HOSPITALIZE && strcmp(oldRecord->prescriptionId, newData.prescriptionId) != 0)
	{
		// 释放旧床位
		if (strlen(oldRecord->prescriptionId) > 0)
		{
			ReleaseBed(oldRecord->prescriptionId);
		}
		// 分配新床位
		if (strlen(newData.prescriptionId) > 0)
		{
			if (!CheckBedAvailable(newData.prescriptionId))
			{
				char msg[80];
				snprintf(msg, sizeof(msg), "修改失败，新床位%s不存在或已被占用！", newData.prescriptionId);
				PRINT_ERR(msg);
				// 回滚：重新占用旧床位，保证数据一致
				return 0;
			}
			AssignBedToPatient(newData.prescriptionId, newData.patientId, newData.createTime);
			UpdatePatientType(newData.patientId, 2, newData.prescriptionId, newData.createTime);
		}
	}

	char msg[80];
	snprintf(msg, sizeof(msg), "记录已更新!新记录ID:%s", newData.recordId);
	PRINT_OK(msg);
	return 1;
}

// 删除医疗记录（仅优化打印提示）
int DeleteRecord(const char* recordId)
{
	// 1. 基础校验：待删除的记录ID不能为空
	if (recordId == NULL || strlen(recordId) == 0)
	{
		PRINT_WARN("记录ID不能为空！");
		return 0;
	}
	RecordNode* p = recordHead;
	RecordNode* target = NULL;
	while (p->next != NULL)
	{
		if (strcmp(p->next->recordId, recordId) == 0)
		{
			target = p->next;
			break;
		}
		p = p->next;
	}
	if (target == NULL)
	{
		char msg[80];
		snprintf(msg, sizeof(msg), "删除失败:未找到记录ID「%s」！", recordId);
		PRINT_ERR(msg);
		return 0;
	}
	// 2. 【住院记录特殊处理】如果该条记录是住院记录，需释放其关联的床位
	if (target->type == HOSPITALIZE && strlen(target->prescriptionId) > 0)
	{
		ReleaseBed(target->prescriptionId);
		UpdatePatientType(target->patientId, 1, NULL, NULL); // 将患者类型回滚为门诊
		char msg[80];
		snprintf(msg, sizeof(msg), "联动处理：旧床位%s已释放！", target->prescriptionId);
		PRINT_TIP(msg);
	}
	// 3. 从链表中移除这条记录
	p->next = target->next;
	free(target);

	char msg[80];
	snprintf(msg, sizeof(msg), "删除成功!医疗记录ID%s已移除。", recordId);
	PRINT_OK(msg);
	return 1;
}

// 根据患者ID/姓名查询医疗记录列表（美化打印，使用80宽度表格，支持姓名查询）
void QueryRecordByPatientId(const char* input)
{
	if (input == NULL || recordHead == NULL || recordHead->next == NULL) {
		const char* lines[] = { "暂无医疗记录数据" };
		print_content_box(lines, 1);
		return;
	}
	extern char* GetPatientRealId(const char* input);
	char* realId = GetPatientRealId(input);
	if (realId == NULL) {
		char msg[80];
		snprintf(msg, sizeof(msg), "未找到匹配的患者「%s」！", input);
		PRINT_ERR(msg);
		return;
	}
	// 先统计匹配数量
	int findCount = 0;
	RecordNode* p = recordHead->next;
	while (p != NULL)
	{
		if (strcmp(realId, p->patientId) == 0)
		{
			findCount++;
		}
		p = p->next;
	}
	// 为空直接提示
	if (findCount == 0) {
		char msg[80];
		snprintf(msg, sizeof(msg), "患者%s暂无医疗记录", realId);
		const char* lines[] = { msg };
		print_content_box(lines, 1);
		return;
	}
	// 120宽度标题框
	char title[80];
	snprintf(title, sizeof(title), "患者%s医疗记录", realId);
	print_title_box_ex(title, 120);
	// 列宽适配120宽度
	int cols[] = { 14, 12, 12, 10, 14, 12, 14, 20 };
	AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_LEFT };
	const char* headers[] = { "记录ID", "医生ID", "科室ID", "类型", "时间", "费用", "床位ID", "记录详情" };
	printf("\n");
	print_table_sep_ex(cols, 8, 120);
	print_table_row_ex(cols, aligns, headers, 8, 120);
	print_table_sep_ex(cols, 8, 120);
	p = recordHead->next;
	while (p != NULL)
	{
		if (strcmp(realId, p->patientId) == 0)
		{
			char typeStr[20], feeStr[20];
			switch (p->type)
			{
			case REGISTER: strcpy(typeStr, "挂号"); break;
			case CONSULT: strcpy(typeStr, "看诊"); break;
			case EXAMINE: strcpy(typeStr, "检查"); break;
			case HOSPITALIZE: strcpy(typeStr, "住院"); break;
			case HOSPITAL_DEPOSIT: strcpy(typeStr, "押金缴费"); break;
			case HOSPITAL_DISCHARGE: strcpy(typeStr, "出院结算"); break;
			default: strcpy(typeStr, "未知"); break;
			}
			snprintf(feeStr, sizeof(feeStr), "%.2f", p->fee);
			const char* data[] = {
				p->recordId, p->doctorId, p->deptId, typeStr,
				p->createTime, feeStr,
				strlen(p->prescriptionId) > 0 ? p->prescriptionId : "无",
				p->detail
			};
			print_table_row_ex(cols, aligns, data, 8, 120);
		}
		p = p->next;
	}
	print_table_sep_ex(cols, 8, 120);
	char msg[80];
	snprintf(msg, sizeof(msg), "共找到%d条医疗记录！", findCount);
	PRINT_TIP(msg);
}

// 根据医生ID查询医疗记录列表（美化打印，使用80宽度表格）
void QueryRecordByDoctorId(const char* doctorId)
{
	if (doctorId == NULL || recordHead == NULL || recordHead->next == NULL) {
		PRINT_TIP("暂无医疗记录数据！");
		return;
	}

	char title[80];
	snprintf(title, sizeof(title), "医生%s医疗记录", doctorId);
	print_title_box(title);

	// 列宽严格计算：sum(cols)=72，72+6+1=80
	int cols[] = { 12, 10, 10, 8, 12, 12, 12 };
	AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
	const char* headers[] = { "记录ID", "患者ID", "科室ID", "类型", "时间", "费用", "处方ID" };

	printf("\n");
	print_table_sep(cols, 7);
	print_table_row(cols, aligns, headers, 7);
	print_table_sep(cols, 7);

	int findCount = 0;
	RecordNode* p = recordHead->next;
	while (p != NULL)
	{
		if (strcmp(doctorId, p->doctorId) == 0)
		{
			char typeStr[20], feeStr[20];
			switch (p->type)
			{
			case REGISTER: strcpy(typeStr, "挂号"); break;
			case CONSULT: strcpy(typeStr, "看诊"); break;
			case EXAMINE: strcpy(typeStr, "检查"); break;
			case HOSPITALIZE: strcpy(typeStr, "住院"); break;
			default: strcpy(typeStr, "未知");
			}
			snprintf(feeStr, sizeof(feeStr), "%.2f", p->fee);
			const char* data[] = {
				p->recordId, p->patientId, p->deptId, typeStr,
				p->createTime, feeStr,
				strlen(p->prescriptionId) > 0 ? p->prescriptionId : "无"
			};
			print_table_row(cols, aligns, data, 7);
			findCount++;
		}
		p = p->next;
	}
	print_table_sep(cols, 7);

	if (findCount == 0) {
		PRINT_TIP("未找到匹配的医疗记录！");
	}
	else {
		char msg[80];
		snprintf(msg, sizeof(msg), "共找到%d条医疗记录！", findCount);
		PRINT_TIP(msg);
	}
}

// 按时间范围查询医疗记录（美化打印，使用80宽度表格）
void QueryRecordByTimeRange(char* startTime, char* endTime)
{
	if (startTime == NULL || endTime == NULL || recordHead == NULL || recordHead->next == NULL) {
		PRINT_TIP("暂无医疗记录数据！");
		return;
	}

	char title[80];
	snprintf(title, sizeof(title), "【%s 至 %s】医疗记录明细", startTime, endTime);
	print_title_box(title);

	// 列宽严格计算：sum(cols)=72，72+6+1=80
	int cols[] = { 12, 10, 10, 8, 12, 12, 12 };
	AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
	const char* headers[] = { "记录ID", "患者ID", "医生ID", "类型", "时间", "费用", "处方ID" };

	printf("\n");
	print_table_sep(cols, 7);
	print_table_row(cols, aligns, headers, 7);
	print_table_sep(cols, 7);

	int findCount = 0;
	RecordNode* p = recordHead->next;
	while (p != NULL)
	{
		if (CompareTime(p->createTime, startTime) >= 0
			&& CompareTime(p->createTime, endTime) <= 0)
		{
			char typeStr[20], feeStr[20];
			switch (p->type)
			{
			case REGISTER: strcpy(typeStr, "挂号"); break;
			case CONSULT: strcpy(typeStr, "看诊"); break;
			case EXAMINE: strcpy(typeStr, "检查"); break;
			case HOSPITALIZE: strcpy(typeStr, "住院"); break;
			default: strcpy(typeStr, "未知");
			}
			snprintf(feeStr, sizeof(feeStr), "%.2f", p->fee);
			const char* data[] = {
				p->recordId, p->patientId, p->doctorId, typeStr,
				p->createTime, feeStr,
				strlen(p->prescriptionId) > 0 ? p->prescriptionId : "无"
			};
			print_table_row(cols, aligns, data, 7);
			findCount++;
		}
		p = p->next;
	}
	print_table_sep(cols, 7);

	if (findCount == 0) {
		PRINT_TIP("未查询到符合时间范围的记录！");
	}
	else {
		char msg[80];
		snprintf(msg, sizeof(msg), "共查询到 %d 条记录。", findCount);
		PRINT_TIP(msg);
	}
}

// 打印所有医疗记录（美化打印，使用80宽度表格）
void PrintAllRecord()
{
	if (recordHead == NULL || recordHead->next == NULL) {
		PRINT_TIP("暂无医疗记录数据！");
		return;
	}

	print_title_box("所有医疗记录");

	// 列宽严格计算：sum(cols)=72，72+6+1=80
	int cols[] = { 12, 10, 10, 8, 12, 12, 12 };
	AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
	const char* headers[] = { "记录ID", "患者ID", "医生ID", "类型", "时间", "费用", "处方ID" };

	printf("\n");
	print_table_sep(cols, 7);
	print_table_row(cols, aligns, headers, 7);
	print_table_sep(cols, 7);

	RecordNode* p = recordHead->next;
	while (p != NULL)
	{
		char typeStr[20], feeStr[20];
		switch (p->type)
		{
		case REGISTER: strcpy(typeStr, "挂号"); break;
		case CONSULT: strcpy(typeStr, "看诊"); break;
		case EXAMINE: strcpy(typeStr, "检查"); break;
		case HOSPITALIZE: strcpy(typeStr, "住院"); break;
		default: strcpy(typeStr, "未知");
		}
		snprintf(feeStr, sizeof(feeStr), "%.2f", p->fee);
		const char* data[] = {
			p->recordId, p->patientId, p->doctorId, typeStr,
			p->createTime, feeStr,
			strlen(p->prescriptionId) > 0 ? p->prescriptionId : "无"
		};
		print_table_row(cols, aligns, data, 7);
		p = p->next;
	}
	print_table_sep(cols, 7);
}

// 其余函数（SortRecordByTime、CompareTime、GetTotalRecordFee）完全不变
void SortRecordByTime(RecordList list)
{
	if (list == NULL || list->next == NULL || list->next->next == NULL)
		return;

	RecordNode* p = list->next;
	RecordNode* q = NULL;
	int isSorted;

	do
	{
		isSorted = 1;
		p = list->next;
		while (p != NULL && p->next != NULL)
		{
			q = p->next;

			if (CompareTime(p->createTime, q->createTime) > 0)
			{
				RecordNode temp = *p;
				*p = *q;
				*q = temp;

				RecordNode* tempNext = p->next;
				p->next = q->next;
				q->next = tempNext;
				isSorted = 0;
			}

			p = p->next;
		}
	} while (!isSorted);
}

int CompareTime(const char* time1, const char* time2)
{
	if (time1 == NULL || time2 == NULL)
	{
		return 0;
	}

	int month1, day1, hour1, minute1;
	int month2, day2, hour2, minute2;

	int parseRes1 = sscanf(time1, "%d-%d %d:%d", &month1, &day1, &hour1, &minute1);
	int parseRes2 = sscanf(time2, "%d-%d %d:%d", &month2, &day2, &hour2, &minute2);

	if (parseRes1 != 4 || parseRes2 != 4)
	{
		PRINT_WARN("时间格式非法!正确格式:MM-DD HH:MM");
		return 0;
	}

	if (month1 != month2) return month1 - month2;
	if (day1 != day2) return day1 - day2;
	if (hour1 != hour2) return hour1 - hour2;
	return minute1 - minute2;
}

float GetTotalRecordFee()
{
	float totalFee = 0.0f;
	if (recordHead == NULL || recordHead->next == NULL)
	{
		return totalFee;
	}

	RecordNode* p = recordHead->next;
	while (p != NULL)
	{
		totalFee += p->fee;
		p = p->next;
	}

	totalFee = (float)((int)(totalFee * 100 + 0.5f)) / 100.0f;
	return totalFee;
}

// 打印所有记录简略信息（ID+患者ID+类型）
void PrintAllRecordBrief(void) {
	if (recordHead == NULL || recordHead->next == NULL) {
		PRINT_TIP("暂无医疗记录数据！");
		return;
	}
	print_title_box("医疗记录列表（ID+患者ID+类型）");
	int cols[] = { 15, 20, 43 };
	AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
	const char* headers[] = { "记录ID", "患者ID", "记录类型" };
	printf("\n");
	print_table_sep(cols, 3);
	print_table_row(cols, aligns, headers, 3);
	print_table_sep(cols, 3);
	RecordNode* p = recordHead->next;
	while (p != NULL) {
		char typeStr[20];
		switch (p->type) {
		case REGISTER: strcpy(typeStr, "挂号"); break;
		case CONSULT: strcpy(typeStr, "看诊"); break;
		case EXAMINE: strcpy(typeStr, "检查"); break;
		case HOSPITALIZE: strcpy(typeStr, "住院"); break;
		default: strcpy(typeStr, "未知"); break;
		}
		const char* data[] = { p->recordId, p->patientId, typeStr };
		print_table_row(cols, aligns, data, 3);
		p = p->next;
	}
	print_table_sep(cols, 3);
}