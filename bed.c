// 先包含系统头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 再包含自定义头文件（优先包含common.h，复用全局定义）
#include "common.h"
#include "safe_utils.h"
#include "log.h"
#include "bed.h"
#include "patient.h" // 补充：解决UpdatePatientType隐式声明问题
#include "department.h" // 补充：解决CheckDepartmentExist未定义警告

// ==================== 内部私有结构体与静态变量 ====================
static BedNode* bedHead = NULL;

// ==================== 内部私有辅助函数 ====================
// 按ID查找床位（内部使用）
static BedNode* FindBedById(const char* bedId) {
    if (bedId == NULL || bedHead == NULL || bedHead->next == NULL) {
        return NULL;
    }
    BedNode* p = bedHead->next;
    while (p != NULL) {
        if (strcmp(p->bedId, bedId) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

// ==================== 对外暴露的函数实现 ====================
// 初始化床位链表
void InitBedList(void) {
    if (bedHead != NULL) {
        FreeBedList();
    }
    bedHead = (BedNode*)malloc(sizeof(BedNode));
    if (bedHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "初始化床位链表", "失败：内存分配失败");
        PRINT_ERR("床位模块初始化失败，内存不足！");
        return;
    }
    memset(bedHead, 0, sizeof(BedNode));
    bedHead->next = NULL;
    WriteLog(LOG_LEVEL_INFO, "系统", "初始化床位链表", "成功");
    PRINT_OK("床位模块初始化成功！");
}

// 校验床位是否存在
int CheckBedExist(const char* bedId) {
    return FindBedById(bedId) != NULL ? 1 : 0;
}

// 校验床位是否空闲
int CheckBedAvailable(const char* bedId) {
    BedNode* p = FindBedById(bedId);
    if (p == NULL) return 0;
    return p->status == BED_FREE ? 1 : 0;
}

// 分配床位给患者
int AssignBedToPatient(const char* bedId, const char* patientId, const char* inTime) {
    if (!CheckNullPtr(3, bedId, patientId, inTime)) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "分配床位", "失败：入参不合法");
        PRINT_ERR("床位ID、患者ID、入院时间不能为空！");
        return 0;
    }
    BedNode* p = FindBedById(bedId);
    if (p == NULL) {
        PRINT_ERR("床位不存在，分配失败！");
        WriteLog(LOG_LEVEL_ERROR, "医生", "分配床位", "失败，床位不存在");
        return 0;
    }
    if (p->status == BED_OCCUPIED) {
        PRINT_ERR("床位已被占用，分配失败！");
        WriteLog(LOG_LEVEL_ERROR, "医生", "分配床位", "失败，床位已占用");
        return 0;
    }
    // 赋值床位信息
    p->status = BED_OCCUPIED;
    SafeStrCopy(p->patientId, patientId, MAX_ID_LEN);
    SafeStrCopy(p->inTime, inTime, MAX_DATA_LEN);
    // 记录日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "床位ID：%s，分配给患者：%s，入院时间：%s", bedId, patientId, inTime);
    WriteLog(LOG_LEVEL_INFO, "医生", "分配床位", logContent);
    char successMsg[100] = { 0 };
    snprintf(successMsg, 100, "患者%s已入住床位%s！", patientId, bedId);
    PRINT_OK(successMsg);
    return 1;
}

// 释放床位
int ReleaseBed(const char* bedId) {
    if (!CheckNullPtr(1, bedId)) {
        PRINT_ERR("床位ID不能为空！");
        return 0;
    }
    BedNode* p = FindBedById(bedId);
    if (p == NULL) {
        PRINT_ERR("床位不存在，释放失败！");
        WriteLog(LOG_LEVEL_ERROR, "医生", "释放床位", "失败，床位不存在");
        return 0;
    }
    if (p->status == BED_FREE) {
        PRINT_TIP("该床位已处于空闲状态，无需释放！");
        WriteLog(LOG_LEVEL_INFO, "医生", "释放床位", "床位已空闲，无需操作");
        return 0;
    }
    // 清空床位信息
    char patientId[MAX_ID_LEN] = { 0 };
    SafeStrCopy(patientId, p->patientId, MAX_ID_LEN);
    p->status = BED_FREE;
    memset(p->patientId, 0, sizeof(p->patientId));
    memset(p->inTime, 0, sizeof(p->inTime));
    // 记录日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "床位ID：%s，患者%s出院", bedId, patientId);
    WriteLog(LOG_LEVEL_INFO, "医生", "释放床位", logContent);
    char successMsg[100] = { 0 };
    snprintf(successMsg, 100, "患者%s已办理出院！", patientId);
    PRINT_OK(successMsg);
    return 1;
}

