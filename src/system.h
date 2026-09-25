#ifndef SYSTEM_H
#define SYSTEM_H

#include "common.h"

#define EXIST_SUCCESS 1
#define DEPT_FILE "department.txt"
#define DOCTOR_FILE "doctor.txt"
#define PATIENT_FILE "patient.txt"
#define RECORD_FILE "record.txt"
#define MEDICINE_FILE "medicine.txt"
#define PRESC_FILE "prescription.txt"
#define BED_FILE "bed.txt"
#define REPORT_FILE "system_report.txt"
#define SYSTEM_FLAG_FILE "system_init.flag"
#define REG_FILE "registration.txt"
//枚举定义角色
typedef enum {
	ROLE_ADMIN = 1,                      //管理员：所有权限
	ROLE_DOCTOR = 2,                       //医生：就诊、处方、患者管理权限
	ROLE_PHARMACY = 3,                     //药房：药品、处方发药权限
	ROLE_PATIENT = 4,                      //患者：个人记录、处方查询权限
}SystemRole;

// ==================== 对外暴露函数声明（解决链接报错）====================
// 获取当前登录用户的角色
SystemRole GetCurrentRole();
// 获取当前登录用户的账号/编号
const char* GetCurrentOperator();

//系统全模块初始化（程序启动时调用一次）
void SystemInit();
//系统数据一键保存
void SystemSave();
//系统安全退出（释放所有内存、保存数据）
void SystemExit();
//系统主循环（菜单调度、用户交互）
void SystemLoop();
//核心经营数据统计
void CalcBusinessProfit();
//全量统计报表导出到文件
void ExportAllReportToFile();
// 获取当前登录患者的真实ID（用于业务调度）
#endif
