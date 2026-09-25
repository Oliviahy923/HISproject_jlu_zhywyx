// 头文件按依赖顺序引入
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>   // 修复：新增time.h，解决time()函数未定义报错
#include <math.h>   // 修复：新增math.h，解决fabs()函数未定义，连带解决格式类型不匹配
// 【必须优先包含common.h，统一全局打印规范】
#include "common.h"
// 再包含自定义业务头文件
#include "patient.h"
#include "doctor.h"
#include "record.h"
#include "bed.h"
#include "safe_utils.h"
#include "log.h"
#include "system.h"
// ==================== 内部静态函数前置声明 ====================
static int FindRecordByPatientAndDoctor(const char* patientId, const char* doctorId);

// ==================== 内部私有静态变量 ====================
static PatientList pathead = NULL;

// ==================== 内部私有辅助函数 ====================
//判断患者是否被指定医生接诊
static int FindRecordByPatientAndDoctor(const char* patientId, const char* doctorId) {
	if (!CheckNullPtr(2, patientId, doctorId) || recordHead == NULL) {
		return 0;
	}
	RecordList p = recordHead->next;
	while (p != NULL) {
		if (strcmp(p->patientId, patientId) == 0 && strcmp(p->doctorId, doctorId) == 0) {
			return 1;
		}
		p = p->next;
	}
	return 0;
}

// 【对外接口】医生接诊校验，给prescription.c调用
int IsPatientTreatedByDoctor(const char* patientId, const char* doctorId) {
	return FindRecordByPatientAndDoctor(patientId, doctorId);
}
// ==================== 对外接口函数实现（和头文件100%匹配）====================
// 初始化患者链表
void InitPatientList(void) {
	if (pathead != NULL) {
		FreePatientList();
	}
	pathead = (PatientList)malloc(sizeof(PatientNode));
	if (pathead == NULL) {
		PRINT_ERR("患者链表初始化失败，内存不足！");
		WriteLog(LOG_LEVEL_ERROR, "系统", "初始化患者链表", "失败：内存分配失败");
		return;
	}
	memset(pathead, 0, sizeof(PatientNode));
	pathead->next = NULL;
	PRINT_OK("患者链表初始化成功！");
	WriteLog(LOG_LEVEL_INFO, "系统", "初始化患者链表", "成功");
}

// 按ID查找患者
PatientList FindPatientById(const char* patientId) {
	if (patientId == NULL || pathead == NULL || pathead->next == NULL) {
		return NULL;
	}
	PatientList p = pathead->next;
	while (p != NULL) {
		if (strcmp(p->patientId, patientId) == 0) {
			return p;
		}
		p = p->next;
	}
	return NULL;
}

// 患者数据保存到文件
int SavePatientToFile(const char* filename) {
	if (pathead == NULL || pathead->next == NULL) {
		PRINT_TIP("暂无患者数据，跳过保存！");
		return 0;
	}
	if (filename == NULL) {
		PRINT_ERR("患者数据文件路径不能为空！");
		WriteLog(LOG_LEVEL_ERROR, "系统", "保存患者数据", "失败：文件名为空");
		return 0;
	}
	FILE* fp = fopen(filename, "w");
	if (fp == NULL) {
		PRINT_ERR("患者数据文件打开失败！");
		WriteLog(LOG_LEVEL_ERROR, "系统", "保存患者数据", "失败：文件打开失败");
		return 0;
	}
	PatientNode* p = pathead->next;
	int saveCount = 0;
	while (p != NULL) {
		fprintf(fp, "%s,%s,%s,%d,%s,%d,%s\n",
			p->patientId, p->name, p->gender, p->age,
			p->phone, p->patientType, p->bedId);
		p = p->next;
		saveCount++;
	}
	fclose(fp);
	char msg[80];
	snprintf(msg, sizeof(msg), "患者数据保存成功，共%d条数据", saveCount);
	PRINT_OK(msg);
	WriteLog(LOG_LEVEL_INFO, "系统", "保存患者数据", msg);
	return 1;
}