// 交互式释放床位
int ReleaseBedInteractive(void) {
    if (bedHead == NULL) {
        PRINT_ERR("床位模块还没初始化！");
        return 0;
    }
    char bedId[MAX_ID_LEN] = { 0 };
    print_title_box("释放床位");
    PRINT_TIP("现有床位列表");
    PrintAllBedBrief();
    printf("\n");
    SafeStrInput("请输入要释放的床位编号", bedId, MAX_ID_LEN);
    BedNode* p = FindBedById(bedId);
    if (p == NULL) {
        PRINT_ERR("该床位编号不存在，释放失败！");
        WriteLog(LOG_LEVEL_ERROR, "医生", "释放床位", "失败，床位不存在");
        return 0;
    }
    if (p->status == BED_FREE) {
        const char* lines[2] = { "【提示】", "该床位已处于空闲状态，无需释放！" };
        print_content_box(lines, 2);
        WriteLog(LOG_LEVEL_INFO, "医生", "释放床位", "床位已空闲，无需操作");
        return 0;
    }
    int confirm = 0;
    SafeIntInput("确认要释放这个床位吗？患者将办理出院！输入1确认，输入0取消", &confirm, 0, 1);
    if (confirm == 0) {
        const char* lines[2] = { "【提示】", "已取消释放操作！" };
        print_content_box(lines, 2);
        WriteLog(LOG_LEVEL_INFO, "医生", "释放床位", "用户取消操作");
        return 0;
    }
    char patientId[MAX_ID_LEN] = { 0 };
    SafeStrCopy(patientId, p->patientId, MAX_ID_LEN);
    p->status = BED_FREE;
    memset(p->patientId, 0, sizeof(p->patientId));
    memset(p->inTime, 0, sizeof(p->inTime));
    UpdatePatientType(patientId, PATIENT_OUTPATIENT, "", "");
    // 记录日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "床位ID：%s，患者%s出院", bedId, patientId);
    WriteLog(LOG_LEVEL_INFO, "医生", "释放床位", logContent);
    char successMsg[100] = { 0 };
    snprintf(successMsg, 100, "患者%s已办理出院！", patientId);
    PRINT_OK(successMsg);
    return 1;
}

// 修改床位信息
int ModifyBed(void) {
    if (bedHead == NULL) {
        PRINT_ERR("床位模块未初始化！");
        return 0;
    }
    char bedId[MAX_ID_LEN] = { 0 };
    print_title_box("修改床位信息");
    PRINT_TIP("现有床位列表");
    PrintAllBedBrief();
    printf("\n");
    SafeStrInput("请输入要修改的床位编号", bedId, MAX_ID_LEN);
    BedNode* p = FindBedById(bedId);
    if (p == NULL) {
        PRINT_ERR("操作失败：床位编号不存在！");
        WriteLog(LOG_LEVEL_ERROR, "管理员", "修改床位", "失败，床位不存在");
        return 0;
    }
    if (p->status == BED_OCCUPIED) {
        PRINT_ERR("操作失败：该床位当前处于占用状态，禁止修改！请先释放床位后再试。");
        WriteLog(LOG_LEVEL_ERROR, "管理员", "修改床位", "失败，床位处于占用状态");
        return 0;
    }
    // 当前信息展示
    char info1[80] = { 0 }, info2[80] = { 0 }, info3[80] = { 0 }, info4[80] = { 0 };
    snprintf(info1, 80, "床位编号：%s", p->bedId);
    snprintf(info2, 80, "病房类型：%s", p->wardType);
    snprintf(info3, 80, "关联科室：%s", p->deptId);
    snprintf(info4, 80, "当前状态：空闲");
    const char* lines[5] = { "当前床位信息", info1, info2, info3, info4 };
    print_content_box(lines, 5);
    char oldWardType[MAX_NAME_LEN] = { 0 };
    char oldDeptId[MAX_ID_LEN] = { 0 };
    SafeStrCopy(oldWardType, p->wardType, MAX_NAME_LEN);
    SafeStrCopy(oldDeptId, p->deptId, MAX_ID_LEN);
    char newWardType[MAX_NAME_LEN] = { 0 };
    char newDeptId[MAX_ID_LEN] = { 0 };
    SafeStrInput("请输入新的病房类型", newWardType, MAX_NAME_LEN);
    while (1) {
        SafeStrInput("请输入新的关联科室编号", newDeptId, MAX_ID_LEN);
        if (!CheckDepartmentExist(newDeptId)) {
            PRINT_WARN("数据校验未通过：关联科室不存在，请重新输入！");
            continue;
        }
        break;
    }
    int confirm = 0;
    SafeIntInput("警告：此操作将修改床位信息。请确认是否继续？输入1确认，输入0取消", &confirm, 0, 1);
    if (confirm == 0) {
        const char* clines[2] = { "【提示】", "操作已取消！" };
        print_content_box(clines, 2);
        WriteLog(LOG_LEVEL_INFO, "管理员", "修改床位", "用户取消操作");
        return 0;
    }
    SafeStrCopy(p->wardType, newWardType, MAX_NAME_LEN);
    SafeStrCopy(p->deptId, newDeptId, MAX_ID_LEN);
    // 记录日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "床位ID：%s，旧病房类型：%s→新病房类型：%s，旧关联科室：%s→新关联科室：%s",
        bedId, oldWardType, newWardType, oldDeptId, newDeptId);
    WriteLog(LOG_LEVEL_INFO, "管理员", "修改床位", logContent);
    PRINT_OK("操作成功：床位信息已更新！");
    return 1;
}

