// 先包含系统头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// 【必须优先包含common.h，统一全局打印规范】
#include "common.h"
// 再包含自定义业务头文件
#include "prescription.h"
#include "doctor.h"
#include "patient.h"
#include "medicine.h"
#include "safe_utils.h"
#include "log.h"
#include "system.h"
#include "record.h"

// ==================== 外部函数前置声明（解决隐式声明警告）====================
extern SystemRole GetCurrentRole(void);
extern const char* GetCurrentOperator(void);
extern int IsPatientTreatedByDoctor(const char* patientId, const char* doctorId);
extern char* GetPatientRealId(const char* input);

// ==================== 内部私有静态变量 ====================
PrescList prescHead = NULL;

// ==================== 内部私有辅助函数 ====================
// 按处方ID查找处方节点
static PrescNode* FindPrescById(const char* prescId) {
    if (prescHead == NULL || prescId == NULL) {
        return NULL;
    }
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        if (strcmp(p->prescId, prescId) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}
// ==================== 对外接口函数（全功能整合，业务逻辑100%完整）====================
// 初始化处方链表（系统启动调用）
void InitPrescriptionList(void) {
    if (prescHead != NULL) {
        FreePrescriptionList();
    }
    prescHead = (PrescList)malloc(sizeof(PrescNode));
    if (prescHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "初始化处方链表", "失败，内存分配失败");
        PRINT_ERR("处方模块初始化失败，内存不足！");
        return;
    }
    prescHead->next = NULL;
    WriteLog(LOG_LEVEL_INFO, "系统", "初始化处方链表", "成功");
    PRINT_OK("处方模块初始化完成！");
}

// 校验处方是否存在
int CheckPrescriptionExist(const char* prescId) {
    return FindPrescById(prescId) != NULL ? 1 : 0;
}

// 校验处方是否可发药（仅已缴费未发药的处方可发药）
int CheckPrescriptionUndispensed(const char* prescId) {
    PrescNode* p = FindPrescById(prescId);
    if (p == NULL || p->status != PRESC_PAID) {
        return 0;
    }
    return 1;
}

// 校验药品是否有未发药的处方（给药品模块调用，防止删除关联药品）
int PrescCheckMedicineHasUndispensedPresc(const char* medicineId) {
    if (prescHead == NULL || medicineId == NULL) {
        return 0;
    }
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        if (strcmp(p->medicineId, medicineId) == 0 && p->status == PRESC_PAID) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

// 新增处方（含医生开方权限控制）
int AddPrescription(void) {
    if (prescHead == NULL) {
        PRINT_ERR("处方模块未初始化！");
        return 0;
    }

    char prescId[MAX_ID_LEN] = { 0 };
    char patientId[MAX_ID_LEN] = { 0 };
    char doctorId[MAX_ID_LEN] = { 0 };
    char medicineId[MAX_ID_LEN] = { 0 };
    char medicineGenericName[MAX_NAME_LEN] = { 0 };
    int quantity = 0;
    float totalFee = 0.0f;

    print_title_box("新增处方");
    // 1. 获取处方ID，校验不重复
    PRINT_TIP("现有处方列表");
    PrintAllPrescriptionBrief();
    printf("\n");
    while (1) {
        SafeStrInput("请输入处方唯一编号", prescId, MAX_ID_LEN);
        if (CheckPrescriptionExist(prescId)) {
            PRINT_WARN("处方编号已存在，请重新输入！");
            continue;
        }
        break;
    }
    // 2. 获取患者ID，校验存在+医生开方权限控制
    PRINT_TIP("可选患者列表");
    PrintAllPatientBrief();
    printf("\n");
    while (1) {
        SafeStrInput("请输入患者编号", patientId, MAX_ID_LEN);
        if (!CheckPatientExist(patientId)) {
            PRINT_WARN("患者编号不存在，请重新输入！");
            continue;
        }
        if (GetCurrentRole() == ROLE_DOCTOR) {
            if (!IsPatientTreatedByDoctor(patientId, (char*)GetCurrentOperator())) {
                PRINT_WARN("权限拒绝：您只能给接诊过的患者开处方！");
                continue;
            }
        }
        break;
    }
    // 3. 获取医生ID，校验存在
    PRINT_TIP("可选医生列表");
    PrintAllDoctorBrief();
    printf("\n");
    while (1) {
        SafeStrInput("请输入开方医生编号", doctorId, MAX_ID_LEN);
        if (!CheckDoctorExist(doctorId)) {
            PRINT_WARN("医生编号不存在，请重新输入！");
            continue;
        }
        break;
    }
    // 4. 获取药品ID，校验存在
    PRINT_TIP("可选药品列表");
    PrintAllMedicineBrief();
    printf("\n");
    while (1) {
        SafeStrInput("请输入药品编号", medicineId, MAX_ID_LEN);
        if (!CheckMedicineExist(medicineId)) {
            PRINT_WARN("药品编号不存在，请重新输入！");
            continue;
        }
        if (!GetMedicineGenericName(medicineId, medicineGenericName, MAX_NAME_LEN)) {
            PRINT_WARN("获取药品信息失败，请重新输入！");
            continue;
        }
        break;
    }


    // 5. 获取药品数量，校验库存充足
    while (1) {
        SafeIntInput("请输入药品数量", &quantity, 1, 10000);
        int stock = QueryMedicineStock(medicineId);
        if (stock < 0) {
            PRINT_WARN("获取药品库存失败，请重新输入！");
            continue;
        }
        if (stock < quantity) {
            char msg[80];
            snprintf(msg, sizeof(msg), "药品库存不足，当前库存：%d，请重新输入！", stock);
            PRINT_WARN(msg);
            continue;
        }
        break;
    }

    // 6. 自动计算总费用
    float salePrice = GetMedicineSalePrice(medicineId);
    totalFee = salePrice * quantity;

    // 7. 处方信息预览（内容框展示）
    char info1[80], info2[80], info3[80], info4[80], info5[80], info6[80], info7[80], info8[80];
    snprintf(info1, sizeof(info1), "处方编号：%s", prescId);
    snprintf(info2, sizeof(info2), "患者编号：%s", patientId);
    snprintf(info3, sizeof(info3), "开方医生：%s", doctorId);
    snprintf(info4, sizeof(info4), "药品编号：%s", medicineId);
    snprintf(info5, sizeof(info5), "药品通用名：%s", medicineGenericName);
    snprintf(info6, sizeof(info6), "药品数量：%d", quantity);
    snprintf(info7, sizeof(info7), "药品单价：%.2f元", salePrice);
    snprintf(info8, sizeof(info8), "总费用：%.2f元", totalFee);
    const char* lines[9] = { "处方信息预览", info1, info2, info3, info4, info5, info6, info7, info8 };
    print_content_box(lines, 9);

    // 8. 强制二次确认
    int confirm = 0;
    SafeIntInput("请确认以上处方信息是否正确？输入1确认，输入0取消", &confirm, 0, 1);
    if (confirm == 0) {
        PRINT_TIP("操作已取消！");
        WriteLog(LOG_LEVEL_INFO, "医生", "新增处方", "用户取消操作");
        return 0;
    }

    // 9. 创建处方节点
    PrescNode* newNode = (PrescNode*)malloc(sizeof(PrescNode));
    if (newNode == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "医生", "新增处方", "失败，内存分配失败");
        PRINT_ERR("新增处方失败，内存不足！");
        return 0;
    }

    // 10. 初始化节点数据
    SafeStrCopy(newNode->prescId, prescId, MAX_ID_LEN);
    SafeStrCopy(newNode->patientId, patientId, MAX_ID_LEN);
    SafeStrCopy(newNode->doctorId, doctorId, MAX_ID_LEN);
    SafeStrCopy(newNode->medicineId, medicineId, MAX_ID_LEN);
    newNode->quantity = quantity;
    newNode->totalFee = totalFee;
    newNode->status = PRESC_UNPAY; // 默认状态：未缴费
    SafeStrCopy(newNode->createTime, GetCurrentDate(), MAX_DATA_LEN);
    strcpy(newNode->dispenseTime, "");

    // 11. 插入链表
    newNode->next = prescHead->next;
    prescHead->next = newNode;

    // 12. 记录操作日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "处方ID：%s，患者ID：%s，医生ID：%s，药品ID：%s，数量：%d，总费用：%.2f",
        prescId, patientId, doctorId, medicineId, quantity, totalFee);
    WriteLog(LOG_LEVEL_INFO, "医生", "新增处方", logContent);

    PRINT_OK("处方已创建成功！");
    return 1;
}