// 从文件加载患者数据
int LoadPatientFromFile(const char* filename) {
	if (pathead == NULL) {
		PRINT_ERR("患者链表未初始化！");
		return 0;
	}
	FILE* fp = fopen(filename, "r");
	if (fp == NULL) {
		PRINT_TIP("未找到患者数据文件，新建空库");
		return 0;
	}
	PatientNode newPatient = { 0 };
	int successCount = 0, failCount = 0;
	char line[512];
	PRINT_TIP("开始加载患者数据...");
	while (fgets(line, sizeof(line), fp) != NULL) {
		int len = (int)strlen(line);
		if (len > 0 && line[len - 1] == '\n') {
			line[len - 1] = '\0';
			len--;
		}
		if (len > 0 && line[len - 1] == ',') line[len - 1] = '\0';
		memset(&newPatient, 0, sizeof(PatientNode));
		char* rest = line;
		char* token = strtok_s(rest, ",", &rest);
		int fieldCount = 0;
		while (token != NULL && fieldCount < 7) {
			switch (fieldCount) {
			case 0: SafeStrCopy(newPatient.patientId, token, MAX_ID_LEN); break;
			case 1: SafeStrCopy(newPatient.name, token, MAX_NAME_LEN); break;
			case 2: SafeStrCopy(newPatient.gender, token, 10); break;
			case 3: newPatient.age = atoi(token); break;
			case 4: SafeStrCopy(newPatient.phone, token, MAX_ID_LEN); break;
			case 5: newPatient.patientType = atoi(token); break;
			case 6: SafeStrCopy(newPatient.bedId, token, MAX_ID_LEN); break;
			}
			fieldCount++;
			token = strtok_s(NULL, ",", &rest);
		}
		if (fieldCount >= 6) {
			PatientNode* newNode = (PatientList)malloc(sizeof(PatientNode));
			if (newNode != NULL) {
				memcpy(newNode, &newPatient, sizeof(PatientNode));
				newNode->next = pathead->next;
				pathead->next = newNode;
				successCount++;
			}
			else failCount++;
		}
		else failCount++;
	}
	fclose(fp);
	if (successCount > 0) {
		char msg[80];
		snprintf(msg, sizeof(msg), "患者数据加载成功，共%d条有效数据", successCount);
		PRINT_OK(msg);
		WriteLog(LOG_LEVEL_INFO, "系统", "加载患者数据", msg);
	}
	else PRINT_TIP("患者数据文件为空，未加载任何数据！");
	return successCount;
}

// 释放患者链表内存
void FreePatientList(void) {
	if (pathead == NULL) return;
	PatientNode* next, * p = pathead;
	while (p != NULL) {
		next = p->next;
		free(p);
		p = next;
	}
	pathead = NULL;
	PRINT_OK("释放患者链表内存成功！");
	WriteLog(LOG_LEVEL_INFO, "系统", "释放患者链表", "成功");
}

// 患者数据合法性校验（新增性别校验）【修复C4090 const限定符不匹配报错】
int CheckPatientValid(const PatientNode* newPatient, int mode) {
	if (!CheckNullPtr(1, newPatient)) {
		printf("患者数据指针为空！\n");
		return 0;
	}
	if (strlen(newPatient->patientId) == 0 || strlen(newPatient->patientId) >= MAX_ID_LEN) {
		printf("患者ID为空或长度超限!\n");
		return 0;
	}
	// 修复：强转const char*为char*，解决工具函数形参无const导致的const不匹配
	if (!IsAlphaNumber((char*)newPatient->patientId)) {
		printf("患者ID格式错误!只能包含字母和数字！\n");
		return 0;
	}
	if (mode == 1 && FindPatientById(newPatient->patientId) != NULL) {
		printf("患者ID已存在,重复!\n");
		return 0;
	}
	if (strlen(newPatient->name) == 0 || strlen(newPatient->name) >= MAX_NAME_LEN) {
		printf("患者姓名为空或长度超限！\n");
		return 0;
	}
	// 修复：强转const char*为char*，解决const不匹配
	if (HasNumber((char*)newPatient->name)) {
		printf("患者姓名不能包含数字！\n");
		return 0;
	}
	if (strlen(newPatient->phone) == 0 || strlen(newPatient->phone) >= MAX_ID_LEN) {
		printf("患者联系方式为空或长度超限！\n");
		return 0;
	}
	// 修复：强转const char*为char*，解决const不匹配
	if (!IsNumber((char*)newPatient->phone)) {
		printf("联系方式格式错误！必须是纯数字！\n");
		return 0;
	}
	if (newPatient->patientType != 1 && newPatient->patientType != 2) {
		printf("患者类型输入错误，无法匹配\n");
		return 0;
	}
	// 性别强制校验
	if (strcmp(newPatient->gender, "男") != 0 && strcmp(newPatient->gender, "女") != 0) {
		printf("患者性别不合法！仅允许输入「男」或「女」\n");
		return 0;
	}
	return 1;
}

// 新增患者（基础接口）
int AddPatient(PatientNode newPatient) {
	if (!CheckPatientValid(&newPatient, 1)) {
		return 0;
	}
	// ========== 重名校验核心代码 ==========
	int sameNameCount = 0;
	PatientNode* p = pathead->next;
	while (p != NULL) {
		if (strcmp(p->name, newPatient.name) == 0) {
			sameNameCount++;
		}
		p = p->next;
	}
	if (sameNameCount > 0) {
		char msg[100];
		snprintf(msg, sizeof(msg), "警告：系统中已存在%d名姓名为【%s】的患者！", sameNameCount, newPatient.name);
		PRINT_WARN(msg);
		int confirm = 0;
		SafeIntInput("是否确认继续新增该同名患者？输入1=确认，输入0=取消", &confirm, 0, 1);
		if (confirm == 0) {
			PRINT_TIP("已取消新增患者操作！");
			return 0;
		}
	}
	// ========== 重名校验结束 ==========
	PatientNode* newNode = (PatientNode*)malloc(sizeof(PatientNode));
	if (newNode == NULL) {
		PRINT_ERR("内存分配失败，新增患者失败！");
		return 0;
	}
	*newNode = newPatient;
	newNode->next = pathead->next;
	pathead->next = newNode;
	PRINT_OK("患者新增成功！");
	return 1;
}

