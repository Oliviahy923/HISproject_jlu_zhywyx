// 先包含系统头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// 【必须优先包含common.h，统一全局打印规范】
#include "common.h"
// 再包含自定义业务头文件
#include "safe_utils.h"
#include "log.h"
#include "department.h"
#include "patient.h"
#include "bed.h"

// ==================== 内部私有结构体 ====================
typedef struct Department {
    char deptId[MAX_ID_LEN];      // 科室唯一编号
    char deptName[MAX_NAME_LEN];  // 科室名称
    char wardType[MAX_NAME_LEN];  // 关联病房类型
    struct Department* next;      // 链表下一个节点
} DepartmentNode, * DepartmentList;

// ==================== 内部私有静态变量 ====================
static DepartmentList deptHead = NULL;

// ==================== 内部私有辅助函数 ====================
static DepartmentNode* FindDeptById(const char* deptId) {
    if (deptHead == NULL || deptHead->next == NULL) {
        return NULL;
    }
    DepartmentNode* p = deptHead->next;
    while (p != NULL) {
        if (strcmp(p->deptId, deptId) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}
// ==================== 对外暴露的函数实现（与头文件声明完全一致）====================
// 初始化科室链表
void InitDepartmentList() {
    deptHead = (DepartmentList)malloc(sizeof(DepartmentNode));
    if (deptHead == NULL) {
        PRINT_ERR("内存分配失败，科室链表初始化失败！");
        return;
    }
    memset(deptHead, 0, sizeof(DepartmentNode));
    deptHead->next = NULL;
    PRINT_OK("科室链表初始化完成！");
}

// 按ID校验科室是否存在
int CheckDepartmentExist(const char* deptId) {
    return FindDeptById(deptId) != NULL ? 1 : 0;
}

// 新增科室（参数类型与头文件一致，此处为char*）
void AddDepartment(char* deptId, char* deptName, char* wardType) {
    if (deptHead == NULL) {
        PRINT_ERR("请先初始化科室链表！");
        return;
    }
    if (FindDeptById(deptId) != NULL) {
        PRINT_ERR("科室ID重复，新增失败！");
        return;
    }
    // ========== 重名校验核心代码 ==========
    DepartmentNode* p = deptHead->next;
    int sameNameCount = 0;
    while (p != NULL) {
        if (strcmp(p->deptName, deptName) == 0) {
            sameNameCount++;
        }
        p = p->next;
    }
    if (sameNameCount > 0) {
        char msg[100];
        snprintf(msg, sizeof(msg), "警告：系统中已存在%d个名称为【%s】的科室！", sameNameCount, deptName);
        PRINT_WARN(msg);
        int confirm = 0;
        SafeIntInput("是否确认继续新增该同名科室？输入1=确认，输入0=取消", &confirm, 0, 1);
        if (confirm == 0) {
            PRINT_TIP("已取消新增科室操作！");
            return;
        }
    }
    // ========== 重名校验结束 ==========
    DepartmentNode* newNode = (DepartmentNode*)malloc(sizeof(DepartmentNode));
    if (newNode == NULL) {
        PRINT_ERR("内存分配失败，新增科室失败！");
        return;
    }
    SafeStrCopy(newNode->deptId, deptId, MAX_ID_LEN);
    SafeStrCopy(newNode->deptName, deptName, MAX_NAME_LEN);
    SafeStrCopy(newNode->wardType, wardType, MAX_NAME_LEN);
    newNode->next = deptHead->next;
    deptHead->next = newNode;
    char msg[100];
    snprintf(msg, sizeof(msg), "科室【%s】新增成功！", deptId);
    PRINT_OK(msg);
}

// 打印所有科室信息
void PrintAllDepartment() {
    if (deptHead == NULL) {
        PRINT_ERR("请先初始化科室链表！");
        return;
    }
    DepartmentNode* p = deptHead->next;
    if (p == NULL) {
        const char* lines[] = { "【提示】", "暂无科室数据！" };
        print_content_box(lines, 2);
        return;
    }

    print_title_box("所有科室信息");

    int cols[] = { 20, 28, 28 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "科室编号", "科室名称", "关联病房类型" };

    printf("\n");
    print_table_sep(cols, 3);
    print_table_row(cols, aligns, headers, 3);
    print_table_sep(cols, 3);

    int count = 0;
    while (p != NULL) {
        const char* data[] = { p->deptId, p->deptName, p->wardType };
        print_table_row(cols, aligns, data, 3);
        count++;
        p = p->next;
    }
    print_table_sep(cols, 3);

    char tip[80];
    snprintf(tip, sizeof(tip), "共打印 %d 条科室信息！", count);
    PRINT_TIP(tip);
}

// 保存科室数据到文件
void SaveDepartmentToFile(char* fileName) {
    FILE* fp = fopen(fileName, "w");
    if (fp == NULL) {
        PRINT_ERR("文件打开失败，科室数据保存失败！");
        return;
    }
    DepartmentNode* p = deptHead->next;
    while (p != NULL) {
        fprintf(fp, "%s,%s,%s\n", p->deptId, p->deptName, p->wardType);
        p = p->next;
    }
    fclose(fp);

    char msg[100];
    snprintf(msg, sizeof(msg), "科室数据已保存至文件：%s", fileName);
    PRINT_OK(msg);
}

// 从文件加载科室数据（逻辑不变，修复size_t转int警告）
int  LoadDepartmentFromFile(char* fileName) {
    FILE* fp = fopen(fileName, "r");
    if (fp == NULL) {
        PRINT_ERR("科室数据加载失败：未找到数据文件！");
        return 0;
    }

    char line[256];
    int loadCount = 0;

    while (fgets(line, (int)sizeof(line), fp) != NULL) {
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';

        if (strlen(line) == 0) continue;

        char deptId[MAX_ID_LEN] = { 0 };
        char deptName[MAX_NAME_LEN] = { 0 };
        char wardType[MAX_NAME_LEN] = { 0 };

        char* token = strtok(line, ",");
        if (token != NULL) SafeStrCopy(deptId, token, MAX_ID_LEN);
        token = strtok(NULL, ",");
        if (token != NULL) SafeStrCopy(deptName, token, MAX_NAME_LEN);
        token = strtok(NULL, ",");
        if (token != NULL) SafeStrCopy(wardType, token, MAX_NAME_LEN);

        if (FindDeptById(deptId) == NULL) {
            DepartmentNode* newNode = (DepartmentNode*)malloc(sizeof(DepartmentNode));
            if (newNode != NULL) {
                SafeStrCopy(newNode->deptId, deptId, MAX_ID_LEN);
                SafeStrCopy(newNode->deptName, deptName, MAX_NAME_LEN);
                SafeStrCopy(newNode->wardType, wardType, MAX_NAME_LEN);
                newNode->next = deptHead->next;
                deptHead->next = newNode;
                loadCount++;
            }
        }
    }

    fclose(fp);

    if (loadCount > 0) {
        char msg[100];
        snprintf(msg, sizeof(msg), "科室数据加载成功，共加载%d条有效数据！", loadCount);
        PRINT_OK(msg);
        return loadCount;
    }
    else {
        PRINT_TIP("科室数据文件为空，未加载任何数据！");
        return 0;
    }
}

// 释放科室链表内存
void FreeDepartmentList() {
    if (deptHead == NULL) {
        PRINT_TIP("科室链表未初始化，无需释放！");
        return;
    }

    DepartmentNode* p = deptHead->next;
    DepartmentNode* q = NULL;

    while (p != NULL) {
        q = p->next;
        free(p);
        p = q;
    }

    free(deptHead);
    deptHead = NULL;

    PRINT_OK("科室链表内存释放成功！");
}

// 按名称模糊查询科室（修复链表遍历逻辑，跳过哨兵头结点）
void QueryDepartmentByName(const char* name)
{
    if (name == NULL || strlen(name) == 0) {
        PRINT_ERR("科室名称不能为空！");
        return;
    }

    DepartmentList p = deptHead->next;
    int found = 0;

    PRINT_TIP("========== 模糊查询科室结果 ==========");
    while (p != NULL) {
        if (strstr(p->deptName, name) != NULL) {
            printf("科室编号：%s\t科室名称：%s\t病房类型：%s\n",
                p->deptId, p->deptName, p->wardType);
            found = 1;
        }
        p = p->next;
    }

    if (!found) {
        PRINT_ERR("未找到包含该名称的科室！");
    }
    PRINT_TIP("====================================\n");
}

// 按科室完整名称获取科室ID，找不到返回NULL
char* GetDepartmentIdByName(const char* deptName)
{
    static char deptId[MAX_ID_LEN] = { 0 };
    memset(deptId, 0, MAX_ID_LEN);
    if (deptHead == NULL || deptName == NULL || strlen(deptName) == 0) {
        return NULL;
    }
    DepartmentNode* p = deptHead->next;
    while (p != NULL) {
        if (strcmp(p->deptName, deptName) == 0) {
            SafeStrCopy(deptId, p->deptId, MAX_ID_LEN);
            return deptId;
        }
        p = p->next;
    }
    return NULL;
}

// 按科室名称关键词模糊查询，统一格式打印，返回匹配数量，唯一匹配时输出科室ID（修复字段名错误）
int QueryDepartmentByNameFuzzy(const char* nameKeyword, char* outDeptId, int outIdLen)
{
    if (nameKeyword == NULL || outDeptId == NULL || outIdLen <= 0) {
        return 0;
    }
    if (deptHead == NULL || deptHead->next == NULL) {
        PRINT_TIP("暂无科室信息！");
        return 0;
    }

    // 初始化输出参数
    memset(outDeptId, 0, outIdLen);

    // 全局统一标题框
    print_title_box("匹配科室列表");

    // 复用你科室模块的表格规范
    int cols[] = { 12, 20, 20 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "科室编号", "科室名称", "病房类型" };

    print_table_sep(cols, 3);
    print_table_row(cols, aligns, headers, 3);
    print_table_sep(cols, 3);

    int matchCount = 0;
    char matchDeptId[MAX_ID_LEN] = { 0 };
    DepartmentNode* p = deptHead->next;

    while (p != NULL) {
        if (strstr(p->deptName, nameKeyword) != NULL) {
            // 【核心修复】把roomType改成和结构体一致的wardType
            const char* data[] = { p->deptId, p->deptName, p->wardType };
            print_table_row(cols, aligns, data, 3);

            // 记录唯一匹配的科室ID
            SafeStrCopy(matchDeptId, p->deptId, MAX_ID_LEN);
            matchCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 3);

    // 仅当匹配结果唯一时，返回科室ID
    if (matchCount == 1) {
        SafeStrCopy(outDeptId, matchDeptId, outIdLen);
    }

    // 统一格式提示
    char msg[80];
    snprintf(msg, sizeof(msg), "共匹配到 %d 个科室", matchCount);
    PRINT_TIP(msg);

    return matchCount;
}

// 打印所有科室简略信息（ID+名称）
void PrintAllDepartmentBrief(void) {
    if (deptHead == NULL || deptHead->next == NULL) {
        PRINT_TIP("暂无科室数据！");
        return;
    }
    print_title_box("科室列表（ID+名称）");
    int cols[] = { 20, 58 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "科室编号", "科室名称" };
    printf("\n");
    print_table_sep(cols, 2);
    print_table_row(cols, aligns, headers, 2);
    print_table_sep(cols, 2);
    DepartmentNode* p = deptHead->next;
    while (p != NULL) {
        const char* data[] = { p->deptId, p->deptName };
        print_table_row(cols, aligns, data, 2);
        p = p->next;
    }
    print_table_sep(cols, 2);
}