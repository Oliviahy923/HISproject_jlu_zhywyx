// 先包含系统头文件
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// 【必须优先包含common.h，统一全局打印规范】
#include "common.h"
// 再包含自定义业务头文件
#include "medicine.h"
#include "log.h"
#include "department.h"
#include "safe_utils.h"
#include "prescription.h"
#include "system.h"

// ==================== 核心数据结构定义 ====================
// 药品信息结构体
typedef struct Medicine {
    char medicineId[MAX_ID_LEN];
    char genericName[MAX_NAME_LEN];
    char productName[MAX_NAME_LEN];
    char alias[MAX_NAME_LEN];
    char category[MAX_NAME_LEN];
    char deptId[MAX_NAME_LEN];
    int stock;
    float purchasePrice;
    float salePrice;
    int totalConsume;
    struct Medicine* next;
} MedicineNode, * MedicineList;

// 药品类别链表（用于库存报表分类统计）
typedef struct CategoryNode {
    char name[MAX_NAME_LEN];
    struct CategoryNode* next;
} CategoryNode;

// ==================== 内部私有静态变量 ====================
static MedicineList medicineHead = NULL;

// ==================== 内部私有辅助函数 ====================
// 按ID查找药品（内部使用）
static MedicineNode* FindMedicineById(const char* medicineId) {
    if (medicineId == NULL || medicineHead == NULL) {
        return NULL;
    }
    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        if (strcmp(p->medicineId, medicineId) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

// 创建系统初始化标志文件
static void CreateSystemFlagFile() {
    FILE* fp = fopen(SYSTEM_FLAG_FILE, "r");
    if (fp != NULL) {
        fclose(fp);
        return;
    }
    fp = fopen(SYSTEM_FLAG_FILE, "w");
    if (fp != NULL) {
        fprintf(fp, "System initialized flag\n");
        fclose(fp);
    }
}

// 报表钻取：按类别打印药品明细
static void DrillDownToCategoryDetail(const char* categoryName, FILE* fp) {
    (void)fp; // 兼容文件输出入参
    // 药品明细用120宽度，调用common.h的_ex函数
    print_title_box_ex(categoryName, RECORD_BOX_WIDTH);
    int cols[] = { 12, 20, 20, 10, 10, 10, 20 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "药品编号","通用名","商品名","库存","进价","售价","库存金额" };
    printf("\n");
    print_table_sep_ex(cols, 7, RECORD_BOX_WIDTH);
    print_table_row_ex(cols, aligns, headers, 7, RECORD_BOX_WIDTH);
    print_table_sep_ex(cols, 7, RECORD_BOX_WIDTH);
    MedicineNode* p = medicineHead->next;
    int count = 0;
    float totalStockValue = 0.0f;
    while (p != NULL) {
        if (strcmp(p->category, categoryName) == 0) {
            float val = p->stock * p->salePrice;
            char buf[4][20];
            snprintf(buf[0], sizeof(buf[0]), "%d", p->stock);
            snprintf(buf[1], sizeof(buf[1]), "%.2f", (double)p->purchasePrice);
            snprintf(buf[2], sizeof(buf[2]), "%.2f", (double)p->salePrice);
            snprintf(buf[3], sizeof(buf[3]), "%.2f", (double)val);
            const char* data[] = { p->medicineId,p->genericName,p->productName,buf[0],buf[1],buf[2],buf[3] };
            print_table_row_ex(cols, aligns, data, 7, RECORD_BOX_WIDTH);
            count++; totalStockValue += val;
        }
        p = p->next;
    }
    print_table_sep_ex(cols, 7, RECORD_BOX_WIDTH);
    char msg[80];
    snprintf(msg, sizeof(msg), "该类别共%d种药品，总金额：%.2f元", count, (double)totalStockValue);
    PRINT_TIP(msg);
}
// ==================== 对外接口函数实现 ====================
// 初始化药品链表
void InitMedicineList(void) {
    if (medicineHead != NULL) {
        FreeMedicineList();
    }

    medicineHead = (MedicineList)malloc(sizeof(MedicineNode));
    if (medicineHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "初始化药品链表", "失败：内存分配失败");
        PRINT_ERR("药品模块初始化失败，内存不足！");
        return;
    }

    medicineHead->next = NULL;
    WriteLog(LOG_LEVEL_INFO, "系统", "初始化药品链表", "成功");
    PRINT_OK("药品模块初始化成功！");
}

// 按ID校验药品是否存在（1=存在，0=不存在）
int CheckMedicineExist(const char* medicineId) {
    return FindMedicineById(medicineId) != NULL ? 1 : 0;
}

// 查询药品库存（失败返回-1）
int QueryMedicineStock(const char* medicineId) {
    MedicineNode* p = FindMedicineById(medicineId);
    if (p == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "查询药品库存", "失败：药品不存在");
        PRINT_ERR("药品不存在！");
        return -1;
    }
    return p->stock;
}

