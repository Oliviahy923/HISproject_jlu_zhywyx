// 先包含系统头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 【必须优先包含common.h，统一全局打印规范】
#include "common.h"
// 再包含自定义业务头文件
#include "safe_utils.h"
#include "doctor.h"
#include "record.h"
#include "department.h"

// ==================== 内部私有结构体与静态变量 ====================
static DoctorList docHead = NULL;

// ==================== 内部私有辅助函数 ====================
static DoctorNode* FindDoctorById(const char* doctorId)
{
    if (docHead == NULL || doctorId == NULL) {
        return NULL;
    }
    DoctorNode* p = docHead->next;
    while (p != NULL) {
        if (strcmp(p->doctorId, doctorId) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}
// ==================== 对外接口函数（【修复4：所有参数const与头文件声明100%匹配】）====================
// 初始化医生链表（仅1个提示框）
void InitDoctorList(void)
{
    docHead = (DoctorList)malloc(sizeof(DoctorNode));
    if (docHead == NULL)
    {
        PRINT_ERR("医生模块初始化失败");
        return;
    }
    memset(docHead, 0, sizeof(DoctorNode));
    docHead->next = NULL;
    PRINT_OK("医生模块初始化成功");
}

// 医生数据保存到文件（仅1个提示框）
int SaveDoctorTofile(const char* filename)
{
    FILE* fp = fopen(filename, "w");
    if (fp == NULL)
    {
        PRINT_ERR("医生数据保存失败");
        return 0;
    }
    DoctorNode* p = docHead->next;
    while (p != NULL)
    {
        fprintf(fp, "%s,%s,%s,%s,%s,%d,%d\n",
            p->doctorId, p->name, p->deptId, p->title, p->phone,
            p->registerLimit, p->todayRegisterCount);
        p = p->next;
    }
    fclose(fp);
    PRINT_OK("医生数据保存成功");
    return 1;
}

// 新增医生：加isLoad参数，加载时不弹提示，仅手动操作弹
int AddDoctor(DoctorNode newDoctor, int isLoad) {
    int mode = 1;
    if (!CheckDoctorValid(&newDoctor, mode)) {
        return 0;
    }
    // ========== 重名校验核心代码 ==========
    if (!isLoad) {
        DoctorNode* p = docHead->next;
        int sameNameCount = 0;
        while (p != NULL) {
            if (strcmp(p->name, newDoctor.name) == 0) {
                sameNameCount++;
            }
            p = p->next;
        }
        if (sameNameCount > 0) {
            char msg[100];
            snprintf(msg, sizeof(msg), "警告：系统中已存在%d名姓名为【%s】的医生！", sameNameCount, newDoctor.name);
            PRINT_WARN(msg);
            int confirm = 0;
            SafeIntInput("是否确认继续新增该同名医生？输入1=确认，输入0=取消", &confirm, 0, 1);
            if (confirm == 0) {
                PRINT_TIP("已取消新增医生操作！");
                return 0;
            }
        }
    }
    // ========== 重名校验结束 ==========
    DoctorNode* newNode = (DoctorNode*)malloc(sizeof(DoctorNode));
    if (newNode == NULL) {
        if (!isLoad) PRINT_ERR("内存分配失败，新增失败");
        return 0;
    }
    *newNode = newDoctor;
    newNode->next = docHead->next;
    docHead->next = newNode;
    if (!isLoad) {
        PRINT_OK("医生新增成功");
    }
    return 1;
}

// 加载医生数据：仅最后1个总提示框，中间无任何提示
int LoadDoctorFromFile(const char* fileName)
{
    FILE* fp = fopen(fileName, "r");
    if (fp == NULL)
    {
        // 仅1个提示框：文件不存在，加载0条
        const char* lines[2] = { "【成功】", "医生数据加载完成，共0条" };
        print_content_box(lines, 2);
        return 0;
    }

    DoctorNode newdoctor;
    int loadCount = 0;

    // 加载时调用AddDoctor，传isLoad=1，屏蔽中间所有成功提示
    while (fscanf(fp, "%[^,],%[^,],%[^,],%[^,],%[^,],%d,%d\n",
        newdoctor.doctorId, newdoctor.name, newdoctor.deptId,
        newdoctor.title, newdoctor.phone, &newdoctor.registerLimit, &newdoctor.todayRegisterCount) == 7)
    {
        if (AddDoctor(newdoctor, 1) == 1)
        {
            loadCount++;
        }
    }
    fclose(fp);

    // 仅最后1个总提示框：显示加载结果+总条数
    char msg[100];
    snprintf(msg, sizeof(msg), "医生数据加载完成，共%d条", loadCount);
    const char* lines[2] = { "【成功】", msg };
    print_content_box(lines, 2);

    return 1;
}

// 释放链表内存（无冗余提示）
void FreeDoctorList(void)
{
    DoctorList next, p = docHead;
    while (p != NULL)
    {
        next = p->next;
        free(p);
        p = next;
    }
    docHead = NULL;
}

// 校验医生数据合法性（功能完全不变）
int CheckDoctorValid(const DoctorNode* newDoctor, int mode)
{
    if (strlen(newDoctor->doctorId) == 0 || strlen(newDoctor->doctorId) >= MAX_ID_LEN)
    {
        PRINT_ERR("医生ID不合法"); return 0;
    }
    if (!IsAlphaNumber(newDoctor->doctorId))
    {
        PRINT_ERR("医生ID格式错误"); return 0;
    }
    if (mode == 1 && FindDoctorById(newDoctor->doctorId) != NULL)
    {
        PRINT_ERR("医生ID重复"); return 0;
    }
    if (strlen(newDoctor->deptId) == 0 || !CheckDepartmentExist(newDoctor->deptId))
    {
        PRINT_ERR("科室ID不合法"); return 0;
    }
    if (strlen(newDoctor->name) == 0 || HasNumber(newDoctor->name))
    {
        PRINT_ERR("姓名不合法"); return 0;
    }
    if (strlen(newDoctor->title) == 0 || HasNumber(newDoctor->title))
    {
        PRINT_ERR("职称不合法"); return 0;
    }
    if (strlen(newDoctor->phone) == 0 || !IsNumber(newDoctor->phone))
    {
        PRINT_ERR("联系方式不合法"); return 0;
    }
    if (newDoctor->registerLimit <= 0 || newDoctor->registerLimit > 100)
    {
        PRINT_ERR("挂号限额不合法"); return 0;
    }
    return 1;
}

// 修改医生信息（仅1个提示框）
int ModifyDoctor(const char* doctorId, DoctorNode newData)
{
    DoctorNode* p = FindDoctorById(doctorId);
    if (p == NULL) { PRINT_ERR("医生不存在，修改失败"); return 0; }
    int mode = 2;
    if (!CheckDoctorValid(&newData, mode)) return 0;
    SafeStrCopy(p->name, newData.name, MAX_NAME_LEN);
    SafeStrCopy(p->deptId, newData.deptId, MAX_ID_LEN);
    SafeStrCopy(p->title, newData.title, MAX_NAME_LEN);
    SafeStrCopy(p->phone, newData.phone, MAX_ID_LEN);
    p->registerLimit = newData.registerLimit;
    PRINT_OK("医生信息修改成功");
    return 1;
}

// 打印所有医生信息
void PrintAllDoctor(void)
{
    DoctorNode* p = docHead->next;
    if (p == NULL) {
        const char* lines[] = { "暂无医生信息" };
        print_content_box(lines, 1);
        return;
    }

    print_title_box("所有医生信息");
    // 修正列宽：总宽度严格等于80，和标题框、分割线对齐
    int cols[] = { 10, 10, 10, 12, 9, 10 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "医生ID","姓名","科室ID","职称","每日限额","当日挂号数" };

    print_table_sep(cols, 6);
    print_table_row(cols, aligns, headers, 6);
    print_table_sep(cols, 6);

    char lim[10], cnt[10];
    while (p != NULL)
    {
        snprintf(lim, sizeof(lim), "%d", p->registerLimit);
        snprintf(cnt, sizeof(cnt), "%d", p->todayRegisterCount);
        const char* data[] = { p->doctorId, p->name, p->deptId, p->title, lim, cnt };
        print_table_row(cols, aligns, data, 6);
        p = p->next;
    }
    print_table_sep(cols, 6);
}

// 【修复】参数const匹配
int AddDoctorRegisterCount(const char* doctorId)
{
    DoctorNode* p = FindDoctorById(doctorId);
    if (!p) { PRINT_ERR("挂号失败，医生不存在"); return 0; }
    if (p->todayRegisterCount >= p->registerLimit) { PRINT_ERR("挂号失败，今日已满"); return 0; }
    p->todayRegisterCount++;
    return 1;
}

// 【修复】参数const匹配，宏名与common.h一致
void StatDoctorVisitCount(const char* doctorId, const char* startTime, const char* endTime)
{
    if (!CheckDoctorExist(doctorId)) { PRINT_ERR("医生不存在"); return; }
    char title[100];
    snprintf(title, sizeof(title), "医生%s接诊统计", doctorId);
    print_title_box(title);

    // 修正列宽：总宽度严格等于80
    int cols[] = { 12,12,14,14,12 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "记录ID","患者ID","记录类型","时间","费用" };

    print_table_sep(cols, 5);
    print_table_row(cols, aligns, headers, 5);
    print_table_sep(cols, 5);

    int totalCount = 0;
    RecordNode* p = GetRecordListHead();
    char fee[10], type[20];
    while (p != NULL)
    {
        if (strcmp(p->doctorId, doctorId) == 0
            && CompareTime(p->createTime, startTime) >= 0
            && CompareTime(p->createTime, endTime) <= 0)
        {
            // 使用与common.h一致的宏名
            switch (p->type) {
            case RECORD_REGISTER: strcpy(type, "挂号"); break;
            case RECORD_CONSULT: strcpy(type, "看诊"); break;
            case RECORD_EXAMINE: strcpy(type, "检查"); break;
            case RECORD_HOSPITALIZE: strcpy(type, "住院"); break;
            default: strcpy(type, "未知"); break;
            }
            snprintf(fee, sizeof(fee), "%.2f", p->fee);
            const char* data[] = { p->recordId, p->patientId, type, p->createTime, fee };
            print_table_row(cols, aligns, data, 5);
            totalCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 5);
}

// 【修复】参数const匹配
void StatDeptPatientDistribution(const char* deptId)
{
    char title[100];
    snprintf(title, sizeof(title), "科室%s患者统计", deptId);
    print_title_box(title);

    // 修正列宽：总宽度严格等于80
    int cols[] = { 16,16,17,18 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "医生ID","医生姓名","接诊人次","接诊患者数(去重)" };

    print_table_sep(cols, 4);
    print_table_row(cols, aligns, headers, 4);
    print_table_sep(cols, 4);

    DoctorNode* docP = docHead->next;
    char vc[10], pc[10];
    while (docP != NULL)
    {
        if (strcmp(docP->deptId, deptId) == 0) {
            int visitCount = 0, patientCount = 0;
            char patientIdList[100][MAX_ID_LEN] = { 0 };
            RecordNode* recP = GetRecordListHead();
            while (recP != NULL)
            {
                if (strcmp(recP->doctorId, docP->doctorId) == 0)
                {
                    visitCount++;
                    int isExist = 0;
                    for (int i = 0; i < patientCount; i++)
                        if (strcmp(patientIdList[i], recP->patientId) == 0) isExist = 1;
                    if (!isExist) SafeStrCopy(patientIdList[patientCount++], recP->patientId, MAX_ID_LEN);
                }
                recP = recP->next;
            }
            snprintf(vc, sizeof(vc), "%d", visitCount);
            snprintf(pc, sizeof(pc), "%d", patientCount);
            const char* data[] = { docP->doctorId, docP->name, vc, pc };
            print_table_row(cols, aligns, data, 4);
        }
        docP = docP->next;
    }
    print_table_sep(cols, 4);
}

// 【修复】参数const匹配
int CheckDoctorExist(const char* doctorId)
{
    return FindDoctorById(doctorId) != NULL ? 1 : 0;
}

// 【修复】参数const匹配
void QueryDoctorById(const char* doctorId)
{
    DoctorNode* p = FindDoctorById(doctorId);
    if (!p) { PRINT_ERR("医生不存在"); return; }
    print_title_box("医生详情");
    char info[6][80];
    snprintf(info[0], sizeof(info[0]), "医生ID: %s", p->doctorId);
    snprintf(info[1], sizeof(info[1]), "姓名: %s", p->name);
    snprintf(info[2], sizeof(info[2]), "科室ID: %s", p->deptId);
    snprintf(info[3], sizeof(info[3]), "职称: %s", p->title);
    snprintf(info[4], sizeof(info[4]), "联系方式: %s", p->phone);
    snprintf(info[5], sizeof(info[5]), "限额:%d  已挂号:%d", p->registerLimit, p->todayRegisterCount);
    const char* lines[] = { info[0],info[1],info[2],info[3],info[4],info[5] };
    print_content_box(lines, 6);
}

// 删除医生（仅1个提示框）
int DeleteDoctor(const char* doctorId)
{
    DoctorNode* p = docHead;
    int found = 0;
    while (p->next != NULL)
    {
        if (strcmp(p->next->doctorId, doctorId) == 0) { found = 1; break; }
        p = p->next;
    }
    if (!found) { PRINT_ERR("医生不存在，删除失败"); return 0; }
    DoctorNode* delNode = p->next;
    p->next = delNode->next;
    free(delNode);
    PRINT_OK("医生删除成功");
    return 1;
}

//按照科室查找医生
void QueryDoctorByDept(const char* deptId) {
    if (!CheckNullPtr(1, deptId) || docHead == NULL || docHead->next == NULL) {
        const char* lines[] = { "暂无医生信息" };
        print_content_box(lines, 1);
        return;
    }
    // 先统计匹配数量
    int findCount = 0;
    DoctorNode* p = docHead->next;
    while (p != NULL) {
        if (strcmp(p->deptId, deptId) == 0) {
            findCount++;
        }
        p = p->next;
    }
    // 为空直接提示
    if (findCount == 0) {
        char msg[80];
        snprintf(msg, sizeof(msg), "科室%s暂无医生信息", deptId);
        const char* lines[] = { msg };
        print_content_box(lines, 1);
        return;
    }
    // 不为空再打印标题和表格
    char title[80];
    snprintf(title, sizeof(title), "科室%s医生列表", deptId);
    print_title_box(title);
    // 修正列宽：总宽度严格等于80
    int cols[] = { 12,12,12,14,14 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "医生ID","姓名","职称","每日限额","当日挂号数" };
    printf("\n");
    print_table_sep(cols, 5);
    print_table_row(cols, aligns, headers, 5);
    print_table_sep(cols, 5);
    char lim[10], cnt[10];
    p = docHead->next;
    while (p != NULL) {
        if (strcmp(p->deptId, deptId) == 0) {
            snprintf(lim, sizeof(lim), "%d", p->registerLimit);
            snprintf(cnt, sizeof(cnt), "%d", p->todayRegisterCount);
            const char* data[] = { p->doctorId,p->name,p->title,lim,cnt };
            print_table_row(cols, aligns, data, 5);
        }
        p = p->next;
    }
    print_table_sep(cols, 5);
    char tip[80];
    snprintf(tip, sizeof(tip), "共找到%d名医生", findCount);
    PRINT_TIP(tip);
}

//按照名字查找医生
void QueryDoctorByName(const char* name) {
    if (name == NULL || docHead == NULL || docHead->next == NULL) {
        const char* lines[] = { "暂无医生信息" };
        print_content_box(lines, 1);
        return;
    }
    // 先统计匹配数量
    int findCount = 0;
    DoctorNode* p = docHead->next;
    while (p != NULL) {
        if (strstr(p->name, name) != NULL) {
            findCount++;
        }
        p = p->next;
    }
    // 为空直接提示
    if (findCount == 0) {
        char msg[80];
        snprintf(msg, sizeof(msg), "未找到姓名含【%s】的医生", name);
        const char* lines[] = { msg };
        print_content_box(lines, 1);
        return;
    }
    // 不为空再打印标题和表格
    char title[80];
    snprintf(title, sizeof(title), "查询医生：%s", name);
    print_title_box(title);
    // 修正列宽：总宽度严格等于80
    int cols[] = { 12,12,12,14,14 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "医生ID","姓名","科室ID","职称","联系方式" };
    printf("\n");
    print_table_sep(cols, 5);
    print_table_row(cols, aligns, headers, 5);
    print_table_sep(cols, 5);
    p = docHead->next;
    while (p != NULL) {
        if (strstr(p->name, name) != NULL) {
            const char* data[] = { p->doctorId,p->name,p->deptId,p->title,p->phone };
            print_table_row(cols, aligns, data, 5);
        }
        p = p->next;
    }
    print_table_sep(cols, 5);
    char tip[80];
    snprintf(tip, sizeof(tip), "共找到%d名匹配的医生", findCount);
    PRINT_TIP(tip);
}

// 【修复】参数const匹配
int SubDoctorRegisterCount(const char* doctorId)
{
    DoctorNode* p = FindDoctorById(doctorId);
    if (!p) return 0;
    if (p->todayRegisterCount > 0) p->todayRegisterCount--;
    return 1;
}

// ==================== 对外暴露：按ID获取医生结构体指针（头文件已声明，补全实现） ====================
DoctorList GetDoctorById(const char* doctorId)
{
    // 直接复用你原本已经写好的内部查找函数，零重复代码、零逻辑改动
    return FindDoctorById(doctorId);
}

// 按科室ID+姓名关键词模糊查询医生（仅当前科室），统一格式打印，返回匹配数量，唯一匹配时输出医生ID
int QueryDoctorByDeptAndName(const char* deptId, const char* nameKeyword, char* outDoctorId, int outIdLen)
{
    if (deptId == NULL || nameKeyword == NULL || outDoctorId == NULL || outIdLen <= 0) {
        return 0;
    }
    if (docHead == NULL || docHead->next == NULL) {
        PRINT_TIP("暂无医生信息！");
        return 0;
    }

    // 初始化输出参数
    memset(outDoctorId, 0, outIdLen);

    // 用你全局统一的标题框格式
    char title[100];
    snprintf(title, sizeof(title), "科室%s 匹配医生列表", deptId);
    print_title_box(title);

    // 复用你已有的表格打印规范，和QueryDoctorByDept完全一致
    int cols[] = { 12, 12, 12, 14, 14 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "医生ID","姓名","科室ID","职称","当日挂号数" };

    // 【修复5：调用你doctor.c里的无参print_table_sep()，不传参数】
    print_table_sep(cols, 5);
    print_table_row(cols, aligns, headers, 5);
    print_table_sep(cols, 5);

    int matchCount = 0;
    char matchDoctorId[MAX_ID_LEN] = { 0 };
    DoctorNode* p = docHead->next;
    char regCount[10];

    // 【核心修复】仅查询当前科室的医生，过滤其他科室
    while (p != NULL) {
        if (strcmp(p->deptId, deptId) == 0 && strstr(p->name, nameKeyword) != NULL) {
            snprintf(regCount, sizeof(regCount), "%d", p->todayRegisterCount);
            const char* data[] = { p->doctorId, p->name, p->deptId, p->title, regCount };
            print_table_row(cols, aligns, data, 5);

            // 记录唯一匹配的医生ID
            SafeStrCopy(matchDoctorId, p->doctorId, MAX_ID_LEN);
            matchCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 5);

    // 仅当匹配结果唯一时，返回医生ID
    if (matchCount == 1) {
        SafeStrCopy(outDoctorId, matchDoctorId, outIdLen);
    }

    // 统一格式提示
    char msg[80];
    snprintf(msg, sizeof(msg), "共匹配到 %d 名医生", matchCount);
    PRINT_TIP(msg);

    return matchCount;
}

// 按姓名精确校验医生是否存在（1=存在，0=不存在）
int CheckDoctorExistByName(const char* name)
{
    if (name == NULL || docHead == NULL || docHead->next == NULL) {
        return 0;
    }
    DoctorNode* p = docHead->next;
    while (p != NULL) {
        if (strcmp(p->name, name) == 0) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

// 按姓名精确匹配医生，返回对应的医生ID（找不到返回NULL）
char* GetDoctorIdByName(const char* name)
{
    static char doctorId[MAX_ID_LEN] = { 0 };
    memset(doctorId, 0, sizeof(doctorId));
    if (name == NULL || docHead == NULL || docHead->next == NULL) {
        return NULL;
    }
    DoctorNode* p = docHead->next;
    while (p != NULL) {
        if (strcmp(p->name, name) == 0) {
            SafeStrCopy(doctorId, p->doctorId, MAX_ID_LEN);
            return doctorId;
        }
        p = p->next;
    }
    return NULL;
}

// 打印所有医生简略信息（ID+姓名+科室）
void PrintAllDoctorBrief(void) {
    if (docHead == NULL || docHead->next == NULL) {
        PRINT_TIP("暂无医生数据！");
        return;
    }
    print_title_box("医生列表（ID+姓名+科室）");
    int cols[] = { 12, 12, 54 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "医生ID", "姓名", "所属科室ID" };
    printf("\n");
    print_table_sep(cols, 3);
    print_table_row(cols, aligns, headers, 3);
    print_table_sep(cols, 3);
    DoctorNode* p = docHead->next;
    while (p != NULL) {
        const char* data[] = { p->doctorId, p->name, p->deptId };
        print_table_row(cols, aligns, data, 3);
        p = p->next;
    }
    print_table_sep(cols, 3);
}
