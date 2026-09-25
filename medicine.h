/*药物管理模块*/
#ifndef MEDICINE_H
#define MEDICINE_H

#define SYSTEM_FLAG_FILE "system_init.flag"          //系统初始化标志文件，存在表示已经初始化过了

#include "stdio.h"

//初始化药品链表（系统启动时调用1次）(失败0，成功1)
void InitMedicineList();
//检查药品是否存在（存在1 不存在0）
int CheckMedicineExist(const char* medicineId);
//查询药品库存
int QueryMedicineStock(const char* medicineId);
//查询药品售价
float GetMedicineSalePrice(const char* medicineId);
//查询药品通用名
int GetMedicineGenericName(const char* medicineId, char* buffer, int bufferLen);
//校验药品有没有未发药的处方
int CheckMedicineHasUndispensedPresc(const char* medicineId);
//药品入库
int MedicineStockIn();
//药品出库
int MedicineStockOut(const char* medicineId, int reduceNum);
//新增药品
int AddMedicine();
//修改药品信息
int ModifyMedicine();
//删除药品
int DeleteMedicine();
//按名称模糊查询药品
void QueryMedicineByName();
//按科室查询药品
void QueryMedicineByDeptId();
//打印所有药品
void PrintAllMedicine();
//药品数据保存到文件
int SaveMedicineToFile(const char* fileName);
//从文件加载药品数据
int LoadMedicineFromFile(const char* fileName);
//释放药品链表内存
void FreeMedicineList();
//统计药品消耗TOP10
void StatMedicineConsumeTop10(FILE* fp);
//生成药品库存报表
void GenerateMedicineStockReport(FILE* fp);
// 获取所有已售出药品的总进价成本（用于计算盈利）
float GetMedicineTotalSoldPurchaseCost();
// 获取所有已售出药品的总售价收入（用于计算盈利）
float GetMedicineTotalSoldSaleIncome();
//按科室ID查询并打印该科室的所有药品（用于科室聚合查询）
void PrintMedicineByDeptId(const char* deptId);
// 打印所有药品简略信息（ID+通用名）
void PrintAllMedicineBrief(void);

#endif // MEDICINE_H