// 查询药品售价（失败返回-1.0f）
float GetMedicineSalePrice(const char* medicineId) {
    if (medicineId == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "查询药品售价", "失败：药品ID为NULL");
        PRINT_ERR("药品ID不能为空！");
        return -1.0f;
    }
    MedicineNode* p = FindMedicineById(medicineId);
    if (p == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "查询药品售价", "失败：药品不存在");
        PRINT_ERR("药品不存在！");
        return -1.0f;
    }
    return p->salePrice;
}

// 查询药品通用名（成功返回1，失败返回0）
int GetMedicineGenericName(const char* medicineId, char* buffer, int bufferLen) {
    if (!CheckNullPtr(2, medicineId, buffer) || bufferLen <= 0 || medicineHead == NULL) {
        return 0;
    }
    MedicineNode* p = FindMedicineById(medicineId);
    if (p == NULL) {
        return 0;
    }
    return SafeStrCopy(buffer, p->genericName, bufferLen);
}

// 校验药品是否有未发药的处方（1=有，0=无，-1=异常）
int CheckMedicineHasUndispensedPresc(const char* medicineId) {
    if (!CheckNullPtr(1, medicineId)) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "检查药品未发药处方", "失败：入参不合法");
        return -1;
    }
    int result = PrescCheckMedicineHasUndispensedPresc(medicineId);
    if (result == -1) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "检查药品未发药处方", "失败：调用处方模块出错");
    }
    return result;
}

// 交互式新增药品
int AddMedicine(void) {
    if (medicineHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "管理员", "新增药品", "失败：药品链表未初始化");
        PRINT_ERR("药品模块未初始化！");
        return 0;
    }

    char medicineId[MAX_ID_LEN] = { 0 };
    char genericName[MAX_NAME_LEN] = { 0 };
    char productName[MAX_NAME_LEN] = { 0 };
    char alias[MAX_NAME_LEN] = { 0 };
    char category[MAX_NAME_LEN] = { 0 };
    char deptId[MAX_ID_LEN] = { 0 };
    int stock = 0;
    float purchasePrice = 0.0f;
    float salePrice = 0.0f;

    print_title_box("新增药品");
    // 校验药品ID唯一性
    PRINT_TIP("现有药品列表");
    PrintAllMedicineBrief();
    printf("\n");
    while (1) {
        SafeStrInput("请输入药品的唯一ID", medicineId, MAX_ID_LEN);
        if (CheckMedicineExist(medicineId)) {
            PRINT_WARN("药品ID已存在，请重新输入！");
            continue;
        }
        break;
    }
    SafeStrInput("请输入药品的通用名", genericName, MAX_NAME_LEN);
    SafeStrInput("请输入药品的商品名", productName, MAX_NAME_LEN);
    SafeStrInput("请输入药品的别名", alias, MAX_NAME_LEN);
    SafeStrInput("请输入药品的类型", category, MAX_NAME_LEN);
    // 校验关联科室是否存在
    PRINT_TIP("可选科室列表");
    PrintAllDepartmentBrief();
    printf("\n");
    while (1) {
        SafeStrInput("请输入药品关联的科室ID", deptId, MAX_ID_LEN);
        if (!CheckDepartmentExist(deptId)) {
            PRINT_WARN("科室ID不存在，请重新输入！");
            continue;
        }
        break;
    }

    SafeIntInput("请输入药品的库存数量", &stock, 0, 100000);

    // 校验进价合法性
    while (1) {
        char purchasePriceStr[MAX_DATA_LEN] = { 0 };
        SafeStrInput("请输入药品的进价", purchasePriceStr, MAX_DATA_LEN);
        purchasePrice = (float)atof(purchasePriceStr);
        if (!CheckFeeValid(purchasePrice)) {
            PRINT_WARN("金额格式错误，请重新输入！");
            continue;
        }
        break;
    }

    // 校验售价合法性
    while (1) {
        char salePriceStr[MAX_DATA_LEN] = { 0 };
        SafeStrInput("请输入药品的售价", salePriceStr, MAX_DATA_LEN);
        salePrice = (float)atof(salePriceStr);
        if (!CheckFeeValid(salePrice)) {
            PRINT_WARN("金额格式错误，请重新输入！");
            continue;
        }
        break;
    }

    // 分配新节点内存
    MedicineNode* newNode = (MedicineList)malloc(sizeof(MedicineNode));
    if (newNode == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "管理员", "新增药品", "失败：内存分配失败");
        PRINT_ERR("内存不足，新增失败！");
        return 0;
    }

    // 赋值新节点数据
    SafeStrCopy(newNode->medicineId, medicineId, MAX_ID_LEN);
    SafeStrCopy(newNode->genericName, genericName, MAX_NAME_LEN);
    SafeStrCopy(newNode->productName, productName, MAX_NAME_LEN);
    SafeStrCopy(newNode->alias, alias, MAX_NAME_LEN);
    SafeStrCopy(newNode->category, category, MAX_NAME_LEN);
    SafeStrCopy(newNode->deptId, deptId, MAX_ID_LEN);
    newNode->stock = stock;
    newNode->purchasePrice = purchasePrice;
    newNode->salePrice = salePrice;
    newNode->totalConsume = 0;

    // 插入链表头部
    newNode->next = medicineHead->next;
    medicineHead->next = newNode;

    // 记录操作日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "药品ID：%s", medicineId);
    WriteLog(LOG_LEVEL_INFO, "管理员", "新增药品", logContent);
    PRINT_OK("药品新增成功！");
    return 1;
}