// 交互式新增患者
// 修复：删除重复的性别输入代码，修正语法块闭合错误
int AddPatientInteractive(void) {
	if (GetCurrentRole() == ROLE_DOCTOR) {
		PRINT_ERR("医生无权新增患者，请联系管理员！");
		WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "新增患者", "失败：权限不足");
		return 0;
	}
	PatientNode newPatient = { 0 };
	print_title_box("新增患者");
	while (1) {
		SafeStrInput("请输入患者唯一ID（如P006）", newPatient.patientId, MAX_ID_LEN);
		if (FindPatientById(newPatient.patientId) != NULL) {
			PRINT_WARN("患者ID已存在，请重新输入！");
			continue;
		}
		break;
	}
	SafeStrInput("请输入患者姓名", newPatient.name, MAX_NAME_LEN);
	// 修复：删除重复的性别输入行，仅保留带校验的循环
	while (1) {
		SafeStrInput("请输入患者性别（仅允许输入：男/女）", newPatient.gender, 10);
		if (strcmp(newPatient.gender, "男") == 0 || strcmp(newPatient.gender, "女") == 0) {
			break;
		}
		PRINT_WARN("性别输入不合法！仅允许输入「男」或「女」，请重新输入！");
	}
	SafeIntInput("请输入患者年龄", &newPatient.age, 0, 120);
	SafeStrInput("请输入患者联系方式（纯数字）", newPatient.phone, MAX_DATA_LEN);
	while (1) {
		SafeIntInput("请输入患者类型（1=门诊，2=住院）", &newPatient.patientType, 1, 2);
		if (newPatient.patientType == 1 || newPatient.patientType == 2) break;
		PRINT_WARN("类型只能是1或2，请重新输入！");
	}
	if (newPatient.patientType == 2) {
		SafeStrInput("请输入绑定的床位ID（如B001）", newPatient.bedId, MAX_ID_LEN);
	}
	else memset(newPatient.bedId, 0, sizeof(newPatient.bedId));
	return AddPatient(newPatient);
}

// 修改患者基础信息
int ModifyPatient(const char* patientId, PatientNode newData) {
	int mode = 2;
	PatientNode* p = FindPatientById(patientId);
	if (p == NULL) {
		char msg[80];
		snprintf(msg, sizeof(msg), "修改失败:未找到患者ID「%s」!", patientId);
		PRINT_ERR(msg);
		WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "修改患者", msg);
		return 0;
	}
	if (!CheckPatientValid(&newData, mode)) return 0;
	if (strcmp(p->bedId, newData.bedId) != 0 && !CheckBedAvailable(newData.bedId)) {
		char msg[80];
		snprintf(msg, sizeof(msg), "床位ID【%s】已被占用,无法分配！", newData.bedId);
		PRINT_ERR(msg);
		return 0;
	}
	SafeStrCopy(p->name, newData.name, MAX_NAME_LEN);
	SafeStrCopy(p->gender, newData.gender, 10);
	SafeStrCopy(p->phone, newData.phone, MAX_ID_LEN);
	p->age = newData.age;
	char msg[80];
	snprintf(msg, sizeof(msg), "修改成功!患者ID「%s」的基础信息已更新。", patientId);
	PRINT_OK(msg);
	WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "修改患者", msg);
	return 1;
}

// 更新患者类型与床位绑定
int UpdatePatientType(const char* patientId, int newType, const char* bedId, const char* inTime) {
	PatientNode* p = FindPatientById(patientId);
	if (p == NULL) {
		char msg[80];
		snprintf(msg, sizeof(msg), "修改失败:未找到患者ID「%s」!", patientId);
		PRINT_ERR(msg);
		WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "修改患者类型", msg);
		return 0;
	}
	if (newType == 2) {
		if (strlen(bedId) == 0) {
			PRINT_ERR("修改失败：住院患者必须填写床位！");
			return 0;
		}
		if (!CheckBedExist(bedId)) {
			PRINT_ERR("修改失败，床位不存在！");
			return 0;
		}
		if (!CheckBedAvailable(bedId)) {
			PRINT_ERR("修改失败：床位已被占用！");
			return 0;
		}
		if (strlen(p->bedId) > 0) ReleaseBed(p->bedId);
		AssignBedToPatient(bedId, patientId, inTime);
		SafeStrCopy(p->bedId, bedId, MAX_ID_LEN);
	}
	else {
		if (strlen(p->bedId) > 0) {
			ReleaseBed(p->bedId);
			memset(p->bedId, 0, sizeof(p->bedId));
		}
	}
	p->patientType = newType;
	PRINT_OK("患者信息修改成功！");
	WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "修改患者类型", "成功");
	return 1;
}