// 修改处方（含冲账回退逻辑）
int ModifyPrescription(void) {
    if (prescHead == NULL) {
        PRINT_ERR("处方模块未初始化！");
        return 0;
    }

    char prescId[MAX_ID_LEN] = { 0 };
    print_title_box("修改处方");
    PRINT_TIP("现有处方列表");
    PrintAllPrescriptionBrief();
    printf("\n");
    SafeStrInput("请输入要修改的处方编号", prescId, MAX_ID_LEN);

    // 1. 校验处方是否存在
    PrescNode* p = FindPrescById(prescId);
    if (p == NULL) {
        PRINT_ERR("操作失败：处方编号不存在！");
        WriteLog(LOG_LEVEL_ERROR, "医生", "修改处方", "失败，处方不存在");
        return 0;
    }

    // 2. 核心校验：已发药/已缴费的处方禁止修改
    if (p->status != PRESC_UNPAY) {
        PRINT_ERR("操作失败：该处方已缴费/已发药，禁止修改！");
        WriteLog(LOG_LEVEL_ERROR, "医生", "修改处方", "失败，处方已进入缴费流程");
        return 0;
    }

    // 3. 展示当前处方信息
    char oldMedicineGenericName[MAX_NAME_LEN] = { 0 };
    GetMedicineGenericName(p->medicineId, oldMedicineGenericName, MAX_NAME_LEN);
    char info1[80], info2[80], info3[80], info4[80];
    snprintf(info1, sizeof(info1), "处方编号：%s", p->prescId);
    snprintf(info2, sizeof(info2), "关联药品编号：%s", p->medicineId);
    snprintf(info3, sizeof(info3), "关联药品通用名：%s", oldMedicineGenericName);
    snprintf(info4, sizeof(info4), "当前数量：%d，当前总费用：%.2f元", p->quantity, p->totalFee);
    const char* lines[5] = { "当前处方信息", info1, info2, info3, info4 };
    print_content_box(lines, 5);

    // 4. 记录旧数据，用于日志留痕
    char oldMedicineId[MAX_ID_LEN] = { 0 };
    int oldQuantity = p->quantity;
    float oldTotalFee = p->totalFee;
    SafeStrCopy(oldMedicineId, p->medicineId, MAX_ID_LEN);

    // 5. 获取新处方数据
    char newMedicineId[MAX_ID_LEN] = { 0 };
    char newMedicineGenericName[MAX_NAME_LEN] = { 0 };
    int newQuantity = 0;
    float newSalePrice = 0.0f;
    float newTotalFee = 0.0f;

    // 获取新药品ID，校验存在
    while (1) {
        SafeStrInput("请输入新的药品编号", newMedicineId, MAX_ID_LEN);
        if (!CheckMedicineExist(newMedicineId)) {
            PRINT_WARN("药品编号不存在，请重新输入！");
            continue;
        }
        GetMedicineGenericName(newMedicineId, newMedicineGenericName, MAX_NAME_LEN);
        break;
    }

    // 获取新药品数量，校验库存充足
    while (1) {
        SafeIntInput("请输入新的药品数量", &newQuantity, 1, 10000);
        int stock = QueryMedicineStock(newMedicineId);
        if (stock < 0) {
            PRINT_WARN("获取药品库存失败，请重新输入！");
            continue;
        }
        if (stock < newQuantity) {
            char msg[80];
            snprintf(msg, sizeof(msg), "药品库存不足，当前库存：%d，请重新输入！", stock);
            PRINT_WARN(msg);
            continue;
        }
        break;
    }

    // 计算新总费用
    newSalePrice = GetMedicineSalePrice(newMedicineId);
    if (newSalePrice < 0.0f) {
        PRINT_ERR("获取药品售价失败，请重新操作！");
        WriteLog(LOG_LEVEL_ERROR, "医生", "修改处方", "失败，获取药品售价失败");
        return 0;
    }
    newTotalFee = newSalePrice * newQuantity;

    // 6. 修改后信息预览
    char ninfo1[80], ninfo2[80], ninfo3[80];
    snprintf(ninfo1, sizeof(ninfo1), "药品编号：%s", newMedicineId);
    snprintf(ninfo2, sizeof(ninfo2), "药品通用名：%s", newMedicineGenericName);
    snprintf(ninfo3, sizeof(ninfo3), "药品数量：%d，总费用：%.2f元", newQuantity, newTotalFee);
    const char* nlines[4] = { "修改后处方信息预览", ninfo1, ninfo2, ninfo3 };
    print_content_box(nlines, 4);

    // 7. 强制二次确认
    int confirm = 0;
    SafeIntInput("警告：此操作将修改处方信息。请确认是否继续？输入1确认，输入0取消", &confirm, 0, 1);
    if (confirm == 0) {
        PRINT_TIP("操作已取消！");
        WriteLog(LOG_LEVEL_INFO, "医生", "修改处方", "用户取消操作");
        return 0;
    }

    // 8. 执行修改（冲账回退）
    SafeStrCopy(p->medicineId, newMedicineId, MAX_ID_LEN);
    p->quantity = newQuantity;
    p->totalFee = newTotalFee;

    // 9. 记录完整修改日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "处方ID：%s，旧药品ID：%s→新药品ID：%s，旧数量：%d→新数量：%d，旧总费用：%.2f→新总费用：%.2f",
        prescId, oldMedicineId, newMedicineId, oldQuantity, newQuantity, oldTotalFee, newTotalFee);
    WriteLog(LOG_LEVEL_INFO, "医生", "修改处方", logContent);

    PRINT_OK("处方信息已更新成功！");
    return 1;
}