// 药品出库（处方发药专用）
int MedicineStockOut(const char* medicineId, int reduceNum) {
    if (!CheckNullPtr(1, medicineId)) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "药品出库", "失败：药品ID参数为空");
        PRINT_ERR("药品ID不能为空！");
        return 0;
    }
    if (reduceNum <= 0) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "药品出库", "失败：出库数量必须大于0");
        PRINT_ERR("出库数量必须大于0！");
        return 0;
    }

    MedicineNode* p = FindMedicineById(medicineId);
    if (p == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "药品出库", "失败：药品不存在");
        PRINT_ERR("药品不存在！");
        return 0;
    }

    // 校验库存是否充足
    if (p->stock < reduceNum) {
        char msg[80];
        snprintf(msg, sizeof(msg), "库存不足！当前：%d，需出库：%d", p->stock, reduceNum);
        PRINT_ERR(msg);
        char logContent[MAX_DETAIL_LEN] = { 0 };
        snprintf(logContent, MAX_DETAIL_LEN, "药品ID：%s 库存不足", medicineId);
        WriteLog(LOG_LEVEL_ERROR, "系统", "药品出库", logContent);
        return 0;
    }

    // 更新库存和累计销量
    p->stock -= reduceNum;
    p->totalConsume += reduceNum;

    // 记录操作日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "药品ID：%s 出库成功", medicineId);
    WriteLog(LOG_LEVEL_INFO, "系统", "药品出库", logContent);
    PRINT_OK("药品出库成功！");
    return 1;
}

// 交互式删除药品
int DeleteMedicine(void) {
    if (medicineHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "管理员", "删除药品", "失败：药品链表未初始化");
        PRINT_ERR("药品模块未初始化！");
        return 0;
    }

    char medicineId[MAX_ID_LEN] = { 0 };
    print_title_box("删除药品");

    PRINT_TIP("现有药品列表");
    PrintAllMedicineBrief();
    printf("\n");

    // 校验药品是否存在
    while (1) {
        SafeStrInput("请输入要删除的药品ID", medicineId, MAX_ID_LEN);
        if (!CheckMedicineExist(medicineId)) {
            PRINT_WARN("药品ID不存在，请重新输入！");
            continue;
        }
        break;
    }

    // 校验是否有未发药处方
    if (CheckMedicineHasUndispensedPresc(medicineId)) {
        PRINT_ERR("该药品有未发药处方，无法删除！");
        WriteLog(LOG_LEVEL_ERROR, "管理员", "删除药品", "失败：药品有未发药的处方");
        return 0;
    }

    // 二次确认删除
    int confirm = 0;
    SafeIntInput("确认删除？1=确认，0=取消", &confirm, 0, 1);
    if (!confirm) {
        PRINT_TIP("已取消删除操作");
        WriteLog(LOG_LEVEL_INFO, "管理员", "删除药品", "用户取消删除操作");
        return 0;
    }

    // 查找前驱节点，边界校验避免空指针
    MedicineNode* prev = medicineHead;
    while (prev->next != NULL && strcmp(prev->next->medicineId, medicineId) != 0) {
        prev = prev->next;
    }
    if (prev->next == NULL) {
        PRINT_ERR("药品不存在，删除失败！");
        return 0;
    }

    // 移除节点并释放内存
    MedicineNode* p = prev->next;
    prev->next = p->next;
    free(p);

    // 记录操作日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "药品ID：%s", medicineId);
    WriteLog(LOG_LEVEL_INFO, "管理员", "删除药品", logContent);
    PRINT_OK("药品删除成功！");
    return 1;
}