// 删除患者
int DeletePatient(const char* patientId) {
	if (pathead == NULL || pathead->next == NULL) {
		PRINT_ERR("暂无患者数据，删除失败！");
		return 0;
	}
	PatientNode* p = pathead;
	int found = 0;
	while (p->next != NULL) {
		if (strcmp(p->next->patientId, patientId) == 0) {
			found = 1;
			break;
		}
		p = p->next;
	}
	if (found == 0) {
		char msg[80];
		snprintf(msg, sizeof(msg), "删除失败:未找到患者ID「%s」!", patientId);
		PRINT_ERR(msg);
		WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "删除患者", msg);
		return 0;
	}
	PatientNode* dele = p->next;
	p->next = dele->next;
	free(dele);
	char msg[80];
	snprintf(msg, sizeof(msg), "删除成功!患者ID「%s」已移除。", patientId);
	PRINT_OK(msg);
	WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "删除患者", msg);
	return 1;
}

// 检查患者是否存在
int CheckPatientExist(const char* patientId) {
	return FindPatientById(patientId) != NULL ? 1 : 0;
}

// 按姓名检查患者是否存在
int CheckPatientExistByName(const char* patientName) {
	if (patientName == NULL || pathead == NULL || pathead->next == NULL) {
		return 0;
	}
	PatientList p = pathead->next;
	while (p != NULL) {
		if (strcmp(p->name, patientName) == 0) return 1;
		p = p->next;
	}
	return 0;
}

// 根据姓名/ID获取患者真实ID
char* GetPatientRealId(const char* input) {
	static char realId[MAX_ID_LEN] = { 0 };
	memset(realId, 0, sizeof(realId));
	if (input == NULL || pathead == NULL || pathead->next == NULL) {
		return NULL;
	}
	PatientList p = pathead->next;
	while (p != NULL) {
		if (strcmp(p->patientId, input) == 0 || strcmp(p->name, input) == 0) {
			SafeStrCopy(realId, p->patientId, MAX_ID_LEN);
			return realId;
		}
		p = p->next;
	}
	return NULL;
}

// 按ID查询患者详情
void QueryPatientById(const char* patientId) {
	PatientNode* p = FindPatientById(patientId);
	if (p == NULL) {
		char msg[80];
		snprintf(msg, sizeof(msg), "查询失败:未找到患者ID「%s」!", patientId);
		PRINT_ERR(msg);
		return;
	}
	char title[80];
	snprintf(title, sizeof(title), "患者详情（ID:%s）", patientId);
	print_title_box(title);
	char info1[80], info2[80], info3[80], info4[80], info5[80], info6[80], info7[80];
	snprintf(info1, sizeof(info1), "患者ID：%s", p->patientId);
	snprintf(info2, sizeof(info2), "姓名：%s", p->name);
	snprintf(info3, sizeof(info3), "性别：%s", p->gender);
	snprintf(info4, sizeof(info4), "年龄：%d", p->age);
	snprintf(info5, sizeof(info5), "联系方式：%s", p->phone);
	snprintf(info6, sizeof(info6), "就医类型：%s", p->patientType == 1 ? "门诊" : "住院");
	snprintf(info7, sizeof(info7), "床位：%s", strlen(p->bedId) > 0 ? p->bedId : "无");
	const char* lines[7] = { info1, info2, info3, info4, info5, info6, info7 };
	print_content_box(lines, 7);
}

void QueryPatientByName(const char* name) {
	if (pathead == NULL || pathead->next == NULL) {
		const char* lines[] = { "暂无患者数据" };
		print_content_box(lines, 1);
		return;
	}
	if (name == NULL || *name == '\0') {
		PRINT_WARN("查询关键词不能为空！");
		return;
	}
	// 先统计匹配数量
	int cnt = 0;
	PatientNode* p = pathead->next;
	while (p != NULL) {
		if (strstr(p->name, name) != NULL) {
			cnt++;
		}
		p = p->next;
	}
	// 为空直接提示
	if (cnt == 0) {
		char msg[80];
		snprintf(msg, sizeof(msg), "未找到姓名含【%s】的患者", name);
		const char* lines[] = { msg };
		print_content_box(lines, 1);
		return;
	}
	// 不为空再打印标题和表格
	char title[80];
	snprintf(title, sizeof(title), "姓名含「%s」的患者列表", name);
	print_title_box(title);
	int cols[] = { 12, 12, 8, 6, 12, 14 };
	AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
	const char* headers[] = { "患者ID", "姓名", "性别", "年龄", "床位ID", "联系方式" };
	printf("\n");
	print_table_sep(cols, 6);
	print_table_row(cols, aligns, headers, 6);
	print_table_sep(cols, 6);
	p = pathead->next;
	while (p != NULL) {
		if (strstr(p->name, name) != NULL) {
			char age[10];
			snprintf(age, sizeof(age), "%d", p->age);
			const char* data[] = {
				p->patientId, p->name, p->gender, age,
				strlen(p->bedId) > 0 ? p->bedId : "无", p->phone
			};
			print_table_row(cols, aligns, data, 6);
		}
		p = p->next;
	}
	print_table_sep(cols, 6);
	char msg[80];
	snprintf(msg, sizeof(msg), "统计：共找到 %d 名匹配的患者。", cnt);
	PRINT_TIP(msg);
	// 重名提示保留
	int exactSameNameCount = 0;
	p = pathead->next;
	while (p != NULL) {
		if (strcmp(p->name, name) == 0) {
			exactSameNameCount++;
		}
		p = p->next;
	}
	if (exactSameNameCount > 1) {
		char warnMsg[100];
		snprintf(warnMsg, sizeof(warnMsg), "注意：查询到%d名姓名完全为【%s】的患者！", exactSameNameCount, name);
		PRINT_WARN(warnMsg);
		PRINT_TIP("请输入目标患者的ID进行精确查询，避免操作错误！");
	}
}