// 删除床位
int DeleteBed(void) {
    if (bedHead == NULL) {
        PRINT_ERR("床位模块未初始化！");
        return 0;
    }
    char bedId[MAX_ID_LEN] = { 0 };
    print_title_box("删除床位");
    PRINT_TIP("现有床位列表");
    PrintAllBedBrief();
    printf("\n");
    SafeStrInput("请输入要删除的床位编号", bedId, MAX_ID_LEN);
    BedNode* p = FindBedById(bedId);
    if (p == NULL) {
        PRINT_ERR("操作失败：床位编号不存在！");
        WriteLog(LOG_LEVEL_ERROR, "管理员", "删除床位", "失败，床位不存在");
        return 0;
    }
    if (p->status == BED_OCCUPIED) {
        PRINT_ERR("操作失败：该床位当前处于占用状态，禁止删除！请先释放床位后再试。");
        WriteLog(LOG_LEVEL_ERROR, "管理员", "删除床位", "失败，床位处于占用状态");
        return 0;
    }
    int confirm = 0;
    SafeIntInput("警告：此操作将永久删除床位信息，删除后无法恢复。请确认是否继续？输入1确认，输入0取消", &confirm, 0, 1);
    if (confirm == 0) {
        const char* lines[2] = { "【提示】", "操作已取消！" };
        print_content_box(lines, 2);
        WriteLog(LOG_LEVEL_INFO, "管理员", "删除床位", "用户取消操作");
        return 0;
    }
    // 修复：前驱节点查找增加空指针边界判断，避免死循环/空指针
    BedNode* pre = bedHead;
    while (pre->next != NULL && pre->next != p) {
        pre = pre->next;
    }
    if (pre->next != p) {
        PRINT_ERR("床位节点异常，删除失败！");
        return 0;
    }
    pre->next = p->next;
    free(p);
    // 记录日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "床位ID：%s", bedId);
    WriteLog(LOG_LEVEL_INFO, "管理员", "删除床位", logContent);
    PRINT_OK("操作成功：床位已删除！");
    return 1;
}

// 按病房类型查询床位
void QueryBedByWardType(void) {
    if (bedHead == NULL || bedHead->next == NULL) {
        const char* lines[2] = { "【提示】", "暂无床位数据！" };
        print_content_box(lines, 2);
        return;
    }
    char wardType[MAX_NAME_LEN] = { 0 };
    print_title_box("床位查询");
    SafeStrInput("请输入要查询的病房类型", wardType, MAX_NAME_LEN);
    // 先统计匹配数量
    int findCount = 0;
    BedNode* p = bedHead->next;
    while (p != NULL) {
        if (strcmp(p->wardType, wardType) == 0) {
            findCount++;
        }
        p = p->next;
    }
    // 为空直接提示
    if (findCount == 0) {
        char msg[80];
        snprintf(msg, sizeof(msg), "未找到病房类型为【%s】的床位", wardType);
        const char* lines[] = { msg };
        print_content_box(lines, 1);
        return;
    }
    // 不为空再打印表格
    int cols[] = { 12, 12, 12, 8, 14, 15 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "床位编号", "病房类型", "关联科室", "状态", "关联患者", "入院时间" };
    printf("\n");
    print_table_sep(cols, 6);
    print_table_row(cols, aligns, headers, 6);
    print_table_sep(cols, 6);
    p = bedHead->next;
    while (p != NULL) {
        if (strcmp(p->wardType, wardType) == 0) {
            const char* data[] = {
                p->bedId,
                p->wardType,
                p->deptId,
                p->status == BED_FREE ? "空闲" : "占用",
                p->status == BED_OCCUPIED ? p->patientId : "无",
                p->status == BED_OCCUPIED ? p->inTime : "无"
            };
            print_table_row(cols, aligns, data, 6);
        }
        p = p->next;
    }
    print_table_sep(cols, 6);
    char tip[80];
    snprintf(tip, sizeof(tip), "共找到%d条匹配的床位数据！", findCount);
    PRINT_TIP(tip);
    WriteLog(LOG_LEVEL_INFO, "用户", "按病房类型查询床位", "成功");
}