// 药品数据保存到文件
int SaveMedicineToFile(const char* fileName) {
    if (medicineHead == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "保存药品数据到文件", "失败：药品链表未初始化");
        PRINT_ERR("药品模块未初始化！");
        return 0;
    }
    if (fileName == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "保存药品数据到文件", "失败：文件名参数为空");
        PRINT_ERR("文件名不能为空！");
        return 0;
    }

    FILE* fp = fopen(fileName, "w");
    if (fp == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "保存药品数据到文件", "失败：文件打开失败");
        PRINT_ERR("文件打开失败！");
        return 0;
    }

    // 遍历链表写入数据
    MedicineNode* p = medicineHead->next;
    int saveCount = 0;
    while (p != NULL) {
        fprintf(fp, "%s,%s,%s,%s,%s,%s,%d,%.2f,%.2f,%d\n",
            p->medicineId, p->genericName, p->productName, p->alias, p->category, p->deptId,
            p->stock, p->purchasePrice, p->salePrice, p->totalConsume);
        p = p->next;
        saveCount++;
    }
    fclose(fp);

    // 创建系统标志文件
    CreateSystemFlagFile();

    // 记录操作日志
    WriteLog(LOG_LEVEL_INFO, "系统", "保存药品数据到文件", "成功");
    char msg[80];
    snprintf(msg, sizeof(msg), "保存成功，共%d条数据", saveCount);
    PRINT_OK(msg);
    return 1;
}

// 从文件加载药品数据
int LoadMedicineFromFile(const char* fileName) {
    FILE* fp = fopen(fileName, "r");
    if (fp == NULL) {
        PRINT_TIP("未找到药品数据文件，新建空库");
        return 0;
    }

    MedicineNode newMedicine = { 0 };
    int successCount = 0;
    int failCount = 0;

    // 按格式读取文件数据
    while (fscanf(fp, "%[^,],%[^,],%[^,],%[^,],%[^,],%[^,],%d,%f,%f,%d\n",
        newMedicine.medicineId,
        newMedicine.genericName,
        newMedicine.productName,
        newMedicine.alias,
        newMedicine.category,
        newMedicine.deptId,
        &newMedicine.stock,
        &newMedicine.purchasePrice,
        &newMedicine.salePrice,
        &newMedicine.totalConsume) == 10) {

        MedicineNode* newNode = (MedicineList)malloc(sizeof(MedicineNode));
        if (newNode != NULL) {
            memcpy(newNode, &newMedicine, sizeof(MedicineNode));
            newNode->next = medicineHead->next;
            medicineHead->next = newNode;
            successCount++;
        }
        else {
            failCount++;
        }
        memset(&newMedicine, 0, sizeof(MedicineNode));
    }

    fclose(fp);
    // 打印加载结果
    if (successCount > 0) {
        char msg[80];
        snprintf(msg, sizeof(msg), "药品数据加载成功，共%d条有效数据", successCount);
        PRINT_OK(msg);
    }
    else {
        PRINT_TIP("药品数据文件为空，未加载任何数据！");
    }
    return successCount;
}