// 打印所有患者信息
void PrintAllPatient(void) {
	if (pathead == NULL || pathead->next == NULL) {
		PRINT_TIP("暂无患者信息！");
		return;
	}
	SystemRole role = GetCurrentRole();
	const char* currentDoc = GetCurrentOperator();
	int isDoctor = (role == ROLE_DOCTOR);
	if (isDoctor) print_title_box("您接诊的患者列表");
	else print_title_box("所有患者信息");
	int cols[] = { 12, 12, 8, 6, 12, 14 };
	AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
	const char* headers[] = { "患者ID", "姓名", "性别", "年龄", "床位ID", "联系方式" };
	printf("\n");
	print_table_sep(cols, 6);
	print_table_row(cols, aligns, headers, 6);
	print_table_sep(cols, 6);
	PatientNode* p = pathead->next;
	while (p != NULL) {
		if (isDoctor && !FindRecordByPatientAndDoctor(p->patientId, currentDoc)) {
			p = p->next;
			continue;
		}
		char age[10];
		snprintf(age, sizeof(age), "%d", p->age);
		const char* data[] = {
			p->patientId, p->name, p->gender, age,
			strlen(p->bedId) > 0 ? p->bedId : "无", p->phone
		};
		print_table_row(cols, aligns, data, 6);
		p = p->next;
	}
	print_table_sep(cols, 6);
}

// 患者费用明细统计
void StatPatientFeeDetail(const char* input) {
	const char* realId = GetPatientRealId(input);
	if (realId == NULL) {
		char msg[80];
		snprintf(msg, sizeof(msg), "患者【%s】不存在!", input);
		PRINT_ERR(msg);
		return;
	}
	char title[80];
	snprintf(title, sizeof(title), "患者费用明细报表（ID:%s）", realId);
	print_title_box(title);
	int cols[] = { 14, 18, 12, 16 };
	AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
	const char* headers[] = { "记录ID", "费用类型", "费用(元)", "产生时间" };
	printf("\n");
	print_table_sep(cols, 4);
	print_table_row(cols, aligns, headers, 4);
	print_table_sep(cols, 4);
	RecordNode* p = recordHead->next;
	int feeCount = 0;
	float totalFee = 0.0f;
	while (p != NULL) {
		if (strcmp(p->patientId, realId) == 0) {
			char typeStr[20] = { 0 };
			switch (p->type) {
			case REGISTER:    strcpy(typeStr, "挂号费"); break;
			case CONSULT:     strcpy(typeStr, "看诊费"); break;
			case EXAMINE:     strcpy(typeStr, "检查费"); break;
			case HOSPITALIZE: strcpy(typeStr, "住院费"); break;
			default:          strcpy(typeStr, "其他费用"); break;
			}
			char fee[20];
			snprintf(fee, sizeof(fee), "%.2f", p->fee);
			const char* data[] = { p->recordId, typeStr, fee, p->createTime };
			print_table_row(cols, aligns, data, 4);
			totalFee += p->fee;
			feeCount++;
		}
		p = p->next;
	}
	print_table_sep(cols, 4);
	if (feeCount == 0) PRINT_TIP("该患者暂无任何医疗费用记录！");
	else {
		char msg[80];
		snprintf(msg, sizeof(msg), "费用明细条数：%d 条，患者累计总费用：%.2f 元", feeCount, totalFee);
		PRINT_OK(msg);
	}
}