// 按科室查询空闲床位
void QueryFreeBedByDept(const char* deptId) {
    if (!CheckNullPtr(1, deptId) || bedHead == NULL) {
        return;
    }
    if (!CheckDepartmentExist(deptId)) {
        PRINT_ERR("数据校验未通过：科室不存在！");
        return;
    }
    // 先统计匹配数量
    int findCount = 0;
    BedNode* p = bedHead->next;
    while (p != NULL) {
        if (strcmp(p->deptId, deptId) == 0 && p->status == BED_FREE) {
            findCount++;
        }
        p = p->next;
    }
    // 为空直接提示
    if (findCount == 0) {
        char msg[80];
        snprintf(msg, sizeof(msg), "科室%s暂无空闲床位", deptId);
        const char* lines[] = { msg };
        print_content_box(lines, 1);
        return;
    }
    // 不为空再打印标题和表格
    char title[80];
    snprintf(title, sizeof(title), "科室%s空闲床位", deptId);
    print_title_box(title);
    int cols[] = { 38, 39 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "床位编号", "病房类型" };
    printf("\n");
    print_table_sep(cols, 2);
    print_table_row(cols, aligns, headers, 2);
    print_table_sep(cols, 2);
    p = bedHead->next;
    while (p != NULL) {
        if (strcmp(p->deptId, deptId) == 0 && p->status == BED_FREE) {
            const char* data[] = { p->bedId, p->wardType };
            print_table_row(cols, aligns, data, 2);
        }
        p = p->next;
    }
    print_table_sep(cols, 2);
    char tip[80];
    snprintf(tip, sizeof(tip), "共找到%d个符合条件的床位！", findCount);
    PRINT_TIP(tip);
}

// 打印所有床位信息
void PrintAllBed(void) {
    if (bedHead == NULL || bedHead->next == NULL) {
        const char* lines[2] = { "【提示】", "暂无床位数据！" };
        print_content_box(lines, 2);
        return;
    }
    print_title_box("所有床位信息");
    // 表格宽度与标题框对齐（80宽度）
    int cols[] = { 12, 12, 12, 8, 14, 15 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "床位编号", "病房类型", "关联科室", "状态", "关联患者", "入院时间" };
    printf("\n");
    print_table_sep(cols, 6);
    print_table_row(cols, aligns, headers, 6);
    print_table_sep(cols, 6);
    int findCount = 0;
    BedNode* p = bedHead->next;
    while (p != NULL) {
        const char* data[] = {
            p->bedId,
            p->wardType,
            p->deptId,
            p->status == BED_FREE ? "空闲" : "占用",
            p->status == BED_OCCUPIED ? p->patientId : "无",
            p->status == BED_OCCUPIED ? p->inTime : "无"
        };
        print_table_row(cols, aligns, data, 6);
        findCount++;
        p = p->next;
    }
    print_table_sep(cols, 6);
    char tip[80] = { 0 };
    snprintf(tip, 80, "共打印%d条床位信息！", findCount);
    PRINT_TIP(tip);
    WriteLog(LOG_LEVEL_INFO, "用户", "打印所有床位", "成功");
}

// 打印所有空闲床位信息
void PrintFreeBed(void) {
    if (bedHead == NULL || bedHead->next == NULL) {
        const char* lines[2] = { "【提示】", "暂无床位数据！" };
        print_content_box(lines, 2);
        return;
    }
    print_title_box("所有空闲床位");
    // 表格宽度与标题框对齐（80宽度）
    int cols[] = { 25, 25, 26 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "床位编号", "病房类型", "关联科室" };
    printf("\n");
    print_table_sep(cols, 3);
    print_table_row(cols, aligns, headers, 3);
    print_table_sep(cols, 3);
    int findCount = 0;
    BedNode* p = bedHead->next;
    while (p != NULL) {
        if (p->status == BED_FREE) {
            const char* data[] = { p->bedId, p->wardType, p->deptId };
            print_table_row(cols, aligns, data, 3);
            findCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 3);
    char tip[80] = { 0 };
    snprintf(tip, 80, "共打印%d条空闲床位信息！", findCount);
    PRINT_TIP(tip);
}

// 床位数据保存到文件
int SaveBedToFile(const char* fileName) {
    if (!CheckNullPtr(1, fileName)) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "保存床位数据", "失败，入参不合法");
        PRINT_ERR("文件名不能为空！");
        return 0;
    }
    if (bedHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "保存床位数据", "失败，床位链表未初始化");
        PRINT_ERR("床位模块还没初始化，请先重启系统！");
        return 0;
    }
    FILE* fp = fopen(fileName, "w");
    if (fp == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "保存床位数据", "失败，文件打开失败");
        PRINT_ERR("床位数据保存失败，无法打开文件！");
        return 0;
    }
    int saveCount = 0;
    BedNode* p = bedHead->next;
    while (p != NULL) {
        fprintf(fp, "%s,%s,%s,%d,%s,%s\n",
            p->bedId, p->wardType, p->deptId, p->status, p->patientId, p->inTime);
        saveCount++;
        p = p->next;
    }
    fclose(fp);
    WriteLog(LOG_LEVEL_INFO, "系统", "保存床位数据", "成功");
    char successMsg[100] = { 0 };
    snprintf(successMsg, 100, "已将%d条床位数据保存至文件%s！", saveCount, fileName);
    PRINT_OK(successMsg);
    return 1;
}