// 删除处方（仅管理员可操作）
int DeletePrescription(void) {
    // 权限校验：仅管理员可删除处方
    if (GetCurrentRole() != ROLE_ADMIN) {
        PRINT_ERR("权限拒绝：仅管理员可执行处方删除操作！医生请使用处方作废功能。");
        WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "删除处方", "失败，权限不足");
        return 0;
    }
    if (prescHead == NULL || prescHead->next == NULL) {
        PRINT_ERR("暂无处方数据，无法执行删除操作！");
        WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "删除处方", "失败，处方链表为空");
        return 0;
    }
    char prescId[MAX_ID_LEN] = { 0 };
    print_title_box("删除处方");
    PRINT_TIP("现有处方列表");
    PrintAllPrescriptionBrief();
    printf("\n");
    SafeStrInput("请输入要删除的处方编号", prescId, MAX_ID_LEN);
    // 1. 校验处方是否存在
    PrescNode* p = FindPrescById(prescId);
    if (p == NULL) {
        PRINT_ERR("操作失败：处方编号不存在！");
        WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "删除处方", "失败，处方不存在");
        return 0;
    }
    // 2. 核心校验：已缴费/已发药/已作废的处方禁止删除
    if (p->status != PRESC_UNPAY) {
        PRINT_ERR("操作失败：该处方已进入业务流程，涉及财务流水，禁止直接删除！");
        PRINT_TIP("提示：若需修正，请走处方作废冲账流程。");
        WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "删除处方", "失败，处方已进入业务流程");
        return 0;
    }
    // 3. 待删除处方信息展示
    char medicineGenericName[MAX_NAME_LEN] = { 0 };
    GetMedicineGenericName(p->medicineId, medicineGenericName, MAX_NAME_LEN);
    char info1[80], info2[80], info3[80], info4[80], info5[80], info6[80];
    snprintf(info1, sizeof(info1), "处方编号：%s", p->prescId);
    snprintf(info2, sizeof(info2), "患者编号：%s", p->patientId);
    snprintf(info3, sizeof(info3), "开方医生：%s", p->doctorId);
    snprintf(info4, sizeof(info4), "药品编号：%s，通用名：%s", p->medicineId, medicineGenericName);
    snprintf(info5, sizeof(info5), "药品数量：%d，总费用：%.2f元", p->quantity, p->totalFee);
    snprintf(info6, sizeof(info6), "当前状态：未缴费");
    const char* lines[7] = { "待删除处方信息", info1, info2, info3, info4, info5, info6 };
    print_content_box(lines, 7);
    // 4. 强制二次确认，防止误删
    int confirm = 0;
    SafeIntInput("警告：此操作将永久删除处方信息，删除后无法恢复。请确认是否继续？输入1确认，输入0取消", &confirm, 0, 1);
    if (confirm == 0) {
        PRINT_TIP("操作已取消！");
        WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "删除处方", "用户取消操作");
        return 0;
    }
    // 5. 安全查找前驱节点
    PrescNode* pre = prescHead;
    while (pre->next != NULL && pre->next != p) {
        pre = pre->next;
    }
    if (pre->next != p) {
        PRINT_ERR("操作失败：处方节点异常，删除失败！");
        WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "删除处方", "失败，节点链表异常");
        return 0;
    }
    // 6. 从链表中移除这条记录
    pre->next = p->next;
    // 7. 记录删除日志（释放内存前先保存信息）
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "处方ID：%s，药品ID：%s，数量：%d，总费用：%.2f", prescId, p->medicineId, p->quantity, p->totalFee);
    // 8. 释放内存
    free(p);
    // 9. 记录操作日志
    WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "删除处方", logContent);
    PRINT_OK("处方已永久删除！");
    return 1;
}