// 患者自助修改个人信息（仅可修改性别、年龄、电话，不可修改ID/姓名）
int ModifyPatientSelfInfo(const char* patientId) {
	if (patientId == NULL || strlen(patientId) == 0) {
		PRINT_ERR("患者ID不能为空！");
		return 0;
	}
	// 查找患者信息
	PatientNode* patient = FindPatientById(patientId);
	if (patient == NULL) {
		PRINT_ERR("操作失败：未找到该患者信息！");
		return 0;
	}
	CLEAR_SCREEN;
	print_title_box("修改个人信息");
	// 展示当前信息
	char info1[80], info2[80], info3[80], info4[80], info5[80];
	snprintf(info1, sizeof(info1), "患者ID：%s（不可修改）", patient->patientId);
	snprintf(info2, sizeof(info2), "患者姓名：%s（不可修改）", patient->name);
	snprintf(info3, sizeof(info3), "当前性别：%s", patient->gender);
	snprintf(info4, sizeof(info4), "当前年龄：%d", patient->age);
	snprintf(info5, sizeof(info5), "当前联系电话：%s", patient->phone);
	const char* lines[6] = { "当前个人信息", info1, info2, info3, info4, info5 };
	print_content_box(lines, 6);
	printf("\n");
	// 1. 修改性别
	int modifyGender = 0;
	SafeIntInput("是否修改性别？输入1=是，输入0=否", &modifyGender, 0, 1);
	if (modifyGender == 1) {
		char newGender[10] = { 0 };
		while (1) {
			SafeStrInput("请输入新的性别（仅允许输入：男/女）", newGender, 10);
			if (strcmp(newGender, "男") == 0 || strcmp(newGender, "女") == 0) {
				break;
			}
			PRINT_WARN("性别输入不合法！仅允许输入「男」或「女」，请重新输入！");
		}
		SafeStrCopy(patient->gender, newGender, 10);
		PRINT_OK("性别修改成功！");
	}
	// 2. 修改年龄
	int modifyAge = 0;
	SafeIntInput("是否修改年龄？输入1=是，输入0=否", &modifyAge, 0, 1);
	if (modifyAge == 1) {
		int newAge = 0;
		SafeIntInput("请输入新的年龄", &newAge, 0, 120);
		patient->age = newAge;
		PRINT_OK("年龄修改成功！");
	}
	// 3. 修改联系电话
	int modifyPhone = 0;
	SafeIntInput("是否修改联系电话？输入1=是，输入0=否", &modifyPhone, 0, 1);
	if (modifyPhone == 1) {
		char newPhone[MAX_ID_LEN] = { 0 };
		while (1) {
			SafeStrInput("请输入新的联系电话（纯数字）", newPhone, MAX_ID_LEN);
			if (!IsNumber(newPhone)) {
				PRINT_WARN("联系方式格式错误！必须是纯数字！");
				continue;
			}
			break;
		}
		SafeStrCopy(patient->phone, newPhone, MAX_ID_LEN);
		PRINT_OK("联系电话修改成功！");
	}
	printf("\n");
	PRINT_OK("个人信息修改流程完成！");
	WriteLog(LOG_LEVEL_INFO, patientId, "修改个人信息", "成功");
	return 1;
}