// 交互式修改药品数据
int ModifyMedicine(void) {
    if (medicineHead == NULL) {
        PRINT_ERR("药品模块未初始化！");
        return 0;
    }

    char medicineId[MAX_ID_LEN] = { 0 };
    print_title_box("修改药品数据");
    PRINT_TIP("现有药品列表");
    PrintAllMedicineBrief();
    printf("\n");
    SafeStrInput("请输入要修改的药品ID", medicineId, MAX_ID_LEN);

    MedicineNode* p = FindMedicineById(medicineId);
    if (p == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "管理员", "修改药品数据", "失败：药品不存在");
        PRINT_ERR("药品不存在！");
        return 0;
    }

    // 标记未使用变量，避免编译警告
    int oldStock = p->stock;
    float oldPurchasePrice = p->purchasePrice;
    float oldSalePrice = p->salePrice;
    (void)oldStock;
    (void)oldPurchasePrice;
    (void)oldSalePrice;

    int newStock = 0;
    float newPurchasePrice = 0.0f;
    float newSalePrice = 0.0f;

    SafeIntInput("请输入新的库存数量", &newStock, 0, 100000);

    // 校验新进价合法性
    while (1) {
        char purchasePriceStr[MAX_DATA_LEN] = { 0 };
        SafeStrInput("请输入新的进价", purchasePriceStr, MAX_DATA_LEN);
        newPurchasePrice = (float)atof(purchasePriceStr);
        if (!CheckFeeValid(newPurchasePrice)) {
            PRINT_WARN("金额格式错误，请重新输入！");
            continue;
        }
        break;
    }

    // 校验新售价合法性
    while (1) {
        char salePriceStr[MAX_DATA_LEN] = { 0 };
        SafeStrInput("请输入新的售价", salePriceStr, MAX_DATA_LEN);
        newSalePrice = (float)atof(salePriceStr);
        if (!CheckFeeValid(newSalePrice)) {
            PRINT_WARN("金额格式错误，请重新输入！");
            continue;
        }
        break;
    }

    // 二次确认修改
    int confirm = 0;
    SafeIntInput("确认修改？1=确认，0=取消", &confirm, 0, 1);
    if (!confirm) {
        PRINT_TIP("已取消修改操作");
        WriteLog(LOG_LEVEL_INFO, "管理员", "修改药品数据", "用户取消修改操作");
        return 0;
    }

    // 更新药品数据
    p->stock = newStock;
    p->purchasePrice = newPurchasePrice;
    p->salePrice = newSalePrice;

    // 记录操作日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "药品ID:%s 修改成功", medicineId);
    WriteLog(LOG_LEVEL_INFO, "管理员", "修改药品数据", logContent);
    PRINT_OK("药品数据修改成功！");
    return 1;
}

// 交互式药品入库
int MedicineStockIn(void) {
    if (medicineHead == NULL) {
        PRINT_ERR("药品模块未初始化！");
        return 0;
    }

    char medicineId[MAX_ID_LEN] = { 0 };
    int addNum = 0;
    print_title_box("药品入库");
    PRINT_TIP("现有药品列表");
    PrintAllMedicineBrief();
    printf("\n");
    SafeStrInput("请输入要入库的药品ID", medicineId, MAX_ID_LEN);

    MedicineNode* p = FindMedicineById(medicineId);
    if (p == NULL) {
        WriteLog(LOG_LEVEL_ERROR, "系统", "药品入库", "失败：药品不存在");
        PRINT_ERR("药品不存在！");
        return 0;
    }

    SafeIntInput("请输入入库数量", &addNum, 1, 100000);
    p->stock += addNum;

    // 记录操作日志
    char logContent[MAX_DETAIL_LEN] = { 0 };
    snprintf(logContent, MAX_DETAIL_LEN, "药品ID:%s 入库成功", medicineId);
    WriteLog(LOG_LEVEL_INFO, "系统", "药品入库", logContent);

    // 打印入库结果
    char msg[80];
    snprintf(msg, sizeof(msg), "入库成功！当前库存：%d", p->stock);
    PRINT_OK(msg);
    return 1;
}

// 按名称模糊查询药品
void QueryMedicineByName(void) {
    if (medicineHead == NULL || medicineHead->next == NULL) {
        PRINT_TIP("暂无药品数据！");
        return;
    }

    char name[MAX_NAME_LEN] = { 0 };
    print_title_box("药品模糊查询");
    SafeStrInput("请输入药品名称关键词", name, MAX_NAME_LEN);

    int cols[] = { 12, 20, 20, 18, 12, 8, 12 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "药品编号","通用名","商品名","类别","关联科室","库存","售价" };

    printf("\n");
    print_table_sep(cols, 7);
    print_table_row(cols, aligns, headers, 7);
    print_table_sep(cols, 7);

    int findCount = 0;
    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        // 通用名/商品名/别名模糊匹配
        if (strstr(p->genericName, name) != NULL || strstr(p->productName, name) != NULL || strstr(p->alias, name) != NULL) {
            char stock[10], price[10];
            snprintf(stock, sizeof(stock), "%d", p->stock);
            snprintf(price, sizeof(price), "%.2f", (double)p->salePrice);
            const char* data[] = { p->medicineId, p->genericName, p->productName, p->category, p->deptId, stock, price };
            print_table_row(cols, aligns, data, 7);
            findCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 7);

    // 打印查询结果
    char msg[80];
    snprintf(msg, sizeof(msg), "查询完成，共找到%d个匹配药品", findCount);
    PRINT_TIP(msg);
    WriteLog(LOG_LEVEL_INFO, "用户", "查询药品", "成功");
}