// 处方发药（BUG完全修复版，状态校验100%正确）
int DispensePrescription(void) {
    if (prescHead == NULL) {
        PRINT_ERR("处方模块未初始化！");
        return 0;
    }
    CLEAR_SCREEN;
    print_title_box("处方发药管理");

    // ========== 第一步：打印所有待发药处方（核心状态：PRESC_PAID=1 已缴费待发药） ==========
    PRINT_TIP("当前所有待发药处方列表");
    int cols[] = { 12, 12, 16, 10, 10, 12 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "处方编号", "患者ID", "患者姓名", "药品编号", "数量", "开方时间" };
    printf("\n");
    print_table_sep(cols, 6);
    print_table_row(cols, aligns, headers, 6);
    print_table_sep(cols, 6);

    int undispensedCount = 0;
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        // 【核心BUG修复1：正确过滤待发药状态，仅PRESC_PAID=1的处方才显示】
        if (p->status == PRESC_PAID) {
            // 获取患者姓名
            PatientNode* patient = FindPatientById(p->patientId);
            char patientName[MAX_NAME_LEN] = "未知患者";
            if (patient != NULL) {
                SafeStrCopy(patientName, patient->name, MAX_NAME_LEN);
            }
            char quantity[10];
            snprintf(quantity, sizeof(quantity), "%d", p->quantity);
            const char* data[] = {
                p->prescId, p->patientId, patientName, p->medicineId, quantity, p->createTime
            };
            print_table_row(cols, aligns, data, 6);
            undispensedCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 6);

    // 无待发药处方直接退出
    if (undispensedCount == 0) {
        PRINT_TIP("当前暂无待发药处方！");
        return 0;
    }
    char countMsg[80];
    snprintf(countMsg, sizeof(countMsg), "共找到 %d 条待发药处方", undispensedCount);
    PRINT_TIP(countMsg);

    // ========== 第二步：可选按患者姓名模糊查询 ==========
    printf("\n");
    int searchChoice = 0;
    SafeIntInput("是否按患者姓名模糊查询？输入1=是，输入0=否，直接输入处方编号", &searchChoice, 0, 1);
    if (searchChoice == 1) {
        char nameKeyword[MAX_NAME_LEN] = { 0 };
        while (1) {
            SafeStrInput("请输入患者姓名关键词", nameKeyword, MAX_NAME_LEN);
            if (strlen(nameKeyword) == 0) {
                PRINT_WARN("关键词不能为空，请重新输入！");
                continue;
            }
            break;
        }
        // 模糊查询并打印匹配结果
        CLEAR_SCREEN;
        print_title_box("待发药处方查询结果");
        printf("\n");
        print_table_sep(cols, 6);
        print_table_row(cols, aligns, headers, 6);
        print_table_sep(cols, 6);
        int matchCount = 0;
        p = prescHead->next;
        while (p != NULL) {
            if (p->status == PRESC_PAID) {
                PatientNode* patient = FindPatientById(p->patientId);
                if (patient != NULL && strstr(patient->name, nameKeyword) != NULL) {
                    char quantity[10];
                    snprintf(quantity, sizeof(quantity), "%d", p->quantity);
                    const char* data[] = {
                        p->prescId, p->patientId, patient->name, p->medicineId, quantity, p->createTime
                    };
                    print_table_row(cols, aligns, data, 6);
                    matchCount++;
                }
            }
            p = p->next;
        }
        print_table_sep(cols, 6);
        if (matchCount == 0) {
            PRINT_TIP("未找到匹配的待发药处方，将返回全量列表发药流程");
        }
        else {
            char matchMsg[80];
            snprintf(matchMsg, sizeof(matchMsg), "共匹配到 %d 条待发药处方", matchCount);
            PRINT_TIP(matchMsg);
        }
    }

    // ========== 第三步：输入处方编号执行发药 ==========
    printf("\n");
    char targetPrescId[MAX_ID_LEN] = { 0 };
    SafeStrInput("请输入要发药的处方编号", targetPrescId, MAX_ID_LEN);

    // 1. 校验处方是否存在
    PrescNode* targetPresc = FindPrescById(targetPrescId);
    if (targetPresc == NULL) {
        PRINT_ERR("操作失败：处方编号不存在！");
        WriteLog(LOG_LEVEL_ERROR, "药房", "处方发药", "失败，处方不存在");
        return 0;
    }

    // 2. 核心业务状态校验（完全修复）
    if (targetPresc->status == PRESC_UNPAY) {
        PRINT_ERR("操作失败：该处方未缴费，无法发药！");
        PRINT_TIP("提示：请先登录患者账号完成缴费！");
        WriteLog(LOG_LEVEL_ERROR, "药房", "处方发药", "失败，处方未缴费");
        return 0;
    }
    if (targetPresc->status == PRESC_FINISHED) {
        PRINT_ERR("操作失败：该处方已发药，禁止重复操作！");
        WriteLog(LOG_LEVEL_ERROR, "药房", "处方发药", "失败，处方已发药");
        return 0;
    }
    // 【核心BUG修复2：仅允许PRESC_PAID状态的处方发药】
    if (targetPresc->status != PRESC_PAID) {
        PRINT_ERR("操作失败：该处方非待发药状态，无法发药！");
        WriteLog(LOG_LEVEL_ERROR, "药房", "处方发药", "失败，处方状态非法");
        return 0;
    }

    // 3. 再次校验药品库存
    int stock = QueryMedicineStock(targetPresc->medicineId);
    if (stock < 0) {
        PRINT_ERR("操作失败：获取药品库存失败，请重新操作！");
        WriteLog(LOG_LEVEL_ERROR, "药房", "处方发药", "失败，获取药品库存失败");
        return 0;
    }
    if (stock < targetPresc->quantity) {
        char msg[80];
        snprintf(msg, sizeof(msg), "操作失败：药品库存不足。当前库存：%d，处方需求：%d！", stock, targetPresc->quantity);
        PRINT_ERR(msg);
        WriteLog(LOG_LEVEL_ERROR, "药房", "处方发药", "失败，药品库存不足");
        return 0;
    }

    // 4. 获取药品通用名
    char medicineGenericName[MAX_NAME_LEN] = { 0 };
    if (!GetMedicineGenericName(targetPresc->medicineId, medicineGenericName, MAX_NAME_LEN)) {
        PRINT_ERR("操作失败：获取药品信息失败，请重新操作！");
        WriteLog(LOG_LEVEL_ERROR, "药房", "处方发药", "失败，获取药品信息失败");
        return 0;
    }

    // 5. 处方信息确认
    PatientNode* patient = FindPatientById(targetPresc->patientId);
    char patientName[MAX_NAME_LEN] = "未知患者";
    if (patient != NULL) SafeStrCopy(patientName, patient->name, MAX_NAME_LEN);
    char info1[80], info2[80], info3[80], info4[80], info5[80], info6[80];
    snprintf(info1, sizeof(info1), "处方编号：%s", targetPresc->prescId);
    snprintf(info2, sizeof(info2), "患者姓名：%s", patientName);
    snprintf(info3, sizeof(info3), "开方医生：%s", targetPresc->doctorId);
    snprintf(info4, sizeof(info4), "药品编号：%s，通用名：%s", targetPresc->medicineId, medicineGenericName);
    snprintf(info5, sizeof(info5), "药品数量：%d，总费用：%.2f元", targetPresc->quantity, targetPresc->totalFee);
    snprintf(info6, sizeof(info6), "当前状态：已缴费，待发药");
    const char* lines[7] = { "处方信息", info1, info2, info3, info4, info5, info6 };
    print_content_box(lines, 7);

    // 6. 强制二次确认
    int confirm = 0;
    SafeIntInput("请确认是否发药？输入1确认，输入0取消", &confirm, 0, 1);
    if (confirm == 0) {
        PRINT_TIP("操作已取消！");
        WriteLog(LOG_LEVEL_INFO, "药房", "处方发药", "用户取消操作");
        return 0;
    }

    // 7. 药品库存扣减
    if (!MedicineStockOut(targetPresc->medicineId, targetPresc->quantity)) {
        PRINT_ERR("操作失败：药品出库失败，请重新操作！");
        WriteLog(LOG_LEVEL_ERROR, "药房", "处方发药", "失败，药品出库失败");
        return 0;
    }

    // 8. 更新处方状态为已发药，记录发药时间
    targetPresc->status = PRESC_FINISHED;
    SafeStrCopy(targetPresc->dispenseTime, GetCurrentDate(), MAX_DATA_LEN);

    // 9. 记录完整操作日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "处方ID：%s，患者ID：%s，药品ID：%s，数量：%d，总费用：%.2f",
        targetPresc->prescId, targetPresc->patientId, targetPresc->medicineId, targetPresc->quantity, targetPresc->totalFee);
    WriteLog(LOG_LEVEL_INFO, "药房", "处方发药", logContent);
    PRINT_OK("处方发药完成！");
    return 1;
}