// 住院登记（含床位智能匹配）
int InHospitalRegister(const char* patientId, const char* deptId, const char* doctorId) {
	if (!CheckNullPtr(3, patientId, deptId, doctorId)) {
		PRINT_ERR("入参不合法，住院登记失败！");
		return 0;
	}
	PatientNode* patient = FindPatientById(patientId);
	if (patient == NULL) {
		PRINT_ERR("患者不存在，住院登记失败！");
		return 0;
	}
	if (patient->inHospitalStatus == INHOSPITAL_STATUS_INHOSPITAL) {
		PRINT_ERR("患者当前已在住院中，无法重复登记！");
		return 0;
	}
	// 智能匹配空闲床位
	char* matchBedId = AutoMatchFreeBedByDept(deptId);
	if (matchBedId == NULL) {
		PRINT_ERR("该科室暂无空闲床位，住院登记失败！");
		return 0;
	}
	PRINT_OK("床位智能匹配成功！");
	char info1[80], info2[80];
	snprintf(info1, sizeof(info1), "匹配床位编号：%s", matchBedId);
	snprintf(info2, sizeof(info2), "住院押金金额：%.2f元", DEFAULT_DEPOSIT_FEE);
	const char* lines[3] = { "住院登记信息", info1, info2 };
	print_content_box(lines, 3);
	// 二次确认
	int confirm = 0;
	SafeIntInput("确认完成住院登记？输入1=确认，输入0=取消", &confirm, 0, 1);
	if (confirm == 0) {
		PRINT_TIP("已取消住院登记！");
		return 0;
	}
	// 更新患者住院信息
	patient->patientType = PATIENT_INPATIENT;
	patient->inHospitalStatus = INHOSPITAL_STATUS_UNPAID;
	patient->depositFee = DEFAULT_DEPOSIT_FEE;
	SafeStrCopy(patient->bedId, matchBedId, MAX_ID_LEN);
	memset(patient->inTime, 0, MAX_DATA_LEN);
	memset(patient->outTime, 0, MAX_DATA_LEN);
	// 生成住院登记记录
	RecordNode newRecord = { 0 };
	srand((unsigned int)time(NULL));
	snprintf(newRecord.recordId, MAX_ID_LEN, "INHOS%06d", rand() % 1000000);
	SafeStrCopy(newRecord.patientId, patientId, MAX_ID_LEN);
	SafeStrCopy(newRecord.doctorId, doctorId, MAX_ID_LEN);
	SafeStrCopy(newRecord.deptId, deptId, MAX_ID_LEN);
	newRecord.type = HOSPITALIZE;
	SafeStrCopy(newRecord.createTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
	snprintf(newRecord.detail, MAX_DETAIL_LEN, "住院登记，匹配床位：%s，押金金额：%.2f元", matchBedId, DEFAULT_DEPOSIT_FEE);
	newRecord.fee = 0.0f;
	SafeStrCopy(newRecord.prescriptionId, matchBedId, MAX_ID_LEN);
	AddRecord(newRecord);
	// 记录日志
	char logContent[MAX_DETAIL_LEN] = { 0 };
	snprintf(logContent, MAX_DETAIL_LEN, "患者ID：%s，科室ID：%s，床位ID：%s，住院登记成功", patientId, deptId, matchBedId);
	WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "住院登记", logContent);
	PRINT_OK("住院登记成功！请完成押金缴费后办理入院！");
	return 1;
}
// 住院押金缴费【修复：doctorId未定义、空指针风险、参数类型不匹配】
int PayHospitalDeposit(const char* patientId) {
	if (!CheckNullPtr(1, patientId)) {
		PRINT_ERR("入参不合法，缴费失败！");
		return 0;
	}
	PatientNode* patient = FindPatientById(patientId);
	if (patient == NULL) {
		PRINT_ERR("患者不存在，缴费失败！");
		return 0;
	}
	if (patient->inHospitalStatus != INHOSPITAL_STATUS_UNPAID) {
		PRINT_ERR("该患者无待缴押金，无需重复缴费！");
		return 0;
	}

	// 修复：新增当前医生ID获取与空指针校验，彻底解决未定义标识符报错
	const char* doctorId = GetCurrentOperator();
	if (doctorId == NULL || strlen(doctorId) == 0) {
		PRINT_ERR("获取当前登录医生信息失败，缴费终止！");
		return 0;
	}
	DoctorNode* currentDoctor = GetDoctorById(doctorId);
	if (currentDoctor == NULL) {
		PRINT_ERR("当前登录医生信息不存在，缴费终止！");
		return 0;
	}

	// 缴费信息确认
	char info1[80], info2[80];
	snprintf(info1, sizeof(info1), "患者姓名：%s", patient->name);
	snprintf(info2, sizeof(info2), "待缴押金金额：%.2f元", patient->depositFee);
	const char* lines[3] = { "押金缴费信息", info1, info2 };
	print_content_box(lines, 3);
	// 二次确认
	int confirm = 0;
	SafeIntInput("确认缴纳住院押金？输入1=确认，输入0=取消", &confirm, 0, 1);
	if (confirm == 0) {
		PRINT_TIP("已取消缴费！");
		return 0;
	}
	// 更新患者状态为已缴费待入院
	patient->inHospitalStatus = INHOSPITAL_STATUS_PAID;
	// 生成押金缴费记录
	RecordNode newRecord = { 0 };
	srand((unsigned int)time(NULL));
	snprintf(newRecord.recordId, MAX_ID_LEN, "DEP%06d", rand() % 1000000);
	SafeStrCopy(newRecord.patientId, patientId, MAX_ID_LEN);
	SafeStrCopy(newRecord.doctorId, doctorId, MAX_ID_LEN);
	// 修复：使用校验后的医生对象获取科室ID，解决类型不匹配报错
	SafeStrCopy(newRecord.deptId, currentDoctor->deptId, MAX_ID_LEN);
	newRecord.type = HOSPITAL_DEPOSIT;
	SafeStrCopy(newRecord.createTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
	snprintf(newRecord.detail, MAX_DETAIL_LEN, "住院押金缴纳，金额：%.2f元", patient->depositFee);
	newRecord.fee = patient->depositFee;
	SafeStrCopy(newRecord.prescriptionId, patient->bedId, MAX_ID_LEN);
	AddRecord(newRecord);
	// 记录日志
	char logContent[MAX_DETAIL_LEN] = { 0 };
	snprintf(logContent, MAX_DETAIL_LEN, "患者ID：%s，押金金额：%.2f元，缴费成功", patientId, patient->depositFee);
	WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "住院押金缴费", logContent);
	PRINT_OK("押金缴纳成功！请前往护士站办理入院手续！");
	return 1;
}
// 办理入院（床位分配+患者状态联动）
int HandleInHospitalCheckIn(const char* patientId) {
	if (!CheckNullPtr(1, patientId)) {
		PRINT_ERR("入参不合法，入院办理失败！");
		return 0;
	}
	PatientNode* patient = FindPatientById(patientId);
	if (patient == NULL) {
		PRINT_ERR("患者不存在，入院办理失败！");
		return 0;
	}
	if (patient->inHospitalStatus != INHOSPITAL_STATUS_PAID) {
		PRINT_ERR("该患者未缴纳押金，无法办理入院！");
		return 0;
	}
	// 分配床位
	if (!AssignBedToPatient(patient->bedId, patientId, GetCurrentTimeMMDDHHMM())) {
		PRINT_ERR("床位分配失败，入院办理失败！");
		return 0;
	}
	// 更新患者状态为住院中
	patient->inHospitalStatus = INHOSPITAL_STATUS_INHOSPITAL;
	SafeStrCopy(patient->inTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
	// 记录日志
	char logContent[MAX_DETAIL_LEN] = { 0 };
	snprintf(logContent, MAX_DETAIL_LEN, "患者ID：%s，床位ID：%s，入院办理成功", patientId, patient->bedId);
	WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "入院办理", logContent);
	PRINT_OK("入院办理成功！患者已正式入住！");
	return 1;
}
// 出院结算办理
int HandleHospitalDischarge(const char* patientId, float settleFee) {
	if (!CheckNullPtr(1, patientId)) {
		PRINT_ERR("入参不合法，出院结算失败！");
		return 0;
	}
	PatientNode* patient = FindPatientById(patientId);
	if (patient == NULL) {
		PRINT_ERR("患者不存在，出院结算失败！");
		return 0;
	}
	if (patient->inHospitalStatus != INHOSPITAL_STATUS_INHOSPITAL) {
		PRINT_ERR("该患者当前未在住院中，无法办理出院！");
		return 0;
	}
	// 结算金额校验
	if (!CheckFeeValid(settleFee)) {
		PRINT_ERR("结算金额不合法！");
		return 0;
	}
	// 结算信息计算
	float refundFee = patient->depositFee - settleFee;
	char info1[80], info2[80], info3[80], info4[80];
	snprintf(info1, sizeof(info1), "患者姓名：%s", patient->name);
	snprintf(info2, sizeof(info2), "住院押金：%.2f元", patient->depositFee);
	snprintf(info3, sizeof(info3), "本次结算总费用：%.2f元", settleFee);
	if (refundFee >= 0) {
		snprintf(info4, sizeof(info4), "应退金额：%.2f元", refundFee);
	}
	else {
		snprintf(info4, sizeof(info4), "需补缴金额：%.2f元", fabs(refundFee));
	}
	const char* lines[5] = { "出院结算信息", info1, info2, info3, info4 };
	print_content_box(lines, 5);
	// 二次确认
	int confirm = 0;
	SafeIntInput("确认办理出院结算？输入1=确认，输入0=取消", &confirm, 0, 1);
	if (confirm == 0) {
		PRINT_TIP("已取消出院结算！");
		return 0;
	}
	// 补缴金额校验
	if (refundFee < 0) {
		int payConfirm = 0;
		SafeIntInput("请先补缴差额，确认已缴费？输入1=确认，输入0=取消", &payConfirm, 0, 1);
		if (payConfirm == 0) {
			PRINT_TIP("已取消出院结算！");
			return 0;
		}
	}
	// 释放床位
	ReleaseBed(patient->bedId);
	// 更新患者状态为已出院，恢复为门诊患者
	patient->patientType = PATIENT_OUTPATIENT;
	patient->inHospitalStatus = INHOSPITAL_STATUS_DISCHARGED;
	SafeStrCopy(patient->outTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
	// 生成出院结算记录
	RecordNode newRecord = { 0 };
	srand((unsigned int)time(NULL));
	snprintf(newRecord.recordId, MAX_ID_LEN, "DISCHG%06d", rand() % 1000000);
	SafeStrCopy(newRecord.patientId, patientId, MAX_ID_LEN);
	SafeStrCopy(newRecord.doctorId, GetCurrentOperator(), MAX_ID_LEN);
	SafeStrCopy(newRecord.deptId, GetDoctorById(GetCurrentOperator())->deptId, MAX_ID_LEN);
	newRecord.type = HOSPITAL_DISCHARGE;
	SafeStrCopy(newRecord.createTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
	snprintf(newRecord.detail, MAX_DETAIL_LEN, "出院结算，总费用：%.2f元，押金：%.2f元，退费：%.2f元", settleFee, patient->depositFee, refundFee);
	newRecord.fee = settleFee;
	SafeStrCopy(newRecord.prescriptionId, patient->bedId, MAX_ID_LEN);
	AddRecord(newRecord);
	// 清空患者床位信息
	memset(patient->bedId, 0, MAX_ID_LEN);
	patient->depositFee = 0.0f;
	// 记录日志
	char logContent[MAX_DETAIL_LEN] = { 0 };
	snprintf(logContent, MAX_DETAIL_LEN, "患者ID：%s，出院结算成功，总费用：%.2f元", patientId, settleFee);
	WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "出院结算", logContent);
	PRINT_OK("出院结算办理成功！患者已正式出院！");
	return 1;
}

void PrintAllPatientBrief(void) {
	if (pathead == NULL || pathead->next == NULL) {
		PRINT_TIP("暂无患者数据！");
		return;
	}
	print_title_box("患者列表（ID+姓名）");
	int cols[] = { 20, 58 };
	AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER };
	const char* headers[] = { "患者ID", "患者姓名" };
	printf("\n");
	print_table_sep(cols, 2);
	print_table_row(cols, aligns, headers, 2);
	print_table_sep(cols, 2);
	PatientNode* p = pathead->next;
	while (p != NULL) {
		const char* data[] = { p->patientId, p->name };
		print_table_row(cols, aligns, data, 2);
		p = p->next;
	}
	print_table_sep(cols, 2);
}