// 从文件加载床位信息
int LoadBedFromFile(const char* fileName) {
    if (!CheckNullPtr(1, fileName)) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "加载床位数据", "失败，入参不合法");
        PRINT_ERR("文件名不能为空！");
        return 0;
    }
    if (bedHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "加载床位数据", "失败，床位链表未初始化");
        PRINT_ERR("床位模块还没初始化，请先重启系统！");
        return 0;
    }
    FILE* fp = fopen(fileName, "r");
    if (fp == NULL) {
        PRINT_TIP("未找到床位历史数据文件，将使用空数据库！");
        WriteLog(LOG_LEVEL_INFO, "系统", "加载床位数据", "未找到历史文件");
        return 1;
    }
    char line[512] = { 0 };
    int loadCount = 0;
    int mem_error = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        char bedId[MAX_ID_LEN] = { 0 };
        char wardType[MAX_NAME_LEN] = { 0 };
        char deptId[MAX_ID_LEN] = { 0 };
        char patientId[MAX_ID_LEN] = { 0 };
        char inTime[MAX_DATA_LEN] = { 0 };
        int status = 0;
        // 修正：size_t转int强制转换，解决警告
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (strlen(line) == 0) continue;
        // 修正：strtok_s参数正确，解决警告
        char* rest = line;
        char* token = strtok_s(rest, ",", &rest);
        if (token) SafeStrCopy(bedId, token, MAX_ID_LEN);
        token = strtok_s(NULL, ",", &rest);
        if (token) SafeStrCopy(wardType, token, MAX_NAME_LEN);
        token = strtok_s(NULL, ",", &rest);
        if (token) SafeStrCopy(deptId, token, MAX_ID_LEN);
        token = strtok_s(NULL, ",", &rest);
        if (token) status = atoi(token);
        token = strtok_s(NULL, ",", &rest);
        if (token) SafeStrCopy(patientId, token, MAX_ID_LEN);
        token = strtok_s(NULL, ",", &rest);
        if (token) SafeStrCopy(inTime, token, MAX_DATA_LEN);
        // 跳过非法数据
        if (strlen(bedId) == 0 || CheckBedExist(bedId) || !CheckDepartmentExist(deptId) || (status != BED_FREE && status != BED_OCCUPIED)) {
            continue;
        }
        BedNode* newNode = (BedNode*)malloc(sizeof(BedNode));
        if (!newNode) {
            mem_error = 1;
            break;
        }
        memset(newNode, 0, sizeof(BedNode));
        SafeStrCopy(newNode->bedId, bedId, MAX_ID_LEN);
        SafeStrCopy(newNode->wardType, wardType, MAX_NAME_LEN);
        SafeStrCopy(newNode->deptId, deptId, MAX_ID_LEN);
        newNode->status = status;
        SafeStrCopy(newNode->patientId, patientId, MAX_ID_LEN);
        SafeStrCopy(newNode->inTime, inTime, MAX_DATA_LEN);
        newNode->next = bedHead->next;
        bedHead->next = newNode;
        loadCount++;
    }
    fclose(fp);
    // 加载结果提示
    if (mem_error) {
        PRINT_ERR("床位数据加载失败，系统内存不足！");
    }
    else if (loadCount > 0) {
        char msg[100] = { 0 };
        snprintf(msg, 100, "床位数据加载成功，共加载%d条有效数据！", loadCount);
        PRINT_OK(msg);
    }
    else {
        PRINT_TIP("床位数据文件为空，未加载任何数据！");
    }
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "成功加载%d条", loadCount);
    WriteLog(LOG_LEVEL_INFO, "系统", "加载床位数据", logContent);
    return 1;
}