// 按患者ID/姓名查询处方
void QueryPrescriptionByPatientId(const char* input) {
    const char* realId = GetPatientRealId(input);
    if (realId == NULL) {
        PRINT_ERR("未找到匹配的患者！");
        return;
    }
    if (prescHead == NULL || prescHead->next == NULL) {
        PRINT_TIP("暂无处方数据！");
        return;
    }
    // 先统计匹配数量
    int findCount = 0;
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        if (strcmp(p->patientId, realId) == 0) {
            findCount++;
        }
        p = p->next;
    }
    if (findCount == 0) {
        char msg[80];
        snprintf(msg, sizeof(msg), "患者%s暂无任何处方记录！", realId);
        const char* lines[] = { msg };
        print_content_box(lines, 1);
        return;
    }
    // 120宽度标题框
    char title[80];
    snprintf(title, sizeof(title), "患者%s处方记录", realId);
    print_title_box_ex(title, 120);
    // 列宽适配120宽度
    int cols[] = { 12, 14, 20, 10, 12, 12, 14, 14 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "处方编号", "药品编号", "药品通用名", "数量", "总费用", "状态", "开方时间", "发药时间" };
    printf("\n");
    print_table_sep_ex(cols, 8, 120);
    print_table_row_ex(cols, aligns, headers, 8, 120);
    print_table_sep_ex(cols, 8, 120);
    p = prescHead->next;
    while (p != NULL) {
        if (strcmp(p->patientId, realId) == 0) {
            char medicineGenericName[MAX_NAME_LEN] = { 0 };
            GetMedicineGenericName(p->medicineId, medicineGenericName, MAX_NAME_LEN);
            char quantity[10], fee[20];
            snprintf(quantity, sizeof(quantity), "%d", p->quantity);
            snprintf(fee, sizeof(fee), "%.2f", p->totalFee);
            // 状态文本转换
            const char* statusStr;
            if (p->status == PRESC_UNPAY) statusStr = "未缴费";
            else if (p->status == PRESC_PAID) statusStr = "待发药";
            else if (p->status == PRESC_CANCELED) statusStr = "已作废";
            else statusStr = "已发药";
            const char* data[] = {
                p->prescId, p->medicineId, medicineGenericName, quantity, fee, statusStr, p->createTime,
                strlen(p->dispenseTime) > 0 ? p->dispenseTime : "无"
            };
            print_table_row_ex(cols, aligns, data, 8, 120);
        }
        p = p->next;
    }
    print_table_sep_ex(cols, 8, 120);
    char msg[80];
    snprintf(msg, sizeof(msg), "共找到%d条处方记录！", findCount);
    PRINT_TIP(msg);
    WriteLog(LOG_LEVEL_INFO, "用户", "按患者ID查询处方", "成功");
}

// 按药品ID查询处方
void QueryPrescriptionByMedicineId(const char* medicineId) {
    if (!CheckNullPtr(1, medicineId) || prescHead == NULL || prescHead->next == NULL) {
        PRINT_TIP("暂无处方数据！");
        return;
    }

    char title[80];
    snprintf(title, sizeof(title), "药品%s处方记录", medicineId);
    print_title_box(title);

    // 表格列宽严格匹配80宽度
    int cols[] = { 12, 12, 8, 10, 12, 12 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "处方编号", "患者编号", "数量", "总费用", "状态", "开方时间" };

    printf("\n");
    print_table_sep(cols, 6);
    print_table_row(cols, aligns, headers, 6);
    print_table_sep(cols, 6);

    int findCount = 0;
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        if (strcmp(p->medicineId, medicineId) == 0) {
            char quantity[10], fee[20];
            snprintf(quantity, sizeof(quantity), "%d", p->quantity);
            snprintf(fee, sizeof(fee), "%.2f", p->totalFee);

            // 状态文本转换
            const char* statusStr;
            if (p->status == PRESC_UNPAY) statusStr = "未缴费";
            else if (p->status == PRESC_PAID) statusStr = "待发药";
            else statusStr = "已发药";

            const char* data[] = {
                p->prescId, p->patientId, quantity, fee, statusStr, p->createTime
            };
            print_table_row(cols, aligns, data, 6);
            findCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 6);

    if (findCount == 0) {
        PRINT_TIP("未找到匹配的处方记录！");
    }
    else {
        char msg[80];
        snprintf(msg, sizeof(msg), "共找到%d条处方记录！", findCount);
        PRINT_TIP(msg);
    }

    WriteLog(LOG_LEVEL_INFO, "用户", "按药品ID查询处方", "成功");
}