// 按科室ID查询关联药品
void QueryMedicineByDeptId(void) {
    if (medicineHead == NULL || medicineHead->next == NULL) {
        PRINT_TIP("暂无药品数据！");
        return;
    }

    char deptId[MAX_NAME_LEN] = { 0 };
    print_title_box("科室关联药品查询");
    PRINT_TIP("可选科室列表");
    PrintAllDepartmentBrief();
    printf("\n");

    // 校验科室是否存在
    while (1) {
        SafeStrInput("请输入科室编号", deptId, MAX_NAME_LEN);
        if (!CheckDepartmentExist(deptId)) {
            PRINT_WARN("科室不存在，请重新输入！");
            continue;
        }
        break;
    }

    int cols[] = { 15, 25, 25, 20, 14, 14 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "药品编号","通用名","商品名","类别","库存","售价" };

    printf("\n");
    print_table_sep(cols, 6);
    print_table_row(cols, aligns, headers, 6);
    print_table_sep(cols, 6);

    int findCount = 0;
    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        if (strcmp(p->deptId, deptId) == 0) {
            char stock[10], price[10];
            snprintf(stock, sizeof(stock), "%d", p->stock);
            snprintf(price, sizeof(price), "%.2f", (double)p->salePrice);
            const char* data[] = { p->medicineId, p->genericName, p->productName, p->category, stock, price };
            print_table_row(cols, aligns, data, 6);
            findCount++;
        }
        p = p->next;
    }
    print_table_sep(cols, 6);

    // 打印查询结果
    char msg[80];
    snprintf(msg, sizeof(msg), "共找到%d个关联药品", findCount);
    PRINT_TIP(msg);
    WriteLog(LOG_LEVEL_INFO, "用户", "查询科室关联药品", "成功");
}

// 打印所有药品信息
void PrintAllMedicine(void) {
    if (medicineHead == NULL) {
        PRINT_ERR("药品模块未初始化！");
        return;
    }
    if (medicineHead->next == NULL) {
        PRINT_TIP("暂无药品数据！");
        return;
    }

    print_title_box("所有药品信息");

    int cols[] = { 12, 20, 20, 18, 12, 8, 12 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "药品编号","通用名","商品名","类别","关联科室","库存","售价" };

    printf("\n");
    print_table_sep(cols, 7);
    print_table_row(cols, aligns, headers, 7);
    print_table_sep(cols, 7);

    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        char stock[10], price[10];
        snprintf(stock, sizeof(stock), "%d", p->stock);
        snprintf(price, sizeof(price), "%.2f", (double)p->salePrice);
        const char* data[] = { p->medicineId, p->genericName, p->productName, p->category, p->deptId, stock, price };
        print_table_row(cols, aligns, data, 7);
        p = p->next;
    }
    print_table_sep(cols, 7);
    WriteLog(LOG_LEVEL_INFO, "用户", "打印所有药品", "成功");
}

