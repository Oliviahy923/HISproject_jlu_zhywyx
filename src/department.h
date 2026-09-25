#ifndef DEPARTMENT_H
#define DEPARTMENT_H

// 固定长度，直接使用，无需修改
#define MAX_ID_LEN 20
#define MAX_NAME_LEN 50

// ==================== 对外仅暴露2个函数，其他模块只能调用这2个 ====================
// 1. 初始化科室链表（系统启动时调用1次）
void InitDepartmentList();
// 2. 按科室ID校验科室是否存在（1=存在，0=不存在，给其他模块调用）
int CheckDepartmentExist(const char* deptId);

// ==================== A同学测试演示用函数声明（仅自身测试使用，不对外强制暴露） ====================
void AddDepartment(char* deptId, char* deptName, char* wardType);
void PrintAllDepartment();
void SaveDepartmentToFile(char* fileName);
int LoadDepartmentFromFile(char* fileName);
void FreeDepartmentList();
// 按名称模糊查询科室并打印
void QueryDepartmentByName(const char* name);
// 按科室完整名称获取科室ID，找不到返回NULL（挂号流程专用，不改动原有逻辑）
char* GetDepartmentIdByName(const char* deptName);
// 按科室名称关键词模糊查询，统一格式打印，返回匹配数量，唯一匹配时输出科室ID到outDeptId
int QueryDepartmentByNameFuzzy(const char* nameKeyword, char* outDeptId, int outIdLen);
// 打印所有科室简略信息（ID+名称）
void PrintAllDepartmentBrief(void);

#endif