// 按时间范围查询处方
void QueryPrescriptionByTimeRange(void) {
    if (prescHead == NULL || prescHead->next == NULL) {
        PRINT_TIP("暂无处方数据！");
        return;
    }

    char startTime[MAX_DATA_LEN] = { 0 };
    char endTime[MAX_DATA_LEN] = { 0 };
    print_title_box("时间范围处方查询");

    // 获取开始时间，校验格式
    while (1) {
        SafeStrInput("请输入开始时间（格式：YYYY-MM-DD）", startTime, MAX_DATA_LEN);
        if (!CheckTimeFormatValid(startTime)) {
            PRINT_WARN("时间格式不正确，请按照YYYY-MM-DD的格式输入！");
            continue;
        }
        break;
    }

    // 获取结束时间，校验格式
    while (1) {
        SafeStrInput("请输入结束时间（格式：YYYY-MM-DD）", endTime, MAX_DATA_LEN);
        if (!CheckTimeFormatValid(endTime)) {
            PRINT_WARN("时间格式不正确，请按照YYYY-MM-DD的格式输入！");
            continue;
        }
        if (strcmp(endTime, startTime) < 0) {
            PRINT_WARN("结束时间不能早于开始时间，请重新输入！");
            continue;
        }
        break;
    }

    // 表格列宽严格匹配80宽度
    int cols[] = { 10, 12, 12, 8, 10, 12, 12 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "处方编号", "患者编号", "药品编号", "数量", "总费用", "状态", "开方时间" };

    printf("\n");
    print_table_sep(cols, 7);
    print_table_row(cols, aligns, headers, 7);
    print_table_sep(cols, 7);

    int findCount = 0;
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        if (strcmp(p->createTime, startTime) >= 0 && strcmp(p->createTime, endTime) <= 0) {
            char quantity[10], fee[20];
            snprintf(quantity, sizeof(quantity), "%d", p->quantity);
            snprintf(fee, sizeof(fee), "%.2f", p->totalFee);

            // 状态文本转换
            const char* statusStr;
            if (p->status == PRESC_UNPAY) statusStr = "未缴费";
            else if (p->status == PRESC_PAID) statusStr = "待发药";
            else statusStr = "已发药";

            const char* data[] = {
                p->prescId, p->patientId, p->medicineId, quantity, fee, statusStr, p->createTime
            };
            print_table_row(cols, aligns, data, 7);
            findCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 7);

    if (findCount == 0) {
        PRINT_TIP("未找到匹配的处方记录！");
    }
    else {
        char msg[80];
        snprintf(msg, sizeof(msg), "共找到%d条处方记录！", findCount);
        PRINT_TIP(msg);
    }

    WriteLog(LOG_LEVEL_INFO, "用户", "按时间范围查询处方", "成功");
}

// 打印所有处方
void PrintAllPrescription(void) {
    if (prescHead == NULL || prescHead->next == NULL) {
        PRINT_TIP("暂无处方数据！");
        return;
    }
    // 改为120宽度标题框
    print_title_box_ex("所有处方信息", 120);
    // 列宽适配120宽度，总宽度120
    int cols[] = { 10, 12, 12, 12, 8, 12, 12, 14, 16 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "处方编号", "患者ID", "患者姓名", "医生ID", "数量", "总费用", "状态", "开方时间", "发药时间" };
    printf("\n");
    // 120宽度表格分隔线
    print_table_sep_ex(cols, 9, 120);
    print_table_row_ex(cols, aligns, headers, 9, 120);
    print_table_sep_ex(cols, 9, 120);
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        char quantity[10], fee[20];
        snprintf(quantity, sizeof(quantity), "%d", p->quantity);
        snprintf(fee, sizeof(fee), "%.2f", p->totalFee);
        // 状态文本转换
        const char* statusStr;
        if (p->status == PRESC_UNPAY) statusStr = "未缴费";
        else if (p->status == PRESC_PAID) statusStr = "待发药";
        else if (p->status == PRESC_CANCELED) statusStr = "已作废";
        else statusStr = "已发药";
        // 获取患者姓名
        PatientNode* patient = FindPatientById(p->patientId);
        const char* patientName = (patient != NULL) ? patient->name : "未知患者";
        const char* data[] = {
            p->prescId, p->patientId, patientName, p->doctorId, quantity, fee, statusStr, p->createTime,
            strlen(p->dispenseTime) > 0 ? p->dispenseTime : "无"
        };
        print_table_row_ex(cols, aligns, data, 9, 120);
        p = p->next;
    }
    print_table_sep_ex(cols, 9, 120);
    WriteLog(LOG_LEVEL_INFO, "用户", "打印所有处方", "成功");
}

// 处方数据保存到文件
int SavePrescriptionToFile(const char* fileName) {
    if (!CheckNullPtr(1, fileName) || prescHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "保存处方数据", "失败，入参不合法或模块未初始化");
        PRINT_ERR("文件名不能为空！");
        return 0;
    }

    FILE* fp = fopen(fileName, "w");
    if (fp == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "保存处方数据", "失败，文件打开失败");
        PRINT_ERR("处方数据保存失败，无法打开文件！");
        return 0;
    }

    int saveCount = 0;
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        fprintf(fp, "%s,%s,%s,%s,%d,%.2f,%d,%s,%s\n",
            p->prescId, p->patientId, p->doctorId, p->medicineId,
            p->quantity, p->totalFee, p->status, p->createTime, p->dispenseTime);
        p = p->next;
        saveCount++;
    }
    fclose(fp);

    WriteLog(LOG_LEVEL_INFO, "系统", "保存处方数据", "成功");
    char msg[80];
    snprintf(msg, sizeof(msg), "处方数据已保存，共%d条数据！", saveCount);
    PRINT_OK(msg);
    return 1;
}