// 药品消耗TOP10统计（支持文件输出）
void StatMedicineConsumeTop10(FILE* fp) {
    if (medicineHead == NULL || medicineHead->next == NULL) {
        PRINT_TIP("暂无药品数据！");
        if (fp != NULL) fprintf(fp, "暂无药品数据！\n");
        return;
    }

    // 统计药品总数
    int count = 0;
    MedicineNode* p = medicineHead->next;
    while (p != NULL) { count++; p = p->next; }

    // 分配排序数组，校验内存
    MedicineNode** arr = (MedicineNode**)malloc(sizeof(MedicineNode*) * count);
    if (arr == NULL) {
        PRINT_ERR("内存不足，统计失败！");
        return;
    }

    // 填充数组
    p = medicineHead->next;
    for (int i = 0; i < count; i++) { arr[i] = p; p = p->next; }

    // 冒泡排序（按累计销量降序）
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - 1 - i; j++) {
            if (arr[j]->totalConsume < arr[j + 1]->totalConsume) {
                MedicineNode* temp = arr[j]; arr[j] = arr[j + 1]; arr[j + 1] = temp;
            }
        }
    }

    // 打印统计结果
    print_title_box("药品消耗TOP10排行");

    int cols[] = { 6, 10, 20, 18, 16, 12, 6, 8, 14 };
    AlignType aligns[] = {
        ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER,
        ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER
    };
    const char* headers[] = { "排名","编号","通用名","商品名","类别","科室","库存","售价","销量" };

    printf("\n");
    print_table_sep(cols, 9);
    print_table_row(cols, aligns, headers, 9);
    print_table_sep(cols, 9);

    // 取TOP10，不足则取全部
    int topCount = count < 10 ? count : 10;
    for (int i = 0; i < topCount; i++) {
        char buf[4][10];
        snprintf(buf[0], sizeof(buf[0]), "%d", i + 1);
        snprintf(buf[1], sizeof(buf[1]), "%d", arr[i]->stock);
        snprintf(buf[2], sizeof(buf[2]), "%.2f", (double)arr[i]->salePrice);
        snprintf(buf[3], sizeof(buf[3]), "%d", arr[i]->totalConsume);

        const char* data[] = {
            buf[0], arr[i]->medicineId, arr[i]->genericName, arr[i]->productName,
            arr[i]->category, arr[i]->deptId, buf[1], buf[2], buf[3]
        };
        print_table_row(cols, aligns, data, 9);
    }
    print_table_sep(cols, 9);

    // 释放内存，避免泄漏
    free(arr);
    WriteLog(LOG_LEVEL_INFO, "用户", "统计药品消耗TOP10", "成功");
}

// 按药品类别生成库存报表（支持文件输出）
void GenerateMedicineStockReport(FILE* fp) {
    if (medicineHead == NULL || medicineHead->next == NULL) {
        PRINT_TIP("暂无药品数据！");
        if (fp != NULL) fprintf(fp, "暂无药品数据！\n");
        return;
    }

    // 初始化类别链表头节点，校验内存
    CategoryNode* categoryHead = (CategoryNode*)malloc(sizeof(CategoryNode));
    if (categoryHead == NULL) {
        PRINT_ERR("内存不足，生成报表失败！");
        return;
    }
    categoryHead->next = NULL;

    // 遍历药品，提取所有不重复的类别
    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        int exist = 0;
        CategoryNode* c = categoryHead->next;
        while (c != NULL) {
            if (strcmp(c->name, p->category) == 0) { exist = 1; break; }
            c = c->next;
        }
        // 新增类别节点
        if (!exist) {
            CategoryNode* newNode = (CategoryNode*)malloc(sizeof(CategoryNode));
            if (newNode == NULL) {
                PRINT_WARN("内存不足，部分类别加载失败！");
                continue;
            }
            SafeStrCopy(newNode->name, p->category, MAX_NAME_LEN);
            newNode->next = categoryHead->next;
            categoryHead->next = newNode;
        }
        p = p->next;
    }

    // 打印分类统计报表
    print_title_box("药品库存报表");

    int cols[] = { 24, 10, 10, 30, 30 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "药品类别","总数","总库存","总进价","总售价" };

    printf("\n");
    print_table_sep(cols, 5);
    print_table_row(cols, aligns, headers, 5);
    print_table_sep(cols, 5);

    // 遍历类别，统计每个类别的数据
    CategoryNode* c = categoryHead->next;
    while (c != NULL) {
        int num = 0, stock = 0;
        float pur = 0.0f, sale = 0.0f;
        p = medicineHead->next;
        while (p != NULL) {
            if (strcmp(p->category, c->name) == 0) {
                num++;
                stock += p->stock;
                pur += p->purchasePrice * p->stock;
                sale += p->salePrice * p->stock;
            }
            p = p->next;
        }
        char buf[4][20];
        snprintf(buf[0], sizeof(buf[0]), "%d", num);
        snprintf(buf[1], sizeof(buf[1]), "%d", stock);
        snprintf(buf[2], sizeof(buf[2]), "%.2f", (double)pur);
        snprintf(buf[3], sizeof(buf[3]), "%.2f", (double)sale);
        const char* data[] = { c->name, buf[0], buf[1], buf[2], buf[3] };
        print_table_row(cols, aligns, data, 5);
        c = c->next;
    }
    print_table_sep(cols, 5);

    // 报表钻取功能
    int needDrill = 0;
    SafeIntInput("需要报表钻取？1=是，0=否", &needDrill, 0, 1);
    if (needDrill) {
        char target[MAX_NAME_LEN] = { 0 };
        SafeStrInput("请输入药品类别", target, MAX_NAME_LEN);
        int exist = 0;
        c = categoryHead->next;
        while (c != NULL) {
            if (strcmp(c->name, target) == 0) { exist = 1; break; }
            c = c->next;
        }
        if (exist) {
            DrillDownToCategoryDetail(target, fp);
        }
        else {
            PRINT_ERR("未找到该类别！");
        }
    }
    else {
        PRINT_TIP("已退出报表查看");
    }

    // 释放类别链表内存，避免泄漏
    c = categoryHead->next;
    while (c != NULL) {
        CategoryNode* t = c;
        c = c->next;
        free(t);
    }
    free(categoryHead);
    WriteLog(LOG_LEVEL_INFO, "管理员", "生成药品库存报表", "成功");
}