// 钻取函数（使用通用表格函数）
static void DrillDownToWardDetail(const char* wardType, FILE* fp) {
    (void)fp; // 兼容文件输出入参
    char title[80] = { 0 };
    snprintf(title, 80, "【%s】病房详细床位清单", wardType);
    printf("\n");
    print_title_box(title);
    // 表格宽度与标题框对齐（80宽度）
    int cols[] = { 12, 12, 12, 8, 14, 15 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "床位编号", "病房类型", "关联科室", "状态", "关联患者", "入院时间" };
    printf("\n");
    print_table_sep(cols, 6);
    print_table_row(cols, aligns, headers, 6);
    print_table_sep(cols, 6);
    BedNode* p = bedHead->next;
    int count = 0, freeCount = 0, occupiedCount = 0;
    while (p != NULL) {
        if (strcmp(p->wardType, wardType) == 0) {
            const char* data[] = {
                p->bedId,
                p->wardType,
                p->deptId,
                p->status == BED_FREE ? "空闲" : "占用",
                p->status == BED_OCCUPIED ? p->patientId : "-",
                p->status == BED_OCCUPIED ? p->inTime : "-"
            };
            print_table_row(cols, aligns, data, 6);
            count++;
            if (p->status == BED_FREE) freeCount++;
            else occupiedCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 6);
    // 统计信息
    char stat[80] = { 0 };
    snprintf(stat, 80, "该类病房共 %d 张床位：空闲 %d 张，占用 %d 张", count, freeCount, occupiedCount);
    const char* slines[2] = { "统计信息", stat };
    print_content_box(slines, 2);
}

// 按病房类型生成报表，并且具有钻取功能
void GenerateBedUsageReport(FILE* fp) {
    if (bedHead == NULL || bedHead->next == NULL) {
        const char* lines[2] = { "【提示】", "暂无床位数据！" };
        print_content_box(lines, 2);
        if (fp != NULL) fprintf(fp, "暂无床位数据！\n");
        return;
    }
    typedef struct WardTypeNode {
        char name[MAX_NAME_LEN];
        struct WardTypeNode* next;
    } WardTypeNode;
    WardTypeNode* wardTypeHead = (WardTypeNode*)malloc(sizeof(WardTypeNode));
    if (wardTypeHead == NULL) {
        PRINT_ERR("内存不足，生成报表失败！");
        return;
    }
    wardTypeHead->next = NULL;
    // 遍历床位，提取所有不重复的病房类型
    BedNode* p = bedHead->next;
    while (p != NULL) {
        int exist = 0;
        WardTypeNode* w = wardTypeHead->next;
        while (w != NULL) {
            if (strcmp(w->name, p->wardType) == 0) { exist = 1; break; }
            w = w->next;
        }
        // 新增类别节点
        if (!exist) {
            WardTypeNode* newNode = (WardTypeNode*)malloc(sizeof(WardTypeNode));
            if (newNode == NULL) {
                PRINT_WARN("内存不足，部分类别加载失败！");
                continue;
            }
            SafeStrCopy(newNode->name, p->wardType, MAX_NAME_LEN);
            newNode->next = wardTypeHead->next;
            wardTypeHead->next = newNode;
        }
        p = p->next;
    }
    // 打印分类统计报表
    print_title_box("床位使用汇总报表");
    int cols[] = { 20, 8, 8, 8, 16, 16 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "病房类型", "总床位数", "空闲数", "占用数", "空闲率", "占用率" };
    printf("\n");
    print_table_sep(cols, 6);
    print_table_row(cols, aligns, headers, 6);
    print_table_sep(cols, 6);
    // 遍历类别，统计每个类别的数据
    WardTypeNode* w = wardTypeHead->next;
    while (w != NULL) {
        int num = 0, free = 0, occupied = 0;
        p = bedHead->next;
        while (p != NULL) {
            if (strcmp(p->wardType, w->name) == 0) {
                num++;
                if (p->status == BED_FREE) free++;
                else occupied++;
            }
            p = p->next;
        }
        char buf[4][20];
        snprintf(buf[0], sizeof(buf[0]), "%d", num);
        snprintf(buf[1], sizeof(buf[1]), "%d", free);
        snprintf(buf[2], sizeof(buf[2]), "%d", occupied);
        float freeRate = num > 0 ? (float)free / num * 100 : 0;
        float occRate = num > 0 ? (float)occupied / num * 100 : 0;
        snprintf(buf[3], sizeof(buf[3]), "%.2f%%", freeRate);
        char buf4[20];
        snprintf(buf4, sizeof(buf4), "%.2f%%", occRate);
        const char* data[] = { w->name, buf[0], buf[1], buf[2], buf[3], buf4 };
        print_table_row(cols, aligns, data, 6);
        w = w->next;
    }
    print_table_sep(cols, 6);    // 报表钻取功能
    int needDrill = 0;
    SafeIntInput("需要报表钻取？1=是，0=否", &needDrill, 0, 1);
    if (needDrill) {
        char target[MAX_NAME_LEN] = { 0 };
        SafeStrInput("请输入病房类型", target, MAX_NAME_LEN);
        int exist = 0;
        w = wardTypeHead->next;
        while (w != NULL) {
            if (strcmp(w->name, target) == 0) { exist = 1; break; }
            w = w->next;
        }
        if (exist) {
            DrillDownToWardDetail(target, fp);
        }
        else {
            PRINT_ERR("未找到该类别！");
        }
    }
    else {
        PRINT_TIP("已退出报表查看");
    }
    // 释放类别链表内存，避免泄漏
    w = wardTypeHead->next;
    while (w != NULL) {
        WardTypeNode* t = w;
        w = w->next;
        free(t);
    }
    free(wardTypeHead);
    WriteLog(LOG_LEVEL_INFO, "管理员", "生成床位使用报表", "成功");
}

// 全院床位使用率统计，含文件和控制台双输出
void StatHospitalBedUsageRate(FILE* fp) {
    if (bedHead == NULL || bedHead->next == NULL) {
        const char* lines[2] = { "【提示】", "暂无床位数据！" };
        print_content_box(lines, 2);
        if (fp != NULL) fprintf(fp, "暂无床位数据！\n");
        return;
    }
    int total = 0, occupied = 0;
    BedNode* p = bedHead->next;
    while (p != NULL) {
        total++;
        if (p->status == BED_OCCUPIED) occupied++;
        p = p->next;
    }
    float rate = total > 0 ? (float)occupied / total * 100 : 0;
    char stat1[80] = { 0 }, stat2[80] = { 0 }, stat3[80] = { 0 };
    snprintf(stat1, 80, "总床位数：%d", total);
    snprintf(stat2, 80, "占用床位数：%d", occupied);
    snprintf(stat3, 80, "全院床位使用率：%.2f%%", rate);
    const char* lines[4] = { "全院床位使用率统计", stat1, stat2, stat3 };
    print_content_box(lines, 4);
    // 文件同步输出
    if (fp != NULL) {
        fprintf(fp, "\n+");
        for (int i = 0; i < BOX_WIDTH - 2; i++) fprintf(fp, "-");
        fprintf(fp, "+\n");
        fprintf(fp, "| %-76s |\n", "全院床位使用率统计");
        fprintf(fp, "| %-76s |\n", stat1);
        fprintf(fp, "| %-76s |\n", stat2);
        fprintf(fp, "| %-76s |\n", stat3);
        fprintf(fp, "+");
        for (int i = 0; i < BOX_WIDTH - 2; i++) fprintf(fp, "-");
        fprintf(fp, "+\n");
    }
    WriteLog(LOG_LEVEL_INFO, "管理员", "统计全院床位使用率", "成功");
}

// 释放床位链表内存
void FreeBedList(void) {
    if (bedHead == NULL) return;
    BedNode* p = bedHead->next;
    while (p != NULL) {
        BedNode* t = p;
        p = p->next;
        free(t);
    }
    free(bedHead);
    bedHead = NULL;
    WriteLog(LOG_LEVEL_INFO, "系统", "释放床位链表内存", "成功");
}

// 打印所有床位简略信息（ID+类型+状态）
void PrintAllBedBrief(void) {
    if (bedHead == NULL || bedHead->next == NULL) {
        PRINT_TIP("暂无床位数据！");
        return;
    }
    print_title_box("床位列表（ID+类型+状态）");
    int cols[] = { 15, 25, 38 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "床位ID", "病房类型", "当前状态" };
    printf("\n");
    print_table_sep(cols, 3);
    print_table_row(cols, aligns, headers, 3);
    print_table_sep(cols, 3);
    BedNode* p = bedHead->next;
    while (p != NULL) {
        const char* data[] = { p->bedId, p->wardType, p->status == BED_FREE ? "空闲" : "已占用" };
        print_table_row(cols, aligns, data, 3);
        p = p->next;
    }
    print_table_sep(cols, 3);
}

// 交互式新增床位
int AddBed(void) {
    if (bedHead == NULL) {
        PRINT_ERR("床位模块未初始化！");
        return 0;
    }
    char bedId[MAX_ID_LEN] = { 0 };
    char wardType[MAX_NAME_LEN] = { 0 };
    char deptId[MAX_ID_LEN] = { 0 };
    print_title_box("新增床位");
    // 校验床位ID唯一性
    PRINT_TIP("现有床位列表");
    PrintAllBedBrief();
    printf("\n");
    while (1) {
        SafeStrInput("请输入床位唯一编号", bedId, MAX_ID_LEN);
        if (CheckBedExist(bedId)) {
            PRINT_WARN("床位编号已存在，请重新输入！");
            continue;
        }
        break;
    }
    SafeStrInput("请输入病房类型", wardType, MAX_NAME_LEN);
    // 校验关联科室是否存在
    PRINT_TIP("可选科室列表");
    PrintAllDepartmentBrief();
    printf("\n");
    while (1) {
        SafeStrInput("请输入关联科室编号", deptId, MAX_ID_LEN);
        if (!CheckDepartmentExist(deptId)) {
            PRINT_WARN("数据校验未通过：关联科室不存在，请重新输入！");
            continue;
        }
        break;
    }
    // 分配新节点内存
    BedNode* newNode = (BedNode*)malloc(sizeof(BedNode));
    if (newNode == NULL) {
        PRINT_ERR("内存分配失败，新增床位失败！");
        return 0;
    }
    // 初始化节点数据
    SafeStrCopy(newNode->bedId, bedId, MAX_ID_LEN);
    SafeStrCopy(newNode->wardType, wardType, MAX_NAME_LEN);
    SafeStrCopy(newNode->deptId, deptId, MAX_ID_LEN);
    newNode->status = BED_FREE;
    memset(newNode->patientId, 0, sizeof(newNode->patientId));
    memset(newNode->inTime, 0, sizeof(newNode->inTime));
    newNode->next = bedHead->next;
    bedHead->next = newNode;
    // 记录日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "床位ID：%s，病房类型：%s，关联科室：%s", bedId, wardType, deptId);
    WriteLog(LOG_LEVEL_INFO, "管理员", "新增床位", logContent);
    PRINT_OK("床位新增成功！");
    return 1;
}

// 【补充实现】交互式分配床位给患者（bed.h声明的缺失函数）
int AssignBedToPatientInteractive(void) {
    if (bedHead == NULL || bedHead->next == NULL) {
        PRINT_ERR("床位模块未初始化或暂无床位数据！");
        return 0;
    }
    char bedId[MAX_ID_LEN] = { 0 };
    char patientId[MAX_ID_LEN] = { 0 };
    char inTime[MAX_DATA_LEN] = { 0 };
    print_title_box("分配床位");
    PRINT_TIP("空闲床位列表");
    PrintFreeBed();
    printf("\n");
    // 输入床位ID
    while (1) {
        SafeStrInput("请输入要分配的床位编号", bedId, MAX_ID_LEN);
        if (!CheckBedExist(bedId)) {
            PRINT_WARN("床位编号不存在，请重新输入！");
            continue;
        }
        if (!CheckBedAvailable(bedId)) {
            PRINT_WARN("床位已被占用，请选择其他床位！");
            continue;
        }
        break;
    }
    // 输入患者ID
    PRINT_TIP("可选患者列表");
    PrintAllPatientBrief();
    printf("\n");
    while (1) {
        SafeStrInput("请输入患者编号", patientId, MAX_ID_LEN);
        if (!CheckPatientExist(patientId)) {
            PRINT_WARN("患者编号不存在，请重新输入！");
            continue;
        }
        break;
    }
    // 自动填充入院时间
    SafeStrCopy(inTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
    // 二次确认
    int confirm = 0;
    char info1[80], info2[80], info3[80];
    snprintf(info1, sizeof(info1), "床位编号：%s", bedId);
    snprintf(info2, sizeof(info2), "患者编号：%s", patientId);
    snprintf(info3, sizeof(info3), "入院时间：%s", inTime);
    const char* lines[4] = { "分配信息预览", info1, info2, info3 };
    print_content_box(lines, 4);
    SafeIntInput("确认分配床位？输入1确认，输入0取消", &confirm, 0, 1);
    if (confirm == 0) {
        PRINT_TIP("已取消分配操作！");
        return 0;
    }
    // 执行分配
    if (AssignBedToPatient(bedId, patientId, inTime)) {
        // 同步更新患者状态
        UpdatePatientType(patientId, PATIENT_INPATIENT, bedId, inTime);
        return 1;
    }
    return 0;
}

// 床位智能匹配：按科室匹配空闲床位，返回匹配的床位ID，无匹配返回NULL
// 【已从AddBed内部移出，修正嵌套函数语法错误】
char* AutoMatchFreeBedByDept(const char* deptId) {
    static char matchBedId[MAX_ID_LEN] = { 0 };
    memset(matchBedId, 0, MAX_ID_LEN);
    if (!CheckNullPtr(1, deptId) || bedHead == NULL || bedHead->next == NULL) {
        return NULL;
    }
    if (!CheckDepartmentExist(deptId)) {
        return NULL;
    }
    // 遍历找到该科室第一个空闲床位
    BedNode* p = bedHead->next;
    while (p != NULL) {
        if (strcmp(p->deptId, deptId) == 0 && p->status == BED_FREE) {
            SafeStrCopy(matchBedId, p->bedId, MAX_ID_LEN);
            return matchBedId;
        }
        p = p->next;
    }
    // 无匹配空闲床位
    return NULL;
}