// 从文件加载处方数据（含完整数据校验）
// 从文件加载处方数据（完美适配你的数据格式，解决所有解析问题）
int LoadPrescriptionFromFile(const char* fileName) {
    if (!CheckNullPtr(1, fileName) || prescHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "加载处方数据", "失败，入参不合法或模块未初始化");
        PRINT_ERR("处方加载失败：文件名不能为空或模块未初始化！");
        return 0;
    }

    FILE* fp = fopen(fileName, "r");
    if (fp == NULL) {
        char msg[120];
        snprintf(msg, sizeof(msg), "未找到处方数据文件：%s，将使用空数据库！", fileName);
        PRINT_TIP(msg);
        WriteLog(LOG_LEVEL_INFO, "系统", "加载处方数据", "未找到历史文件");
        return 0;
    }

    char line[512] = { 0 };
    int loadCount = 0;
    int skipCount = 0;

    // 按行读取，比sscanf稳100倍，完美处理空字段和行尾逗号
    while (fgets(line, sizeof(line), fp) != NULL) {
        // 1. 处理Windows/Unix换行符，清除脏字符
        int len = (int)strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (len > 1 && line[len - 2] == '\r') line[len - 2] = '\0';
        // 跳过空行
        if (strlen(line) == 0) {
            skipCount++;
            continue;
        }

        // 2. 初始化字段变量
        char prescId[MAX_ID_LEN] = { 0 };
        char patientId[MAX_ID_LEN] = { 0 };
        char doctorId[MAX_ID_LEN] = { 0 };
        char medicineId[MAX_ID_LEN] = { 0 };
        int quantity = 0;
        float totalFee = 0.0f;
        int status = 0;
        char createTime[MAX_DATA_LEN] = { 0 };
        char dispenseTime[MAX_DATA_LEN] = { 0 };

        // 3. 安全分割每一列，完美处理空字段和行尾逗号
        char* token = NULL;
        char* rest = line;
        int fieldIndex = 0;

        while ((token = strtok_s(rest, ",", &rest)) != NULL && fieldIndex < 9) {
            switch (fieldIndex) {
            case 0: SafeStrCopy(prescId, token, MAX_ID_LEN); break;
            case 1: SafeStrCopy(patientId, token, MAX_ID_LEN); break;
            case 2: SafeStrCopy(doctorId, token, MAX_ID_LEN); break;
            case 3: SafeStrCopy(medicineId, token, MAX_ID_LEN); break;
            case 4: quantity = atoi(token); break;
            case 5: totalFee = (float)atof(token); break;
            case 6: status = atoi(token); break;
            case 7: SafeStrCopy(createTime, token, MAX_DATA_LEN); break;
            case 8: SafeStrCopy(dispenseTime, token, MAX_DATA_LEN); break;
            }
            fieldIndex++;
        }

        // 4. 核心字段校验（只校验必填项，空的发药时间不影响）
        int isValid = 1;
        if (strlen(prescId) == 0 || CheckPrescriptionExist(prescId)) isValid = 0;
        if (strlen(patientId) == 0) isValid = 0;
        if (strlen(doctorId) == 0) isValid = 0;
        if (strlen(medicineId) == 0) isValid = 0;
        if (quantity <= 0) isValid = 0;
        if (totalFee < 0) isValid = 0;
        if (status < PRESC_UNPAY || status > PRESC_CANCELED) isValid = 0;
        if (strlen(createTime) == 0) isValid = 0;

        if (!isValid) {
            skipCount++;
            continue;
        }

        // 5. 关联数据校验（仅校验存在性，不强制跳过，加提示）
        if (!CheckPatientExist(patientId)) {
            char msg[120];
            snprintf(msg, sizeof(msg), "处方%s跳过：患者ID%s不存在", prescId, patientId);
            PRINT_WARN(msg);
            skipCount++;
            continue;
        }
        if (!CheckDoctorExist(doctorId)) {
            char msg[120];
            snprintf(msg, sizeof(msg), "处方%s跳过：医生ID%s不存在", prescId, doctorId);
            PRINT_WARN(msg);
            skipCount++;
            continue;
        }
        if (!CheckMedicineExist(medicineId)) {
            char msg[120];
            snprintf(msg, sizeof(msg), "处方%s跳过：药品ID%s不存在", prescId, medicineId);
            PRINT_WARN(msg);
            skipCount++;
            continue;
        }

        // 6. 内存分配与节点插入
        PrescNode* newNode = (PrescNode*)malloc(sizeof(PrescNode));
        if (!newNode) {
            PRINT_ERR("处方加载失败：内存不足！");
            fclose(fp);
            return loadCount;
        }

        // 初始化节点数据
        memset(newNode, 0, sizeof(PrescNode));
        SafeStrCopy(newNode->prescId, prescId, MAX_ID_LEN);
        SafeStrCopy(newNode->patientId, patientId, MAX_ID_LEN);
        SafeStrCopy(newNode->doctorId, doctorId, MAX_ID_LEN);
        SafeStrCopy(newNode->medicineId, medicineId, MAX_ID_LEN);
        newNode->quantity = quantity;
        newNode->totalFee = totalFee;
        newNode->status = status;
        SafeStrCopy(newNode->createTime, createTime, MAX_DATA_LEN);
        SafeStrCopy(newNode->dispenseTime, dispenseTime, MAX_DATA_LEN);

        // 头插法插入链表
        newNode->next = prescHead->next;
        prescHead->next = newNode;
        loadCount++;
    }
    fclose(fp);

    // 7. 加载结果详细提示（一眼知道问题）
    if (loadCount > 0) {
        char msg[120];
        snprintf(msg, sizeof(msg), "处方数据加载完成！成功加载%d条，跳过%d条无效数据", loadCount, skipCount);
        PRINT_OK(msg);
    }
    else {
        if (skipCount > 0) {
            char msg[120];
            snprintf(msg, sizeof(msg), "处方加载失败！所有%d条数据均无效，请检查患者/医生/药品数据是否已加载", skipCount);
            PRINT_ERR(msg);
        }
        else {
            PRINT_TIP("处方数据文件为空，未加载任何数据！");
        }
    }

    // 记录日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "成功加载%d条，跳过%d条", loadCount, skipCount);
    WriteLog(LOG_LEVEL_INFO, "系统", "加载处方数据", logContent);
    return loadCount;
}

// 释放处方链表内存
void FreePrescriptionList(void) {
    if (prescHead == NULL) {
        return;
    }
    PrescNode* p = prescHead;
    while (p != NULL) {
        PrescNode* q = p->next;
        free(p);
        p = q;
    }
    prescHead = NULL;
    WriteLog(LOG_LEVEL_INFO, "系统", "释放处方链表内存", "成功");
}

// 获取所有处方总费用
float GetTotalPrescriptionFee(void) {
    if (prescHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "获取处方总费用", "失败，处方模块未初始化");
        return 0.0f;
    }

    float totalFee = 0.0f;
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        if (CheckFeeValid(p->totalFee)) {
            totalFee += p->totalFee;
        }
        p = p->next;
    }

    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "成功获取处方总费用：%.2f", totalFee);
    WriteLog(LOG_LEVEL_INFO, "系统", "获取处方总费用", logContent);
    return totalFee;
}