// 释放药品链表内存
void FreeMedicineList(void) {
    if (medicineHead == NULL) return;
    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        MedicineNode* t = p;
        p = p->next;
        free(t);
    }
    free(medicineHead);
    medicineHead = NULL;
    WriteLog(LOG_LEVEL_INFO, "系统", "释放药品链表内存", "成功");
}

// 获取已售药品总采购成本（用于经营统计）
float GetMedicineTotalSoldPurchaseCost(void) {
    if (medicineHead == NULL) return 0.0f;
    float total = 0.0f;
    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        if (CheckFeeValid(p->purchasePrice)) {
            total += p->purchasePrice * p->totalConsume;
        }
        p = p->next;
    }
    return total;
}

// 获取已售药品总销售收入（用于经营统计）
float GetMedicineTotalSoldSaleIncome(void) {
    if (medicineHead == NULL) return 0.0f;
    float total = 0.0f;
    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        if (CheckFeeValid(p->salePrice)) {
            total += p->salePrice * p->totalConsume;
        }
        p = p->next;
    }
    return total;
}

// 按科室ID打印药品（科室聚合查询专用）
void PrintMedicineByDeptId(const char* deptId) {
    if (!CheckNullPtr(1, deptId) || medicineHead == NULL || medicineHead->next == NULL) {
        PRINT_TIP("暂无该科室药品数据！");
        return;
    }
    if (!CheckDepartmentExist(deptId)) {
        PRINT_ERR("科室不存在！");
        return;
    }

    int cols[] = { 20, 40, 25, 30 };
    AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
    const char* headers[] = { "药品编号","通用名","库存","售价" };
    printf("\n");
    print_table_sep(cols, 4);
    print_table_row(cols, aligns, headers, 4);
    print_table_sep(cols, 4);

    int find = 0;
    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        if (strcmp(p->deptId, deptId) == 0) {
            char s[2][10];
            snprintf(s[0], sizeof(s[0]), "%d", p->stock);
            snprintf(s[1], sizeof(s[1]), "%.2f", (double)p->salePrice);
            const char* data[] = { p->medicineId,p->genericName,s[0],s[1] };
            print_table_row(cols, aligns, data, 4);
            find++;
        }
        p = p->next;
    }
    print_table_sep(cols, 4);

    // 打印查询结果
    char msg[80];
    snprintf(msg, sizeof(msg), "共找到%d个药品", find);
    PRINT_TIP(msg);
}

// 打印所有药品简略信息（ID+通用名）
void PrintAllMedicineBrief(void) {
    if (medicineHead == NULL || medicineHead->next == NULL) {
        PRINT_TIP("暂无药品数据！");
        return;
    }
    print_title_box("药品列表（ID+通用名）");
    int cols[] = { 20, 58 };
    AlignType aligns[] = { ALIGN_CENTER, ALIGN_CENTER };
    const char* headers[] = { "药品ID", "药品通用名" };
    printf("\n");
    print_table_sep(cols, 2);
    print_table_row(cols, aligns, headers, 2);
    print_table_sep(cols, 2);
    MedicineNode* p = medicineHead->next;
    while (p != NULL) {
        const char* data[] = { p->medicineId, p->genericName };
        print_table_row(cols, aligns, data, 2);
        p = p->next;
    }
    print_table_sep(cols, 2);
}