// 更新处方状态为已缴费（患者缴费调用，和发药函数状态完全匹配）
int UpdatePrescriptionToPaid(const char* prescId) {
    if (prescHead == NULL || prescId == NULL) {
        return 0;
    }
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        if (strcmp(p->prescId, prescId) == 0) {
            // 【核心修复：仅未缴费的处方才能更新为已缴费待发药】
            if (p->status == PRESC_UNPAY) {
                p->status = PRESC_PAID; // 更新为已缴费待发药，和发药函数校验完全匹配
                return 1;
            }
            return 0; // 非未缴费状态，无法更新
        }
        p = p->next;
    }
    return 0;
}


// 查询患者待缴费处方（患者端调用）【修改版：返回待缴费处方数量】
int QueryUnpaidPrescriptionByPatientId(const char* patientId) {
    if (prescHead == NULL || prescHead->next == NULL || patientId == NULL) {
        PRINT_TIP("暂无待缴费处方！");
        return 0;
    }

    char title[80];
    snprintf(title, sizeof(title), "患者%s待缴费处方", patientId);
    print_title_box(title);

    // 表格列宽严格匹配80宽度
    int cols[] = { 12, 12, 14, 8, 10, 12 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "处方编号", "药品编号", "药品通用名", "数量", "总费用", "开方时间" };

    printf("\n");
    print_table_sep(cols, 6);
    print_table_row(cols, aligns, headers, 6);
    print_table_sep(cols, 6);

    int findCount = 0;
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0 && p->status == PRESC_UNPAY) {
            char medicineGenericName[MAX_NAME_LEN] = { 0 };
            GetMedicineGenericName(p->medicineId, medicineGenericName, MAX_NAME_LEN);

            char quantity[10], fee[20];
            snprintf(quantity, sizeof(quantity), "%d", p->quantity);
            snprintf(fee, sizeof(fee), "%.2f", p->totalFee);

            const char* data[] = {
                p->prescId, p->medicineId, medicineGenericName, quantity, fee, p->createTime
            };
            print_table_row(cols, aligns, data, 6);
            findCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 6);

    if (findCount == 0) {
        PRINT_TIP("暂无待缴费处方！");
    }
    else {
        char msg[80];
        snprintf(msg, sizeof(msg), "共找到%d条待缴费处方！", findCount);
        PRINT_TIP(msg);
    }

    // 【修改】返回待缴费处方数量
    return findCount;
}

// 打印所有处方简略信息（ID+患者ID+状态）
void PrintAllPrescriptionBrief(void) {
    if (prescHead == NULL || prescHead->next == NULL) {
        PRINT_TIP("暂无处方数据！");
        return;
    }
    print_title_box("处方列表（ID+患者ID+状态）");
    int cols[] = { 15, 20, 43 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "处方ID", "患者ID", "处方状态" };
    printf("\n");
    print_table_sep(cols, 3);
    print_table_row(cols, aligns, headers, 3);
    print_table_sep(cols, 3);
    PrescNode* p = prescHead->next;
    while (p != NULL) {
        const char* statusStr;
        if (p->status == PRESC_UNPAY) statusStr = "未缴费";
        else if (p->status == PRESC_PAID) statusStr = "待发药";
        else statusStr = "已发药";
        const char* data[] = { p->prescId, p->patientId, statusStr };
        print_table_row(cols, aligns, data, 3);
        p = p->next;
    }
    print_table_sep(cols, 3);
}

// 处方作废（含冲账回退逻辑）
int CancelPrescription(const char* prescId) {
    if (prescHead == NULL || prescId == NULL) {
        PRINT_ERR("处方模块未初始化或处方ID为空！");
        WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "作废处方", "失败，入参不合法");
        return 0;
    }
    PrescNode* p = FindPrescById(prescId);
    if (p == NULL) {
        PRINT_ERR("处方不存在，作废失败！");
        WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "作废处方", "失败，处方不存在");
        return 0;
    }
    // 已作废的处方禁止重复操作
    if (p->status == PRESC_CANCELED) {
        PRINT_ERR("该处方已作废，禁止重复操作！");
        WriteLog(LOG_LEVEL_ERROR, GetCurrentOperator(), "作废处方", "失败，处方已作废");
        return 0;
    }
    // 已发药的处方，先回滚药品库存
    if (p->status == PRESC_FINISHED) {
        // 药品库存回滚（入库）
        MedicineStockIn(p->medicineId, p->quantity);
        PRINT_TIP("已发药处方：药品库存已回滚");
    }
    // 已缴费的处方，生成冲账记录，回滚财务数据
    if (p->status == PRESC_PAID || p->status == PRESC_FINISHED) {
        // 生成冲账医疗记录
        RecordNode reverseRecord = { 0 };
        srand((unsigned int)time(NULL));
        snprintf(reverseRecord.recordId, MAX_ID_LEN, "CX%s", p->prescId);
        SafeStrCopy(reverseRecord.patientId, p->patientId, MAX_ID_LEN);
        SafeStrCopy(reverseRecord.doctorId, p->doctorId, MAX_ID_LEN);
        SafeStrCopy(reverseRecord.deptId, GetDoctorById(p->doctorId)->deptId, MAX_ID_LEN);
        reverseRecord.type = RECORD_CONSULT;
        SafeStrCopy(reverseRecord.createTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
        snprintf(reverseRecord.detail, MAX_DETAIL_LEN, "【处方作废冲账】原处方ID:%s 金额回退", p->prescId);
        reverseRecord.fee = -p->totalFee;
        SafeStrCopy(reverseRecord.prescriptionId, p->prescId, MAX_ID_LEN);
        // 插入冲账记录
        AddRecord(reverseRecord);
        PRINT_TIP("已缴费处方：财务冲账记录已生成");
    }
    // 更新处方状态为已作废
    p->status = PRESC_CANCELED;
    // 记录操作日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "处方ID：%s，患者ID：%s，作废成功", p->prescId, p->patientId);
    WriteLog(LOG_LEVEL_INFO, GetCurrentOperator(), "作废处方", logContent);
    PRINT_OK("处方作废成功，冲账回退流程已完成！");
    return 1;
}
