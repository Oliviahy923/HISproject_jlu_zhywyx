// ==================== 先包含系统头文件（必须放在最开头！！！）====================
#include <stdarg.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <conio.h> // 新增，用于_getch()实现键盘原地编辑
#include <ctype.h>

// ==================== 【唯一通用定义源】包含 common.h（绝不修改 common.h，完全复用其定义）====================
#include "common.h"

// ==================== 再包含自定义业务头文件（按依赖顺序排序）====================
#include "system.h"
#include "log.h"
#include "safe_utils.h"
#include "department.h"
#include "doctor.h"
#include "patient.h"
#include "record.h"
#include "medicine.h"
#include "prescription.h"
#include "bed.h"
#include "registration.h"

// 【全局通用】彻底清空输入缓冲区，解决所有换行符残留问题
static void ClearInputBuffer(void) {
	int ch;
	while ((ch = getchar()) != '\n' && ch != EOF);
}

// ==================== 【仅 system.c 独有、common.h 没有的补充宏定义】====================
// UI界面尺寸宏（common.h 中已定义基础尺寸，此处为扩展专用）
#define BOX_WIDTH_120       120   // 处方专用宽框宽度
#define SCREEN_WIDTH_120    140   // 宽框对应的屏幕总宽度
#define MAX_MEDICINE_NUM    20    // 处方最大药品数量
#define MAX_INPUT_LEN 256
#define KEY_LEFT 75
#define KEY_RIGHT 77
#define KEY_BACKSPACE 8
#define KEY_ENTER 13
#define KEY_ESC 27

// ==================== 静态函数前置声明（仅保留1遍完整无截断的声明）====================
// 核心系统函数
static int UserLogin(void);
static void ShowMenuByRole(void);
static void PrintToBoth(FILE* fp, const char* format, ...);

// 业务查询函数
static void QueryAggregateByDept(void);
static void PatientInfoSubMenu(void);

// 全流程诊疗业务函数
static char* GeneratePatientId(void);
static void PatientRegisterInteractive(void);
static void DoctorReceivePatient(void);
static int WriteMedicalRecord(PatientNode* patient, const char* doctorId, char* recordIdOut);
static int AddPrescriptionAfterConsult(PatientNode* patient, const char* doctorId);
static void print_screen_center_120(void);
static void print_title_box_120(const char* title);
static void print_table_sep_120(const int* cols, int n_cols);
static void print_table_row_120(const int* cols, const AlignType* aligns, const char** data, int n_cols);
static int PatientViewUnpaidPrescriptions(void);
static void PatientConfirmPayment(void);
static void DoctorWriteMedicalRecord(void);
static void AdminAddDoctor(void);

// 异常退出自动保存函数
static void AutoSaveBeforeExit(void);
static void SignalHandler(int signalNum);

// 打印辅助函数（static为system.c独有，全局函数复用common.h声明）
static void print_content_box_centered(const char** lines, int line_count);
static void print_status_box_highlight(const char* type, const char* msg);
static void print_menu_sep(const char* title);


// ==================== 【system.c 专用状态提示宏】====================
// 带错误标记的专用宏，避免与common.h全局宏重定义
#define SYS_PRINT_ERR(msg)  do { print_status_box("【操作失败】", msg); g_initHasError = 1; } while(0)
#define SYS_PRINT_WARN(msg) do { print_status_box("【警告】", msg); g_initHasError = 1; } while(0)
// 高亮成功提示（独有）
#define PRINT_OK_HIGHLIGHT(msg) print_status_box_highlight("【操作成功】", msg)

// ==================== 全局变量（唯一一份，无重复定义）====================
static int g_initHasError = 0;
static char currentOperator[MAX_NAME_LEN] = { 0 };
static char currentOperatorName[MAX_NAME_LEN] = { 0 };
static SystemRole currentRole = 0;
static int systemRunning = 0;
static int g_isExiting = 0;

// ==================== 对外接口函数 ====================
SystemRole GetCurrentRole(void) {
	return currentRole;
}

const char* GetCurrentOperator(void) {
	return currentOperator;
}

// ==================== system.c独有打印辅助函数 ====================
static void print_content_box_centered(const char** lines, int line_count) {
	print_screen_center();
	printf("+");
	for (int i = 0; i < BOX_WIDTH - 2; i++) printf("=");
	printf("+\n");

	for (int i = 0; i < line_count; i++) {
		print_screen_center();
		printf("|");
		print_centered(lines[i], BOX_WIDTH - 2);
		printf("|\n");
	}

	print_screen_center();
	printf("+");
	for (int i = 0; i < BOX_WIDTH - 2; i++) printf("=");
	printf("+\n");
}

static void print_status_box_highlight(const char* type, const char* msg) {
	const char* lines[2] = { type, msg };
	print_content_box_centered(lines, 2);
}

static void print_menu_sep(const char* title) {
	int title_len = (int)strlen(title);
	int sep_len = (BOX_WIDTH - 2 - title_len - 4) / 2;
	print_screen_center();
	printf("+");
	for (int i = 0; i < sep_len; i++) printf("-");
	printf("  %s  ", title);
	for (int i = 0; i < sep_len; i++) printf("-");
	if ((sep_len * 2 + title_len + 4) < (BOX_WIDTH - 2)) printf("-");
	printf("+\n");
}

// ==================== 处方专用120宽度打印辅助函数 ====================
static void print_screen_center_120(void) {
	int pad = (SCREEN_WIDTH_120 - BOX_WIDTH_120) / 2;
	for (int i = 0; i < pad; i++) printf(" ");
}

static void print_title_box_120(const char* title) {
	print_screen_center_120();
	printf("+");
	for (int i = 0; i < BOX_WIDTH_120 - 2; i++) printf("-");
	printf("+\n");

	print_screen_center_120();
	printf("|");
	int len = (int)strlen(title);
	int left = (BOX_WIDTH_120 - 2 - len) / 2;
	int right = BOX_WIDTH_120 - 2 - len - left;
	printf("%*s%s%*s", left, "", title, right, "");
	printf("|\n");

	print_screen_center_120();
	printf("+");
	for (int i = 0; i < BOX_WIDTH_120 - 2; i++) printf("-");
	printf("+\n");
}

static void print_table_sep_120(const int* cols, int n_cols) {
	print_screen_center_120();
	printf("+");
	for (int i = 0; i < n_cols; i++) {
		for (int j = 0; j < cols[i]; j++) printf("-");
		printf("+");
	}
	printf("\n");
}

static void print_table_row_120(const int* cols, const AlignType* aligns, const char** data, int n_cols) {
	print_screen_center_120();
	printf("|");
	for (int i = 0; i < n_cols; i++) {
		int width = cols[i] - 2;
		const char* str = data[i] ? data[i] : "";
		int len = (int)strlen(str);

		if (aligns && aligns[i] == ALIGN_CENTER) {
			int left = (width - len) / 2;
			int right = width - len - left;
			printf(" %*s%s%*s |", left, "", str, right, "");
		}
		else if (aligns && aligns[i] == ALIGN_RIGHT) {
			printf(" %*s |", width, str);
		}
		else {
			printf(" %-*s |", width, str);
		}
	}
	printf("\n");
}

// ==================== 内部私有辅助函数 ====================
static char* GeneratePatientId(void) {
	static char newId[MAX_ID_LEN] = { 0 };
	srand((unsigned int)time(NULL));
	snprintf(newId, MAX_ID_LEN, "P%06d", rand() % 1000000);
	return newId;
}


static void PrintToBoth(FILE* fp, const char* format, ...) {
	va_list args_screen, args_file;
	va_start(args_screen, format);
	vprintf(format, args_screen);
	va_end(args_screen);

	if (fp != NULL) {
		va_start(args_file, format);
		vfprintf(fp, format, args_file);
		va_end(args_file);
	}
}

// ==================== 程序异常/正常退出自动保护机制 ====================
static BOOL WINAPI ConsoleHandlerRoutine(DWORD fdwCtrlType);

static void AutoSaveBeforeExit(void) {
	if (g_isExiting) return;
	g_isExiting = 1;

	if (systemRunning) {
		printf("\n");
		print_screen_center();
		printf("!!! 系统保护：正在自动保存所有数据，请稍候... !!!\n");
		SystemSave();
	}
}

static BOOL WINAPI ConsoleHandlerRoutine(DWORD fdwCtrlType) {
	if (fdwCtrlType == CTRL_CLOSE_EVENT || fdwCtrlType == CTRL_SHUTDOWN_EVENT) {
		printf("\n\n[系统保护] 检测到窗口关闭，正在紧急保存数据...\n");
		AutoSaveBeforeExit();
		Sleep(1000);
		ExitProcess(0);
		return TRUE;
	}
	return FALSE;
}

static void SignalHandler(int signalNum) {
	const char* signalName = "未知信号";
	switch (signalNum) {
	case SIGINT: signalName = "Ctrl+C"; break;
	case SIGTERM: signalName = "终止指令"; break;
	case SIGBREAK: signalName = "Ctrl+Break"; break;
	default: signalName = "程序关闭"; break;
	}

	printf("\n\n[系统保护] 捕获到终止信号：%s，正在执行安全退出...\n", signalName);
	AutoSaveBeforeExit();
	exit(signalNum);
}

// ==================== 用户登录与权限校验 ====================
static int UserLogin(void) {
	while (1) {
		CLEAR_SCREEN;
		printf("\n");
		print_title_box("医院信息管理系统 (HIS) 登录");
		printf("\n");

		const char* roleLines[6] = {
			"系统角色说明：",
			"1 - 管理员   (全系统权限)",
			"2 - 医生     (就诊、处方、患者管理)",
			"3 - 药房     (药品、发药管理)",
			"4 - 患者     (个人信息查询、挂号缴费)",
			"0 - 退出系统"
		};
		print_content_box(roleLines, 6);
		printf("\n");

		int roleInput = 0;
		SafeIntInput("请选择您的角色 (输入数字 0-4)", &roleInput, 0, 4);

		if (roleInput == 0) {
			PRINT_TIP("您已选择退出系统。");
			WaitEnter();
			return 0;
		}
		currentRole = (SystemRole)roleInput;

		char input[MAX_NAME_LEN] = { 0 };
		printf("\n");
		SafeStrInput("请输入您的姓名/编号 (输入 q 可返回上一级)", input, MAX_NAME_LEN);

		if (strcmp(input, "q") == 0 || strcmp(input, "Q") == 0) {
			PRINT_TIP("已返回角色选择界面。");
			WaitEnter();
			continue;
		}

		int loginSuccess = 0;
		memset(currentOperator, 0, sizeof(currentOperator));
		memset(currentOperatorName, 0, sizeof(currentOperatorName));

		if (currentRole == ROLE_ADMIN) {
			if (strcmp(input, "admin") == 0) {
				char pwd[50] = { 0 };
				SafeStrInput("请输入管理员密码", pwd, 50);
				if (strcmp(pwd, "123456") == 0) {
					SafeStrCopy(currentOperator, input, MAX_NAME_LEN);
					SafeStrCopy(currentOperatorName, "系统管理员", MAX_NAME_LEN);
					loginSuccess = 1;
				}
				else {
					SYS_PRINT_ERR("密码错误！");
				}
			}
			else {
				SYS_PRINT_ERR("管理员账号错误！");
			}
		}
		else if (currentRole == ROLE_DOCTOR) {
			char* doctorId = NULL;
			if (CheckDoctorExist(input)) {
				doctorId = input;
			}
			else if (CheckDoctorExistByName(input)) {
				doctorId = GetDoctorIdByName(input);
			}
			if (doctorId != NULL) {
				SafeStrCopy(currentOperator, doctorId, MAX_ID_LEN);
				DoctorList doc = GetDoctorById(doctorId);
				if (doc != NULL) {
					SafeStrCopy(currentOperatorName, doc->name, MAX_NAME_LEN);
				}
				else {
					SafeStrCopy(currentOperatorName, "未知医生", MAX_NAME_LEN);
				}
				PRINT_OK("医生登录成功！");
				loginSuccess = 1;
			}
			else {
				SYS_PRINT_ERR("医生不存在！");
			}
		}
		else if (currentRole == ROLE_PATIENT) {
			char* realId = GetPatientRealId(input);
			if (realId == NULL) {
				PRINT_TIP("患者不存在，正在为您创建新账户，请完善个人信息...");

				PatientNode newPatient = { 0 };
				SafeStrCopy(newPatient.patientId, GeneratePatientId(), MAX_ID_LEN);
				SafeStrCopy(newPatient.name, input, MAX_NAME_LEN);

				printf("\n");
				print_title_box("完善个人信息");
				// 替换原有的性别输入行
				SafeStrInput("请输入性别 (男/女/其他)", newPatient.gender, MAX_NAME_LEN);
				// 替换为以下带校验的循环代码
				while (1) {
					SafeStrInput("请输入性别（仅允许输入：男/女）", newPatient.gender, MAX_NAME_LEN);
					if (strcmp(newPatient.gender, "男") == 0 || strcmp(newPatient.gender, "女") == 0) {
						break;
					}
					SYS_PRINT_WARN("性别输入不合法！仅允许输入「男」或「女」，请重新输入！");
				}                SafeIntInput("请输入年龄", &newPatient.age, 0, 150);
				SafeStrInput("请输入联系电话", newPatient.phone, MAX_DATA_LEN);

				newPatient.patientType = 1;
				memset(newPatient.bedId, 0, sizeof(newPatient.bedId));

				if (AddPatient(newPatient)) {
					char msg[80];
					snprintf(msg, sizeof(msg), "新患者创建成功！您的患者ID为：%s", newPatient.patientId);
					PRINT_OK(msg);
					SafeStrCopy(currentOperator, newPatient.patientId, MAX_ID_LEN);
					SafeStrCopy(currentOperatorName, newPatient.name, MAX_NAME_LEN);
					loginSuccess = 1;
				}
				else {
					SYS_PRINT_ERR("新患者创建失败！");
				}
			}
			else {
				SafeStrCopy(currentOperator, realId, MAX_ID_LEN);
				PatientNode* pat = FindPatientById(realId);
				if (pat != NULL) {
					SafeStrCopy(currentOperatorName, pat->name, MAX_NAME_LEN);
				}
				else {
					SafeStrCopy(currentOperatorName, "未知患者", MAX_NAME_LEN);
				}
				loginSuccess = 1;
			}
		}
		else if (currentRole == ROLE_PHARMACY) {
			SafeStrCopy(currentOperator, input, MAX_NAME_LEN);
			SafeStrCopy(currentOperatorName, "药房管理员", MAX_NAME_LEN);
			PRINT_OK("药房登录成功！");
			loginSuccess = 1;
		}

		if (loginSuccess) {
			char roleName[20] = { 0 };
			switch (currentRole) {
			case ROLE_ADMIN:    strcpy(roleName, "管理员(1)"); break;
			case ROLE_DOCTOR:   strcpy(roleName, "医生(2)");    break;
			case ROLE_PHARMACY: strcpy(roleName, "药房(3)");    break;
			case ROLE_PATIENT:  strcpy(roleName, "患者(4)");    break;
			default:            strcpy(roleName, "未知");       break;
			}

			char logContent[MAX_DETAIL_LEN] = { 0 };
			snprintf(logContent, MAX_DETAIL_LEN, "用户：%s(%s)，角色：%s，登录成功", currentOperatorName, currentOperator, roleName);
			WriteLog(LOG_LEVEL_INFO, "系统", "用户登录", logContent);

			return 1;
		}
		else {
			WaitEnter();
			continue;
		}
	}
}

// ==================== 按角色显示菜单 ====================
static void ShowMenuByRole(void) {
	printf("\n");
	char title[120];
	snprintf(title, sizeof(title), "医院HIS系统主菜单（当前用户: %s(%s)）", currentOperatorName, currentOperator);
	print_title_box(title);

	print_screen_center();
	printf("| %-76s |\n", "0 - 退出系统（自动保存数据）");
	print_screen_center();
	printf("| %-76s |\n", "99 - 切换用户/重新登录");

	if (currentRole == ROLE_DOCTOR) {
		print_menu_sep("患者管理");
		print_screen_center(); printf("| %-76s |\n", "1 - 患者信息查询（按姓名模糊查询）");
		print_screen_center(); printf("| %-76s |\n", "2 - 我的接诊记录查询");

		print_menu_sep("处方管理");
		print_screen_center(); printf("| %-76s |\n", "3 - 修改处方信息");
		print_screen_center(); printf("| %-76s |\n", "4 - 作废处方（含冲账回退）");
		print_screen_center(); printf("| %-76s |\n", "5 - 查看所有处方");

		print_menu_sep("接诊业务");
		print_screen_center(); printf("| %-76s |\n", "6 - 个人接诊量统计");
		print_screen_center(); printf("| %-76s |\n", "7 - 接诊待诊患者（含病历+处方）");
		print_screen_center(); printf("| %-76s |\n", "8 - 住院管理（登记/入院/出院）");
	}
	else if (currentRole == ROLE_ADMIN) {
		print_menu_sep("基础数据管理");
		print_screen_center(); printf("| %-76s |\n", "1 - 科室信息管理");
		print_screen_center(); printf("| %-76s |\n", "2 - 医生信息管理");
		print_screen_center(); printf("| %-76s |\n", "3 - 新增医生");
		print_screen_center(); printf("| %-76s |\n", "4 - 患者信息管理");
		print_screen_center(); printf("| %-76s |\n", "5 - 药品信息管理");
		print_screen_center(); printf("| %-76s |\n", "6 - 床位信息管理");
		print_menu_sep("业务单据管理");
		print_screen_center(); printf("| %-76s |\n", "7 - 医疗记录管理");
		print_screen_center(); printf("| %-76s |\n", "8 - 处方记录管理");
		print_menu_sep("综合统计查询");
		print_screen_center(); printf("| %-76s |\n", "9 - 经营数据统计 (营业额/盈利)");
		print_screen_center(); printf("| %-76s |\n", "10 - 药品库存报表");
		print_screen_center(); printf("| %-76s |\n", "11 - 床位使用报表");
		print_screen_center(); printf("| %-76s |\n", "12 - 药品消耗TOP10");
		print_screen_center(); printf("| %-76s |\n", "13 - 全院床位使用率");
		print_screen_center(); printf("| %-76s |\n", "14 - 科室患者分布统计");
		print_screen_center(); printf("| %-76s |\n", "15 - 科室聚合查询 (医生/床位/药品)");
		print_menu_sep("系统数据管理");
		print_screen_center(); printf("| %-76s |\n", "16 - 手动保存所有数据");
		print_screen_center(); printf("| %-76s |\n", "17 - 导出所有报表到文件");
		print_screen_center(); printf("| %-76s |\n", "18 - 查看系统操作日志");
		print_screen_center(); printf("| %-76s |\n", "19 - 清空系统操作日志");
	}
	else if (currentRole == ROLE_PHARMACY) {
		print_menu_sep("药品管理");
		print_screen_center(); printf("| %-76s |\n", "1 - 药品信息查询 (按名称模糊查询)");
		print_screen_center(); printf("| %-76s |\n", "2 - 药品入库操作");
		print_screen_center(); printf("| %-76s |\n", "3 - 查看所有药品信息");
		print_menu_sep("处方发药");
		print_screen_center(); printf("| %-76s |\n", "4 - 处方发药操作");
		print_screen_center(); printf("| %-76s |\n", "5 - 查看所有处方记录");
		print_menu_sep("统计报表");
		print_screen_center(); printf("| %-76s |\n", "6 - 药品库存报表");
		print_screen_center(); printf("| %-76s |\n", "7 - 药品消耗TOP10统计");
		print_screen_center(); printf("| %-76s |\n", "8 - 经营数据统计 (营业额/盈利)");
	}
	// 【完整可替换的患者角色菜单代码】
	else if (currentRole == ROLE_PATIENT) {
		print_menu_sep("个人信息查询");
		print_screen_center(); printf("| %-76s |\n", "1 - 我的历史医疗记录");
		print_screen_center(); printf("| %-76s |\n", "2 - 我的处方记录明细");
		print_screen_center(); printf("| %-76s |\n", "3 - 我的费用明细统计");
		print_screen_center(); printf("| %-76s |\n", "4 - 修改我的个人信息"); // 新增选项
		print_menu_sep("诊疗业务");
		print_screen_center(); printf("| %-76s |\n", "5 - 门诊挂号");
		print_screen_center(); printf("| %-76s |\n", "6 - 我的待缴费处方");
		print_screen_center(); printf("| %-76s |\n", "7 - 确认缴费");
		print_screen_center(); printf("| %-76s |\n", "8 - 住院押金缴费");
	}
	print_screen_center();
	printf("+");
	for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
	printf("+\n");
}

// ==================== 对外核心系统函数实现 ====================
// ==================== 对外核心系统函数实现 ====================
void SystemInit(void) {
	g_isExiting = 0;
	atexit(AutoSaveBeforeExit);
	signal(SIGINT, SignalHandler);
	signal(SIGBREAK, SignalHandler);
	SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
	g_initHasError = 0;
	printf("\n");
	print_title_box("系统启动中");
	printf("\n");
	InitLog();
	PRINT_OK("日志模块初始化完成！");
	InitDepartmentList();
	PRINT_OK("科室模块初始化完成！");
	InitDoctorList();
	InitPatientList();
	InitRecordList();
	PRINT_OK("就诊业务模块初始化完成！");
	InitMedicineList();
	InitPrescriptionList();
	InitBedList();
	InitRegistrationList();
	PRINT_OK("资源管理与挂号模块初始化完成！");
	PRINT_TIP("开始加载历史数据...");
	// 定义模块名称和加载结果数组
	typedef struct {
		char moduleName[50];
		int loadCount;
	} LoadResult;
	LoadResult loadResults[] = {
		{"科室数据", 0},
		{"医生数据", 0},
		{"患者数据", 0},
		{"医疗记录数据", 0},
		{"药品数据", 0},
		{"处方数据", 0},
		{"床位数据", 0},
		{"挂号记录数据", 0}
	};
	int moduleCount = sizeof(loadResults) / sizeof(loadResults[0]);
	// 执行加载，记录条数
	loadResults[0].loadCount = LoadDepartmentFromFile(DEPT_FILE);
	loadResults[1].loadCount = LoadDoctorFromFile(DOCTOR_FILE);
	loadResults[2].loadCount = LoadPatientFromFile(PATIENT_FILE);
	loadResults[3].loadCount = LoadRecordFromFile(RECORD_FILE);
	loadResults[4].loadCount = LoadMedicineFromFile(MEDICINE_FILE);
	loadResults[5].loadCount = LoadPrescriptionFromFile(PRESC_FILE);
	loadResults[6].loadCount = LoadBedFromFile(BED_FILE);
	loadResults[7].loadCount = LoadRegistrationFromFile(REG_FILE);

	// ==================== 修改点1：加载完所有模块后停留2秒 ====================
	Sleep(2000);

	// 统计空载入的模块
	int emptyModuleCount = 0;
	char emptyModuleList[10][50] = { 0 };
	for (int i = 0; i < moduleCount; i++) {
		if (loadResults[i].loadCount == 0) {
			SafeStrCopy(emptyModuleList[emptyModuleCount], loadResults[i].moduleName, 50);
			emptyModuleCount++;
		}
	}
	systemRunning = 1;
	WriteLog(LOG_LEVEL_INFO, "系统", "系统初始化", "所有模块初始化完成，数据加载完成");
	// 清屏，处理空载入提示
	CLEAR_SCREEN;
	// 有模块空载入
	if (emptyModuleCount > 0) {
		print_title_box("系统数据加载提示");
		printf("\n");
		PRINT_WARN("以下模块未加载到任何历史数据（空载入）：");
		// ==================== 修改点2：空载入模块用 [1][2] 带框编号 ====================
		for (int i = 0; i < emptyModuleCount; i++) {
			printf("  [%d] %s\n", i + 1, emptyModuleList[i]);
		}
		printf("\n");
		PRINT_TIP("空载入不影响系统使用，首次启动会出现此提示，数据将在使用中自动生成。");
		printf("\n");
		// 让用户确认是否继续登录
		int confirmContinue = 0;
		SafeIntInput("是否继续登录系统？输入1=继续，输入0=退出系统", &confirmContinue, 0, 1);
		if (confirmContinue == 0) {
			PRINT_TIP("用户选择退出系统，正在执行安全退出...");
			SystemExit();
			return;
		}
		// 确认继续，清屏
		CLEAR_SCREEN;
		PRINT_OK_HIGHLIGHT("系统启动完成，所有模块初始化成功！欢迎使用HIS系统！");
		printf("\n");
		Sleep(1500);
		CLEAR_SCREEN;
	}
	// 无空载入，正常启动
	else {
		if (g_initHasError == 0) {
			PRINT_OK_HIGHLIGHT("系统启动完成，所有模块初始化成功！欢迎使用HIS系统！");
			printf("\n");
			Sleep(2000);
			CLEAR_SCREEN;
		}
		else {
			SYS_PRINT_WARN("系统启动完成，但部分模块加载出现问题，请查看历史提示！");
			printf("\n");
			Sleep(2000);
			CLEAR_SCREEN;
		}
	}
}

void SystemSave(void) {
	if (!systemRunning) {
		SYS_PRINT_ERR("错误：系统未初始化，无法保存数据！");
		return;
	}
	PRINT_TIP("开始保存所有数据...");

	SaveDepartmentToFile(DEPT_FILE);
	SaveDoctorTofile(DOCTOR_FILE);
	SavePatientToFile(PATIENT_FILE);
	SaveRecordToFile(RECORD_FILE);
	SaveMedicineToFile(MEDICINE_FILE);
	SavePrescriptionToFile(PRESC_FILE);
	SaveBedToFile(BED_FILE);
	SaveRegistrationToFile(REG_FILE);

	WriteLog(LOG_LEVEL_INFO, currentOperator, "手动保存数据", "所有数据保存成功");
	PRINT_OK("所有数据保存完成！");
}

void SystemExit(void) {
	if (g_isExiting) return;

	printf("\n");
	print_title_box("系统退出");
	printf("\n");

	int confirm = 0;
	SafeIntInput("警告：退出系统将自动保存所有数据。确认退出？输入1确认，输入0取消", &confirm, 0, 1);
	if (confirm == 0) {
		PRINT_TIP("已取消退出操作！");
		return;
	}

	g_isExiting = 1;
	SystemSave();

	FreeRegistrationList();
	FreeBedList();
	FreePrescriptionList();
	FreeMedicineList();
	FreeRecordList();
	FreePatientList();
	FreeDoctorList();
	FreeDepartmentList();

	WriteLog(LOG_LEVEL_INFO, currentOperator, "系统退出", "系统安全退出，所有内存已释放");
	systemRunning = 0;

	PRINT_OK("系统已安全退出！");
	WaitEnter();
	exit(EXIT_SUCCESS);
}

void CalcBusinessProfit(void) {
	if (currentRole != ROLE_ADMIN && currentRole != ROLE_PHARMACY) {
		SYS_PRINT_ERR("操作失败：权限不足！");
		WriteLog(LOG_LEVEL_ERROR, currentOperator, "经营数据统计", "失败，权限不足");
		return;
	}

	print_title_box("医院经营数据统计");
	float totalRecordFee = GetTotalRecordFee();
	float totalPrescFee = GetTotalPrescriptionFee();
	float totalPurchaseCost = GetMedicineTotalSoldPurchaseCost();
	float totalSaleIncome = GetMedicineTotalSoldSaleIncome();
	float totalTurnover = totalRecordFee + totalPrescFee;
	float totalProfit = totalSaleIncome - totalPurchaseCost;

	char stat1[80], stat2[80], stat3[80], stat4[80];
	snprintf(stat1, sizeof(stat1), "总营业额（医疗+处方）：%.2f元", totalTurnover);
	snprintf(stat2, sizeof(stat2), "药品总销售收入：%.2f元", totalSaleIncome);
	snprintf(stat3, sizeof(stat3), "药品总采购成本：%.2f元", totalPurchaseCost);
	snprintf(stat4, sizeof(stat4), "总盈利额：%.2f元", totalProfit);
	const char* lines[5] = { "医院经营数据统计", stat1, stat2, stat3, stat4 };
	print_content_box(lines, 5);

	WriteLog(LOG_LEVEL_INFO, currentOperator, "经营数据统计", "成功");
}

void ExportAllReportToFile(void) {
	if (currentRole != ROLE_ADMIN) {
		SYS_PRINT_ERR("操作失败：仅管理员有权限导出报表！");
		WriteLog(LOG_LEVEL_ERROR, currentOperator, "导出报表", "失败，权限不足");
		return;
	}

	FILE* fp = fopen(REPORT_FILE, "w");
	if (fp == NULL) {
		SYS_PRINT_ERR("操作失败：无法打开报表文件！");
		WriteLog(LOG_LEVEL_ERROR, currentOperator, "导出报表", "失败，文件打开失败");
		return;
	}

	PRINT_TIP("开始导出报表...");
	fprintf(fp, "==================== 医院HIS系统全量统计报表 ====================\n");
	fprintf(fp, "报表生成时间：%s\n", GetCurrentFullTimeStr());
	fprintf(fp, "操作人：%s\n", currentOperator);
	fprintf(fp, "\n");
	PRINT_OK("已写入报表头部！");

	fprintf(fp, "\n");
	GenerateMedicineStockReport(fp);
	PRINT_OK("已导出药品库存报表！");

	fprintf(fp, "\n");
	GenerateBedUsageReport(fp);
	PRINT_OK("已导出床位使用报表！");

	fprintf(fp, "\n");
	StatMedicineConsumeTop10(fp);
	PRINT_OK("已导出药品消耗TOP10！");

	fprintf(fp, "\n");
	StatHospitalBedUsageRate(fp);
	PRINT_OK("已导出全院床位使用率！");

	fclose(fp);
	WriteLog(LOG_LEVEL_INFO, currentOperator, "导出报表", "所有报表导出成功");
	printf("\n");
	char successMsg[100];
	snprintf(successMsg, sizeof(successMsg), "所有报表已成功导出到文件：%s", REPORT_FILE);
	PRINT_OK(successMsg);
}

static void QueryAggregateByDept(void) {
	char deptId[MAX_ID_LEN] = { 0 };
	print_title_box("科室聚合查询");
	SafeStrInput("请输入科室编号", deptId, MAX_ID_LEN);

	if (!CheckDepartmentExist(deptId)) {
		char msg[80];
		snprintf(msg, sizeof(msg), "错误：科室编号【%s】不存在！", deptId);
		SYS_PRINT_ERR(msg);
		WaitEnter();
		return;
	}

	CLEAR_SCREEN;
	char title[100] = { 0 };
	snprintf(title, sizeof(title), "科室【%s】综合信息", deptId);
	print_title_box(title);

	printf("\n");
	PRINT_TIP("1. 该科室在岗医生");
	QueryDoctorByDept(deptId);

	printf("\n");
	PRINT_TIP("2. 该科室空闲床位");
	QueryFreeBedByDept(deptId);

	printf("\n");
	PRINT_TIP("3. 该科室专科药品");
	PrintMedicineByDeptId(deptId);

}

static void PatientInfoSubMenu(void) {
	int subChoice = 0;
	while (1) {
		CLEAR_SCREEN;
		printf("\n");
		print_title_box("患者信息管理子菜单");

		print_screen_center(); printf("| %-76s |\n", "1 - 新增患者信息");
		print_screen_center(); printf("| %-76s |\n", "2 - 查看所有患者");
		print_screen_center(); printf("| %-76s |\n", "3 - 按ID查询患者");
		print_screen_center(); printf("| %-76s |\n", "4 - 按姓名模糊查询患者");
		print_screen_center(); printf("| %-76s |\n", "5 - 删除患者信息");
		print_screen_center(); printf("| %-76s |\n", "0 - 返回主菜单");

		print_screen_center();
		printf("+");
		for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
		printf("+\n");

		SafeIntInput("请输入子菜单操作编号", &subChoice, 0, 5);
		if (subChoice == 0) break;

		char patientId[MAX_ID_LEN] = { 0 };
		char name[MAX_NAME_LEN] = { 0 };
		switch (subChoice) {
		case 1:
			AddPatientInteractive();
			break;
		case 2:
			PrintAllPatient();
			break;
		case 3:
			SafeStrInput("请输入患者ID", patientId, MAX_ID_LEN);
			QueryPatientById(patientId);
			break;
		case 4:
			SafeStrInput("请输入患者姓名关键词", name, MAX_NAME_LEN);
			QueryPatientByName(name);
			break;
		case 5:
			SafeStrInput("请输入要删除的患者ID", patientId, MAX_ID_LEN);
			DeletePatient(patientId);
			break;
		default:
			SYS_PRINT_WARN("输入无效，请重新选择！");
			break;
		}
		WaitEnter();
	}
}

// ==================== 系统主循环 ====================
void SystemLoop(void) {
	int c;
	while ((c = getchar()) != '\n' && c != EOF);

	while (1) {
		if (!UserLogin()) {
			SystemExit();
			return;
		}

		while (systemRunning) {
			CLEAR_SCREEN;
			ShowMenuByRole();
			int menuChoice = 0;
			SafeIntInput("请输入您要执行的操作编号（请输入0-99之间的整数）：", &menuChoice, 0, 99);

			if (menuChoice == 0) {
				SystemExit();
				continue;
			}

			if (menuChoice == 99) {
				PRINT_TIP("正在退出当前用户，准备重新登录...");
				WaitEnter();
				systemRunning = 0;
				break;
			}

			if (currentRole == ROLE_ADMIN) {
				switch (menuChoice) {
				case 1: PrintAllDepartment(); break;
				case 2: PrintAllDoctor(); break;
				case 3:AdminAddDoctor(); break;
				case 4: PatientInfoSubMenu(); break;
				case 5: PrintAllMedicine(); break;
				case 6: PrintAllBed(); break;
				case 7: PrintAllRecord(); break;
					// 管理员菜单case8替换
				case 8: {
					int subChoice = 0;
					while (1) {
						CLEAR_SCREEN;
						print_title_box("处方记录管理子菜单");
						print_screen_center(); printf("| %-76s |\n", "1 - 查看所有处方");
						print_screen_center(); printf("| %-76s |\n", "2 - 删除处方（仅未缴费）");
						print_screen_center(); printf("| %-76s |\n", "3 - 作废处方（含冲账回退）");
						print_screen_center(); printf("| %-76s |\n", "0 - 返回主菜单");
						print_screen_center();
						printf("+");
						for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
						printf("+\n");
						SafeIntInput("请输入子菜单操作编号", &subChoice, 0, 3);
						if (subChoice == 0) break;
						switch (subChoice) {
						case 1: PrintAllPrescription(); break;
						case 2: DeletePrescription(); break;
						case 3: {
							char prescId[MAX_ID_LEN] = { 0 };
							print_title_box("作废处方");
							PRINT_TIP("现有处方列表");
							PrintAllPrescriptionBrief();
							printf("\n");
							SafeStrInput("请输入要作废的处方编号", prescId, MAX_ID_LEN);
							CancelPrescription(prescId);
							break;
						}
						default: PRINT_WARN("输入无效，请重新选择！"); break;
						}
						WaitEnter();
					}
					break;
				}
				case 9: CalcBusinessProfit(); break;
				case 10: GenerateMedicineStockReport(NULL); break;
				case 11: GenerateBedUsageReport(NULL); break;
				case 12: StatMedicineConsumeTop10(NULL); break;
				case 13: StatHospitalBedUsageRate(NULL); break;
				case 14: {
					char deptId[MAX_ID_LEN] = { 0 };
					PRINT_TIP("可选科室列表");
					PrintAllDepartmentBrief();
					printf("\n");
					SafeStrInput("请输入科室编号", deptId, MAX_ID_LEN);
					StatDeptPatientDistribution(deptId);
					break;
				}
				case 15: QueryAggregateByDept(); break;
				case 16: SystemSave(); break;
				case 17: ExportAllReportToFile(); break;
				case 18: PrintAllLog(); break;
				case 19: ClearAllLog(); break;
				default: SYS_PRINT_WARN("输入的操作编号无效，请重新输入！");
					break;
				}
			}
			else if (currentRole == ROLE_DOCTOR) {
				switch (menuChoice) {
				case 1: {
					char name[MAX_NAME_LEN] = { 0 };
					SafeStrInput("请输入要查询的患者姓名", name, MAX_NAME_LEN);
					QueryPatientByName(name);
					break;
				}
				case 2: QueryRecordByDoctorId(currentOperator); break;
				case 3: ModifyPrescription(); break;
				case 4: {
					char prescId[MAX_ID_LEN] = { 0 };
					print_title_box("作废处方");
					PRINT_TIP("现有处方列表");
					PrintAllPrescriptionBrief();
					printf("\n");
					SafeStrInput("请输入要作废的处方编号", prescId, MAX_ID_LEN);
					CancelPrescription(prescId);
					break;
				}
				case 5: PrintAllPrescription(); break;
				case 6: {
					CLEAR_SCREEN;
					print_title_box("医生接诊量统计");
					const char* timeLines[4] = {
						"请选择统计时间段：",
						"1 - 本日",
						"2 - 本周",
						"3 - 本月"
					};
					print_content_box(timeLines, 4);
					int timeChoice = 0;
					char startTime[MAX_DATA_LEN] = { 0 };
					char endTime[MAX_DATA_LEN] = { 0 };
					time_t now = time(NULL);
					struct tm* t = localtime(&now);
					while (1) {
						SafeIntInput("请输入时间段编号（1-3）", &timeChoice, 1, 3);
						break;
					}
					switch (timeChoice) {
					case 1:
						snprintf(startTime, MAX_DATA_LEN, "%02d-%02d 00:00", t->tm_mon + 1, t->tm_mday);
						snprintf(endTime, MAX_DATA_LEN, "%02d-%02d 23:59", t->tm_mon + 1, t->tm_mday);
						break;
					case 2:
						int weekDay = t->tm_wday == 0 ? 7 : t->tm_wday;
						int startDay = t->tm_mday - weekDay + 1;
						int endDay = t->tm_mday + (7 - weekDay);
						snprintf(startTime, MAX_DATA_LEN, "%02d-%02d 00:00", t->tm_mon + 1, startDay > 0 ? startDay : 1);
						snprintf(endTime, MAX_DATA_LEN, "%02d-%02d 23:59", t->tm_mon + 1, endDay);
						break;
					case 3:
						snprintf(startTime, MAX_DATA_LEN, "%02d-01 00:00", t->tm_mon + 1);
						snprintf(endTime, MAX_DATA_LEN, "%02d-31 23:59", t->tm_mon + 1);
						break;
					}
					StatDoctorVisitCount(currentOperator, startTime, endTime);
					char msg[80];
					snprintf(msg, sizeof(msg), "统计范围：%s 至 %s", startTime, endTime);
					PRINT_TIP(msg);
					break;
				}
				case 7: DoctorReceivePatient(); break;
				case 8: {
					int subChoice = 0;
					while (1) {
						CLEAR_SCREEN;
						print_title_box("住院管理子菜单");
						print_screen_center(); printf("| %-76s |\n", "1 - 患者住院登记（含床位智能匹配）");
						print_screen_center(); printf("| %-76s |\n", "2 - 办理患者入院手续");
						print_screen_center(); printf("| %-76s |\n", "3 - 患者出院结算");
						print_screen_center(); printf("| %-76s |\n", "0 - 返回主菜单");
						print_screen_center();
						printf("+");
						for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
						printf("+\n");
						SafeIntInput("请输入子菜单操作编号", &subChoice, 0, 3);
						if (subChoice == 0) break;
						char patientId[MAX_ID_LEN] = { 0 };
						char deptId[MAX_ID_LEN] = { 0 };
						switch (subChoice) {
						case 1: {
							PRINT_TIP("可选患者列表");
							PrintAllPatientBrief();
							printf("\n");
							SafeStrInput("请输入患者ID", patientId, MAX_ID_LEN);
							PRINT_TIP("可选科室列表");
							PrintAllDepartmentBrief();
							printf("\n");
							SafeStrInput("请输入就诊科室ID", deptId, MAX_ID_LEN);
							InHospitalRegister(patientId, deptId, GetCurrentOperator());
							break;
						}
						case 2: {
							PRINT_TIP("待入院患者列表");
							PrintAllPatientBrief();
							printf("\n");
							SafeStrInput("请输入患者ID", patientId, MAX_ID_LEN);
							HandleInHospitalCheckIn(patientId);
							break;
						}
						case 3: {
							PRINT_TIP("住院中患者列表");
							PrintAllPatientBrief();
							printf("\n");
							SafeStrInput("请输入患者ID", patientId, MAX_ID_LEN);
							char feeStr[MAX_DATA_LEN] = { 0 };
							float settleFee = 0.0f;
							while (1) {
								SafeStrInput("请输入本次结算总费用", feeStr, MAX_DATA_LEN);
								settleFee = (float)atof(feeStr);
								if (!CheckFeeValid(settleFee)) {
									PRINT_WARN("金额格式错误，请重新输入！");
									continue;
								}
								break;
							}
							HandleHospitalDischarge(patientId, settleFee);
							break;
						}
						default: PRINT_WARN("输入无效，请重新选择！"); break;
						}
						WaitEnter();
					}
					break;
				}
				default: SYS_PRINT_WARN("输入的操作编号无效，请重新输入！");
					break;
				}
			}
			else if (currentRole == ROLE_PHARMACY) {
				switch (menuChoice) {
				case 1: QueryMedicineByName(); break;
				case 2: MedicineStockIn(); break;
				case 3: PrintAllMedicine(); break;
				case 4: DispensePrescription(); break;
				case 5: PrintAllPrescription(); break;
				case 6: GenerateMedicineStockReport(NULL); break;
				case 7: StatMedicineConsumeTop10(NULL); break;
				case 8: CalcBusinessProfit(); break;
				default: SYS_PRINT_WARN("输入的操作编号无效，请重新输入！"); break;
				}
			}
			// 【完整可替换的患者角色case分支代码】
			else if (currentRole == ROLE_PATIENT) {
				switch (menuChoice) {
				case 1: QueryRecordByPatientId(currentOperator); break;
				case 2: QueryPrescriptionByPatientId(currentOperator); break;
				case 3: StatPatientFeeDetail(currentOperator); break;
				case 4: ModifyPatientSelfInfo(currentOperator); break; // 新增选项对应的函数调用
				case 5: PatientRegisterInteractive(); break;
				case 6: PatientViewUnpaidPrescriptions(); break;
				case 7: PatientConfirmPayment(); break;
				case 8: PayHospitalDeposit(currentOperator); break;
				default: SYS_PRINT_WARN("输入的操作编号无效，请重新输入！"); break;
				}
			}            WaitEnter();
		}

		systemRunning = 1;
		memset(currentOperator, 0, sizeof(currentOperator));
		currentRole = 0;
	}
}

// ==================== 患者自助挂号 ====================
static void PatientRegisterInteractive(void) {
	CLEAR_SCREEN;
	print_title_box("门诊挂号");

	char* realPatientId = GetPatientRealId((char*)currentOperator);
	if (realPatientId == NULL) {
		SYS_PRINT_ERR("无法获取您的患者信息，请重新登录！");
		WaitEnter();
		return;
	}
	char userInfo[80];
	snprintf(userInfo, sizeof(userInfo), "当前患者：%s(ID:%s)", currentOperatorName, realPatientId);
	PRINT_TIP(userInfo);
	printf("\n");

	PRINT_TIP("【第一步：选择就诊科室】");
	PrintAllDepartmentBrief();
	printf("\n");

	char deptKeyword[MAX_NAME_LEN] = { 0 };
	char targetDeptId[MAX_ID_LEN] = { 0 };
	int deptMatchCount = 0;

	while (1) {
		memset(deptKeyword, 0, MAX_NAME_LEN);
		memset(targetDeptId, 0, MAX_ID_LEN);
		SafeStrInput("请输入科室名称关键词（支持模糊查询，输入q返回上一级）", deptKeyword, MAX_NAME_LEN);

		if (strcmp(deptKeyword, "q") == 0 || strcmp(deptKeyword, "Q") == 0) {
			PRINT_TIP("已取消挂号，返回上一级！");
			WaitEnter();
			return;
		}

		deptMatchCount = QueryDepartmentByNameFuzzy(deptKeyword, targetDeptId, MAX_ID_LEN);
		printf("\n");

		if (deptMatchCount == 0) {
			SYS_PRINT_ERR("未找到匹配的科室，请重新输入关键词！");
			continue;
		}
		else if (deptMatchCount == 1) {
			char successMsg[80];
			snprintf(successMsg, sizeof(successMsg), "已自动匹配科室：%s", deptKeyword);
			PRINT_OK(successMsg);
			break;
		}
		else {
			char confirmDeptName[MAX_NAME_LEN] = { 0 };
			SafeStrInput("匹配到多个科室，请输入目标科室的完整名称（输入q重新查询）", confirmDeptName, MAX_NAME_LEN);

			if (strcmp(confirmDeptName, "q") == 0 || strcmp(confirmDeptName, "Q") == 0) {
				continue;
			}

			char* confirmDeptId = GetDepartmentIdByName(confirmDeptName);
			if (confirmDeptId == NULL) {
				SYS_PRINT_ERR("科室名称输入错误，请重新操作！");
				continue;
			}

			SafeStrCopy(targetDeptId, confirmDeptId, MAX_ID_LEN);
			char successMsg[80];
			snprintf(successMsg, sizeof(successMsg), "已选择科室：%s", confirmDeptName);
			PRINT_OK(successMsg);
			break;
		}
	}
	printf("\n");

	PRINT_TIP("【第二步：选择就诊医生】");
	PRINT_TIP("当前科室在岗医生列表：");
	QueryDoctorByDept(targetDeptId);
	printf("\n");

	char docKeyword[MAX_NAME_LEN] = { 0 };
	char targetDoctorId[MAX_ID_LEN] = { 0 };
	int docMatchCount = 0;
	DoctorList targetDoctor = NULL;

	while (1) {
		memset(docKeyword, 0, MAX_NAME_LEN);
		memset(targetDoctorId, 0, MAX_ID_LEN);
		SafeStrInput("请输入医生姓名关键词（支持模糊查询，输入q返回上一级）", docKeyword, MAX_NAME_LEN);

		if (strcmp(docKeyword, "q") == 0 || strcmp(docKeyword, "Q") == 0) {
			PRINT_TIP("已取消挂号，返回上一级！");
			WaitEnter();
			return;
		}

		docMatchCount = QueryDoctorByDeptAndName(targetDeptId, docKeyword, targetDoctorId, MAX_ID_LEN);
		printf("\n");

		if (docMatchCount == 0) {
			SYS_PRINT_ERR("当前科室未找到匹配的医生，请重新输入关键词！");
			continue;
		}
		else if (docMatchCount == 1) {
			char successMsg[80];
			snprintf(successMsg, sizeof(successMsg), "已自动匹配医生：%s", docKeyword);
			PRINT_OK(successMsg);
			break;
		}
		else {
			char confirmDocName[MAX_NAME_LEN] = { 0 };
			SafeStrInput("匹配到多名医生，请输入目标医生的完整姓名（输入q重新查询）", confirmDocName, MAX_NAME_LEN);

			if (strcmp(confirmDocName, "q") == 0 || strcmp(confirmDocName, "Q") == 0) {
				continue;
			}

			char* confirmDocId = GetDoctorIdByName(confirmDocName);
			if (confirmDocId == NULL) {
				SYS_PRINT_ERR("医生姓名输入错误，请重新操作！");
				continue;
			}

			DoctorList confirmDoc = GetDoctorById(confirmDocId);
			if (confirmDoc == NULL || strcmp(confirmDoc->deptId, targetDeptId) != 0) {
				SYS_PRINT_ERR("该医生不属于当前选择的科室，请重新选择！");
				continue;
			}

			SafeStrCopy(targetDoctorId, confirmDocId, MAX_ID_LEN);
			char successMsg[80];
			snprintf(successMsg, sizeof(successMsg), "已选择医生：%s", confirmDocName);
			PRINT_OK(successMsg);
			break;
		}
	}
	printf("\n");

	targetDoctor = GetDoctorById(targetDoctorId);
	if (targetDoctor == NULL || strcmp(targetDoctor->deptId, targetDeptId) != 0) {
		SYS_PRINT_ERR("挂号失败：医生信息校验不通过！");
		WaitEnter();
		return;
	}
	if (targetDoctor->todayRegisterCount >= targetDoctor->registerLimit) {
		SYS_PRINT_ERR("挂号失败：该医生今日挂号已满，请选择其他医生！");
		WaitEnter();
		return;
	}

	PRINT_TIP("【第三步：确认挂号信息】");

	char regId[MAX_ID_LEN] = { 0 };
	srand((unsigned int)time(NULL));
	snprintf(regId, MAX_ID_LEN, "REG%06d", rand() % 1000000);

	RegistrationNode newReg = { 0 };
	SafeStrCopy(newReg.regId, regId, MAX_ID_LEN);
	SafeStrCopy(newReg.patientId, realPatientId, MAX_ID_LEN);
	SafeStrCopy(newReg.doctorId, targetDoctorId, MAX_ID_LEN);
	SafeStrCopy(newReg.deptId, targetDeptId, MAX_ID_LEN);
	SafeStrCopy(newReg.createTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
	newReg.fee = 10.0f;
	newReg.status = REG_WAITING;

	char info1[80], info2[80], info3[80], info4[80], info5[80], info6[80];
	snprintf(info1, sizeof(info1), "挂号编号：%s", newReg.regId);
	snprintf(info2, sizeof(info2), "患者姓名：%s", currentOperatorName);
	snprintf(info3, sizeof(info3), "就诊科室：%s", targetDeptId);
	snprintf(info4, sizeof(info4), "就诊医生：%s", targetDoctor->name);
	snprintf(info5, sizeof(info5), "挂号时间：%s", newReg.createTime);
	snprintf(info6, sizeof(info6), "挂号费用：%.2f元", newReg.fee);
	const char* lines[7] = { "挂号信息预览", info1, info2, info3, info4, info5, info6 };
	print_content_box(lines, 7);
	printf("\n");

	int confirm = 0;
	SafeIntInput("确认挂号？输入1确认，输入0取消", &confirm, 0, 1);
	if (confirm == 0) {
		PRINT_TIP("已取消挂号！");
		WaitEnter();
		return;
	}

	if (!AddDoctorRegisterCount(targetDoctorId)) {
		SYS_PRINT_ERR("挂号失败，医生号源已用尽！");
		WaitEnter();
		return;
	}

	if (AddRegistration(newReg)) {
		char successMsg[80];
		snprintf(successMsg, sizeof(successMsg), "挂号成功！您的挂号编号为：%s", newReg.regId);
		PRINT_OK(successMsg);
		PRINT_TIP("请等待医生叫号就诊！");

		char logContent[MAX_DETAIL_LEN] = { 0 };
		snprintf(logContent, MAX_DETAIL_LEN, "患者：%s，医生：%s，科室：%s，挂号费：%.2f",
			newReg.patientId, newReg.doctorId, newReg.deptId, newReg.fee);
		WriteLog(LOG_LEVEL_INFO, currentOperator, "门诊挂号", logContent);
	}
	else {
		SYS_PRINT_ERR("挂号失败，请稍后重试！");
		SubDoctorRegisterCount(targetDoctorId);
	}

}

// ==================== 医生接诊全流程闭环 ====================
static void DoctorReceivePatient(void) {
	CLEAR_SCREEN;
	print_title_box("接诊待诊患者");

	const char* doctorId = GetCurrentOperator();
	char msg[120];
	snprintf(msg, sizeof(msg), "当前医生：%s(ID:%s)", currentOperatorName, doctorId);
	PRINT_TIP(msg);

	RegistrationList nextReg = GetNextWaitingRegByDoctorId(doctorId);
	if (nextReg == NULL) {
		PRINT_TIP("当前暂无待诊患者！");
		WaitEnter();
		return;
	}

	PatientNode* patient = FindPatientById(nextReg->patientId);
	if (patient == NULL) {
		SYS_PRINT_ERR("患者信息获取失败，无法接诊！");
		WaitEnter();
		return;
	}

	printf("\n");
	PRINT_TIP("下一位待诊患者信息");
	char info1[120], info2[120], info3[120], info4[120], info5[120];
	snprintf(info1, sizeof(info1), "挂号编号：%s", nextReg->regId);
	snprintf(info2, sizeof(info2), "患者姓名：%s", patient->name);
	snprintf(info3, sizeof(info3), "患者ID：%s", patient->patientId);
	snprintf(info4, sizeof(info4), "挂号时间：%s", nextReg->createTime);
	snprintf(info5, sizeof(info5), "挂号费用：%.2f元", nextReg->fee);
	const char* lines[6] = { "待诊患者信息", info1, info2, info3, info4, info5 };
	print_content_box(lines, 6);

	int confirm = 0;
	SafeIntInput("确认接诊该患者？输入1确认，输入0取消", &confirm, 0, 1);
	if (confirm == 0) {
		PRINT_TIP("已取消接诊！");
		WaitEnter();
		return;
	}

	UpdateRegStatus(nextReg->regId, REG_FINISHED);
	AddDoctorRegisterCount(doctorId);
	PRINT_OK("接诊成功！");
	WaitEnter();

	CLEAR_SCREEN;
	print_title_box("患者就诊信息");
	char p_info1[120], p_info2[120], p_info3[120], p_info4[120], p_info5[120];
	snprintf(p_info1, sizeof(p_info1), "患者姓名：%s", patient->name);
	snprintf(p_info2, sizeof(p_info2), "患者ID：%s", patient->patientId);
	snprintf(p_info3, sizeof(p_info3), "性别：%s", patient->gender);
	snprintf(p_info4, sizeof(p_info4), "年龄：%d岁", patient->age);
	snprintf(p_info5, sizeof(p_info5), "联系电话：%s", patient->phone);
	const char* p_lines[6] = { p_info1, p_info2, p_info3, p_info4, p_info5 };
	print_content_box(p_lines, 5);
	printf("\n");

	PRINT_TIP("【必填】请完成患者病历书写，完成后才可继续后续操作");
	char recordId[MAX_ID_LEN] = { 0 };
	int writeResult = WriteMedicalRecord(patient, doctorId, recordId);
	if (writeResult == 0) {
		SYS_PRINT_ERR("病历书写失败，流程终止！");
		WaitEnter();
		return;
	}
	printf("\n");
	PRINT_OK("病历书写完成，已成功保存！");
	WaitEnter();

	int needPresc = 0;
	printf("\n");
	SafeIntInput("是否为该患者开具处方？输入1=是，输入0=否，返回主菜单", &needPresc, 0, 1);
	if (needPresc == 0) {
		PRINT_TIP("接诊流程已完成，返回主菜单");
		WaitEnter();
		return;
	}

	CLEAR_SCREEN;
	int prescResult = AddPrescriptionAfterConsult(patient, doctorId);
	if (prescResult == 1) {
		PRINT_OK("处方开具完成，接诊全流程结束！");
	}
	else {
		SYS_PRINT_WARN("处方开具未完成，接诊流程结束！");
	}

	char logContent[MAX_DETAIL_LEN] = { 0 };
	snprintf(logContent, MAX_DETAIL_LEN, "医生：%s，接诊患者：%s，病历ID：%s",
		doctorId, patient->patientId, recordId);
	WriteLog(LOG_LEVEL_INFO, currentOperator, "接诊患者全流程", logContent);

}

// 医生书写病历
static void DoctorWriteMedicalRecord(void) {
	print_title_box("书写病历");

	char* doctorId = (char*)currentOperator;
	char msg[80];
	snprintf(msg, sizeof(msg), "当前医生ID：%s", doctorId);
	PRINT_TIP(msg);

	printf("\n");
	PRINT_TIP("您的待诊患者列表");
	PrintDoctorRegList(doctorId);

	char regId[MAX_ID_LEN] = { 0 };
	SafeStrInput("请输入要写病历的挂号编号", regId, MAX_ID_LEN);
	if (!CheckRegExist(regId)) {
		SYS_PRINT_ERR("挂号编号不存在！");
		return;
	}

	RegistrationList pReg = FindRegById(regId);
	if (pReg == NULL) {
		SYS_PRINT_ERR("未找到对应的挂号记录！");
		return;
	}

	char detail[MAX_DETAIL_LEN] = { 0 };
	printf("\n");
	PRINT_TIP("请输入病历详情（如：主诉、诊断、治疗建议等）");
	SafeStrInput("请输入病历详情", detail, MAX_DETAIL_LEN);

	char recordId[MAX_ID_LEN] = { 0 };
	srand((unsigned int)time(NULL));
	snprintf(recordId, MAX_ID_LEN, "REC%06d", rand() % 1000000);

	RecordNode newRecord = { 0 };
	SafeStrCopy(newRecord.recordId, recordId, MAX_ID_LEN);
	SafeStrCopy(newRecord.patientId, pReg->patientId, MAX_ID_LEN);
	SafeStrCopy(newRecord.doctorId, doctorId, MAX_ID_LEN);
	SafeStrCopy(newRecord.deptId, pReg->deptId, MAX_ID_LEN);
	SafeStrCopy(newRecord.createTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
	SafeStrCopy(newRecord.detail, detail, MAX_DETAIL_LEN);
	newRecord.fee = 50.0f;
	newRecord.type = RECORD_CONSULT;

	printf("\n");
	PRINT_TIP("确认病历信息");
	char info1[80], info2[80], info3[80], info4[80], info5[80];
	snprintf(info1, sizeof(info1), "记录编号：%s", newRecord.recordId);
	snprintf(info2, sizeof(info2), "患者编号：%s", newRecord.patientId);
	snprintf(info3, sizeof(info3), "就诊医生：%s", newRecord.doctorId);
	snprintf(info4, sizeof(info4), "看诊费用：%.2f元", newRecord.fee);
	snprintf(info5, sizeof(info5), "病历详情：%s", newRecord.detail);
	const char* infoLines[6] = { "病历信息预览", info1, info2, info3, info4, info5 };
	print_content_box(infoLines, 6);

	int confirm = 0;
	SafeIntInput("确认保存病历？输入1确认，输入0取消", &confirm, 0, 1);
	if (confirm == 0) {
		PRINT_TIP("已取消保存！");
		return;
	}

	if (AddConsultRecord(newRecord)) {
		PRINT_OK("病历保存成功！");
		PRINT_TIP("提示：请继续完成处方开具！");
		UpdateRegStatus(regId, REG_FINISHED);

		char logContent[MAX_DETAIL_LEN] = { 0 };
		snprintf(logContent, MAX_DETAIL_LEN, "医生：%s，患者：%s，病历：%s",
			doctorId, newRecord.patientId, newRecord.detail);
		WriteLog(LOG_LEVEL_INFO, currentOperator, "书写病历", logContent);
	}
	else {
		SYS_PRINT_ERR("病历保存失败，请稍后重试！");
	}
}

// ==================== 患者待缴费处方查询 ====================
static int PatientViewUnpaidPrescriptions(void) {
	print_title_box("我的待缴费处方");

	char* realId = GetPatientRealId((char*)currentOperator);
	if (realId == NULL) {
		SYS_PRINT_ERR("无法获取您的患者信息，请重新登录！");
		return 0;
	}

	return QueryUnpaidPrescriptionByPatientId(realId);
}

// ==================== 患者确认缴费 ====================
static void PatientConfirmPayment(void) {
	print_title_box("确认缴费");

	int unpaidCount = PatientViewUnpaidPrescriptions();
	if (unpaidCount == 0) {
		return;
	}

	char prescId[MAX_ID_LEN] = { 0 };
	printf("\n");
	SafeStrInput("请输入要缴费的处方编号", prescId, MAX_ID_LEN);
	if (!CheckPrescriptionExist(prescId)) {
		SYS_PRINT_ERR("处方编号不存在！");
		return;
	}

	int confirm = 0;
	SafeIntInput("确认缴费？输入1确认，输入0取消", &confirm, 0, 1);
	if (confirm == 0) {
		PRINT_TIP("已取消缴费！");
		return;
	}

	if (UpdatePrescriptionToPaid(prescId)) {
		PRINT_OK("缴费成功！");
		PRINT_TIP("提示：请前往药房完成发药！");

		char logContent[MAX_DETAIL_LEN] = { 0 };
		snprintf(logContent, MAX_DETAIL_LEN, "患者：%s，缴费处方：%s",
			currentOperator, prescId);
		WriteLog(LOG_LEVEL_INFO, currentOperator, "确认缴费", logContent);
	}
	else {
		SYS_PRINT_ERR("缴费失败！该处方可能已缴费或已发药！");
	}
}

// ==================== 【最终修复版】病历书写函数 ====================
// 100%复用项目成熟的SafeStrInput，彻底解决缓冲区、不能输入、乱换行问题
static int WriteMedicalRecord(PatientNode* patient, const char* doctorId, char* recordIdOut) {
	char chiefComplaint[MAX_DETAIL_LEN] = { 0 };
	char diagnosis[MAX_DETAIL_LEN] = { 0 };
	char treatment[MAX_DETAIL_LEN] = { 0 };

	// 【关键】进入函数先彻底清空缓冲区，干掉之前残留的换行符
	ClearInputBuffer();

	while (1) {
		CLEAR_SCREEN;
		print_title_box("病历书写");
		printf("\n");
		PRINT_TIP("操作说明：按提示输入内容，回车确认，支持中文输入");
		printf("\n");

		// ========== 1. 输入主诉（复用项目成熟的SafeStrInput，绝对稳定） ==========
		if (strlen(chiefComplaint) > 0) {
			printf("【已有主诉】%s\n", chiefComplaint);
		}
		SafeStrInput("请输入患者主诉", chiefComplaint, MAX_DETAIL_LEN);

		// ========== 2. 输入诊断 ==========
		printf("\n");
		if (strlen(diagnosis) > 0) {
			printf("【已有诊断】%s\n", diagnosis);
		}
		SafeStrInput("请输入诊断结果", diagnosis, MAX_DETAIL_LEN);

		// ========== 3. 输入治疗建议 ==========
		printf("\n");
		if (strlen(treatment) > 0) {
			printf("【已有治疗建议】%s\n", treatment);
		}
		SafeStrInput("请输入治疗建议", treatment, MAX_DETAIL_LEN);

		// ========== 校验必填项 ==========
		if (strlen(chiefComplaint) == 0 || strlen(diagnosis) == 0 || strlen(treatment) == 0) {
			SYS_PRINT_ERR("保存失败！主诉、诊断、治疗建议必须全部填写，不可为空！");
			WaitEnter();
			// 再次清空缓冲区，避免残留
			ClearInputBuffer();
			continue;
		}

		// ========== 确认保存 ==========
		CLEAR_SCREEN;
		print_title_box("病历内容确认");
		char confirmLine1[MAX_DETAIL_LEN * 2], confirmLine2[MAX_DETAIL_LEN * 2], confirmLine3[MAX_DETAIL_LEN * 2];
		snprintf(confirmLine1, sizeof(confirmLine1), "【主诉】%s", chiefComplaint);
		snprintf(confirmLine2, sizeof(confirmLine2), "【诊断】%s", diagnosis);
		snprintf(confirmLine3, sizeof(confirmLine3), "【治疗建议】%s", treatment);
		const char* confirmLines[5] = { "请确认以下病历内容是否正确：", confirmLine1, confirmLine2, confirmLine3, "" };
		print_content_box(confirmLines, 5);
		printf("\n");

		int confirm = 0;
		SafeIntInput("确认保存病历？输入1=确认保存，输入0=返回修改", &confirm, 0, 1);
		if (confirm == 0) {
			// 返回修改前清空缓冲区
			ClearInputBuffer();
			continue;
		}

		// ========== 保存病历到系统 ==========
		char detail[MAX_DETAIL_LEN * 3] = { 0 };
		snprintf(detail, sizeof(detail),
			"【主诉】%s\n【诊断】%s\n【治疗建议】%s",
			chiefComplaint, diagnosis, treatment);
		srand((unsigned int)time(NULL));
		char recordId[MAX_ID_LEN] = { 0 };
		snprintf(recordId, MAX_ID_LEN, "REC%06d", rand() % 1000000);
		RecordNode newRecord = { 0 };
		SafeStrCopy(newRecord.recordId, recordId, MAX_ID_LEN);
		SafeStrCopy(newRecord.patientId, patient->patientId, MAX_ID_LEN);
		SafeStrCopy(newRecord.doctorId, doctorId, MAX_ID_LEN);
		SafeStrCopy(newRecord.deptId, GetDoctorById(doctorId)->deptId, MAX_ID_LEN);
		SafeStrCopy(newRecord.createTime, GetCurrentTimeMMDDHHMM(), MAX_DATA_LEN);
		SafeStrCopy(newRecord.detail, detail, MAX_DETAIL_LEN);
		newRecord.fee = 50.0f;
		newRecord.type = RECORD_CONSULT;

		if (!AddConsultRecord(newRecord)) {
			SYS_PRINT_ERR("病历保存失败！");
			WaitEnter();
			return 0;
		}
		SafeStrCopy(recordIdOut, recordId, MAX_ID_LEN);
		PRINT_OK("病历书写完成，已成功保存！");
		return 1;
	}
}

// ==================== 【新增】接诊后处方开具函数（多药品、自动填充、120宽度）====================
// 返回值：1=成功，0=失败
static int AddPrescriptionAfterConsult(PatientNode* patient, const char* doctorId) {
	// 临时药品列表结构体
	typedef struct {
		char medicineId[MAX_ID_LEN];
		char medicineName[MAX_NAME_LEN];
		int quantity;
		float unitPrice;
		float totalFee;
	} TempMedicine;

	TempMedicine medList[MAX_MEDICINE_NUM] = { 0 };
	int medCount = 0;

	// 自动生成处方编号前缀（多药品共用前缀，后缀加序号）
	srand((unsigned int)time(NULL));
	char prescPrefix[MAX_ID_LEN] = { 0 };
	snprintf(prescPrefix, MAX_ID_LEN, "PRE%06d", rand() % 1000000);

	// 获取医生信息
	DoctorList doctor = GetDoctorById(doctorId);
	if (doctor == NULL) {
		SYS_PRINT_ERR("医生信息获取失败！");
		return 0;
	}

	while (1) {
		CLEAR_SCREEN;
		print_title_box_120("处方开具");
		printf("\n");

		// 打印处方基础信息（自动填充）
		PRINT_TIP("处方基础信息");
		char info1[120], info2[120], info3[120], info4[120];
		snprintf(info1, sizeof(info1), "处方编号前缀：%s", prescPrefix);
		snprintf(info2, sizeof(info2), "患者信息：%s(ID:%s)", patient->name, patient->patientId);
		snprintf(info3, sizeof(info3), "开方医生：%s(ID:%s)", doctor->name, doctorId);
		snprintf(info4, sizeof(info4), "开方科室：%s", doctor->deptId);
		const char* infoLines[5] = { info1, info2, info3, info4, "" };
		print_content_box(infoLines, 5);
		printf("\n");

		// 打印已添加药品列表（120宽度表格）
		PRINT_TIP("已添加药品列表");
		if (medCount == 0) {
			PRINT_TIP("暂无添加药品");
		}
		else {
			int cols[] = { 6, 12, 20, 10, 12, 12, 12 };
			AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
			const char* headers[] = { "序号", "药品编号", "药品通用名", "单价", "数量", "单药总价", "库存余量" };

			print_table_sep_120(cols, 7);
			print_table_row_120(cols, aligns, headers, 7);
			print_table_sep_120(cols, 7);

			for (int i = 0; i < medCount; i++) {
				char idx[10], qty[10], price[20], total[20], stock[20];
				snprintf(idx, sizeof(idx), "%d", i + 1);
				snprintf(qty, sizeof(qty), "%d", medList[i].quantity);
				snprintf(price, sizeof(price), "%.2f", medList[i].unitPrice);
				snprintf(total, sizeof(total), "%.2f", medList[i].totalFee);
				snprintf(stock, sizeof(stock), "%d", QueryMedicineStock(medList[i].medicineId));

				const char* data[] = {
					idx, medList[i].medicineId, medList[i].medicineName,
					price, qty, total, stock
				};
				print_table_row_120(cols, aligns, data, 7);
			}
			print_table_sep_120(cols, 7);

			// 计算总金额
			float totalAmount = 0.0f;
			for (int i = 0; i < medCount; i++) {
				totalAmount += medList[i].totalFee;
			}
			char totalMsg[120];
			snprintf(totalMsg, sizeof(totalMsg), "处方总金额：%.2f元", totalAmount);
			PRINT_TIP(totalMsg);
		}
		printf("\n");

		// 操作选项
		PRINT_TIP("操作选项");
		printf("\n");
		print_screen_center(); printf("| %-76s |\n", "1 - 添加药品");
		print_screen_center(); printf("| %-76s |\n", "2 - 删除药品");
		print_screen_center(); printf("| %-76s |\n", "3 - 预览完整处方");
		print_screen_center(); printf("| %-76s |\n", "4 - 确认保存处方（至少添加1种药品）");
		print_screen_center(); printf("| %-76s |\n", "0 - 取消开具，返回主菜单");
		print_screen_center();
		printf("+");
		for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
		printf("+\n");

		// 获取用户选择
		int choice = 0;
		SafeIntInput("请选择操作（0-4）", &choice, 0, 4);

		switch (choice) {
		case 0:
			// 取消开具
			return 0;

		case 1:
			// 添加药品
			if (medCount >= MAX_MEDICINE_NUM) {
				SYS_PRINT_ERR("添加失败！最多支持添加20种药品");
				WaitEnter();
				continue;
			}

			char medId[MAX_ID_LEN] = { 0 };
			int quantity = 0;
			printf("\n");
			SafeStrInput("请输入药品编号", medId, MAX_ID_LEN);

			// 校验药品是否存在
			if (!CheckMedicineExist(medId)) {
				SYS_PRINT_ERR("药品编号不存在！");
				WaitEnter();
				continue;
			}

			// 校验药品是否已添加
			int isExist = 0;
			for (int i = 0; i < medCount; i++) {
				if (strcmp(medList[i].medicineId, medId) == 0) {
					isExist = 1;
					break;
				}
			}
			if (isExist) {
				SYS_PRINT_ERR("该药品已添加，请勿重复添加！");
				WaitEnter();
				continue;
			}

			// 获取药品信息
			char medName[MAX_NAME_LEN] = { 0 };
			GetMedicineGenericName(medId, medName, MAX_NAME_LEN);
			float unitPrice = GetMedicineSalePrice(medId);
			int stock = QueryMedicineStock(medId);

			// 输入数量，校验库存
			while (1) {
				SafeIntInput("请输入药品数量", &quantity, 1, 1000);
				if (stock < quantity) {
					char msg[120];
					snprintf(msg, sizeof(msg), "库存不足！当前库存：%d，请重新输入", stock);
					SYS_PRINT_ERR(msg);
					continue;
				}
				break;
			}

			// 加入药品列表
			SafeStrCopy(medList[medCount].medicineId, medId, MAX_ID_LEN);
			SafeStrCopy(medList[medCount].medicineName, medName, MAX_NAME_LEN);
			medList[medCount].quantity = quantity;
			medList[medCount].unitPrice = unitPrice;
			medList[medCount].totalFee = unitPrice * quantity;
			medCount++;

			PRINT_OK("药品添加成功！");
			WaitEnter();
			break;

		case 2:
			// 删除药品
			if (medCount == 0) {
				SYS_PRINT_ERR("暂无药品可删除！");
				WaitEnter();
				continue;
			}
			int delIdx = 0;
			SafeIntInput("请输入要删除的药品序号", &delIdx, 1, medCount);
			delIdx--; // 转换为0开始的数组下标

			// 校验下标合法性
			if (delIdx < 0 || delIdx >= medCount || medCount > MAX_MEDICINE_NUM) {
				SYS_PRINT_ERR("删除失败：输入的序号非法！");
				WaitEnter();
				continue;
			}

			// 前移数组元素
			for (int i = delIdx; i < medCount - 1 && i < MAX_MEDICINE_NUM - 1; i++) {
				medList[i] = medList[i + 1];
			}

			// 清空最后一个有效元素，避免脏数据
			memset(&medList[medCount - 1], 0, sizeof(TempMedicine));
			medCount--;

			PRINT_OK("药品删除成功！");
			WaitEnter();
			break;
		case 3:
			// 预览完整处方
			if (medCount == 0) {
				SYS_PRINT_ERR("暂无药品，无法预览！");
				WaitEnter();
				continue;
			}

			CLEAR_SCREEN;
			print_title_box_120("处方预览");
			printf("\n");

			// 处方头信息
			char h1[120], h2[120], h3[120], h4[120], h5[120];
			snprintf(h1, sizeof(h1), "处方编号：%s", prescPrefix);
			snprintf(h2, sizeof(h2), "患者姓名：%s    患者ID：%s", patient->name, patient->patientId);
			snprintf(h3, sizeof(h3), "开方医生：%s    医生ID：%s", doctor->name, doctorId);
			snprintf(h4, sizeof(h4), "开方科室：%s    开方时间：%s", doctor->deptId, GetCurrentDate());
			float totalAmount = 0.0f;
			for (int i = 0; i < medCount; i++) totalAmount += medList[i].totalFee;
			snprintf(h5, sizeof(h5), "处方总金额：%.2f元", totalAmount);
			const char* hLines[6] = { h1, h2, h3, h4, h5, "" };
			print_content_box(hLines, 6);
			printf("\n");

			// 药品明细表格
			int cols[] = { 6, 12, 24, 10, 12, 12 };
			AlignType aligns[] = { ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER,ALIGN_CENTER };
			const char* headers[] = { "序号", "药品编号", "药品通用名", "单价", "数量", "金额" };

			print_table_sep_120(cols, 6);
			print_table_row_120(cols, aligns, headers, 6);
			print_table_sep_120(cols, 6);

			for (int i = 0; i < medCount; i++) {
				char idx[10], qty[10], price[20], total[20];
				snprintf(idx, sizeof(idx), "%d", i + 1);
				snprintf(qty, sizeof(qty), "%d", medList[i].quantity);
				snprintf(price, sizeof(price), "%.2f", medList[i].unitPrice);
				snprintf(total, sizeof(total), "%.2f", medList[i].totalFee);

				const char* data[] = { idx, medList[i].medicineId, medList[i].medicineName, price, qty, total };
				print_table_row_120(cols, aligns, data, 6);
			}
			print_table_sep_120(cols, 6);

			printf("\n");
			PRINT_TIP("预览完成，按回车键返回");
			WaitEnter();
			break;

		case 4:
			// 确认保存处方
			if (medCount == 0) {
				SYS_PRINT_ERR("保存失败！至少添加1种药品才可保存处方");
				WaitEnter();
				continue;
			}

			// 二次确认
			int confirm = 0;
			SafeIntInput("确认保存处方？保存后不可修改，输入1确认，输入0取消", &confirm, 0, 1);
			if (confirm == 0) {
				PRINT_TIP("已取消保存！");
				WaitEnter();
				continue;
			}

			// 遍历药品列表，逐个生成处方记录
			int successCount = 0;
			for (int i = 0; i < medCount; i++) {
				PrescNode newPresc = { 0 };
				// 生成唯一处方编号
				char prescId[MAX_ID_LEN] = { 0 };
				snprintf(prescId, MAX_ID_LEN, "%s_%02d", prescPrefix, i + 1);

				// 填充处方数据（全量自动填充）
				SafeStrCopy(newPresc.prescId, prescId, MAX_ID_LEN);
				SafeStrCopy(newPresc.patientId, patient->patientId, MAX_ID_LEN);
				SafeStrCopy(newPresc.doctorId, doctorId, MAX_ID_LEN);
				SafeStrCopy(newPresc.medicineId, medList[i].medicineId, MAX_ID_LEN);
				newPresc.quantity = medList[i].quantity;
				newPresc.totalFee = medList[i].totalFee;
				newPresc.status = PRESC_UNPAY;
				SafeStrCopy(newPresc.createTime, GetCurrentDate(), MAX_DATA_LEN);
				memset(newPresc.dispenseTime, 0, sizeof(newPresc.dispenseTime));

				// 插入处方链表
				PrescNode* newNode = (PrescNode*)malloc(sizeof(PrescNode));
				if (newNode == NULL) {
					char msg[120];
					snprintf(msg, sizeof(msg), "药品【%s】处方创建失败，内存不足！", medList[i].medicineName);
					SYS_PRINT_ERR(msg);
					continue;
				}
				*newNode = newPresc;

				// 头插法插入链表
				newNode->next = prescHead->next;
				prescHead->next = newNode;
				successCount++;
			}

			// 保存结果提示
			if (successCount == medCount) {
				char msg[120];
				snprintf(msg, sizeof(msg), "处方保存成功！共生成%d条处方记录", successCount);
				PRINT_OK(msg);
				WaitEnter();
				return 1;
			}
			else {
				char msg[120];
				snprintf(msg, sizeof(msg), "处方部分保存成功！成功%d条，失败%d条", successCount, medCount - successCount);
				SYS_PRINT_WARN(msg);
				WaitEnter();
				return successCount > 0 ? 1 : 0;
			}
		}
	}
}

//管理员新增医生
static void AdminAddDoctor(void) {
	CLEAR_SCREEN;
	print_title_box("管理员新增医生");
	DoctorNode newDoctor = { 0 };
	memset(&newDoctor, 0, sizeof(DoctorNode));

	while (1) {
		SafeStrInput("请输入医生唯一ID", newDoctor.doctorId, MAX_ID_LEN);
		if (CheckDoctorExist(newDoctor.doctorId)) {
			PRINT_WARN("医生ID已存在，请重新输入！");
			continue;
		}
		if (!IsAlphaNumber(newDoctor.doctorId)) {
			PRINT_WARN("医生ID仅允许字母和数字，请重新输入！");
			continue;
		}
		break;
	}

	while (1) {
		SafeStrInput("请输入医生姓名", newDoctor.name, MAX_NAME_LEN);
		if (HasNumber(newDoctor.name)) {
			PRINT_WARN("医生姓名不能包含数字，请重新输入！");
			continue;
		}
		break;
	}

	printf("\n");
	PrintAllDepartmentBrief();
	printf("\n");
	while (1) {
		SafeStrInput("请输入所属科室ID", newDoctor.deptId, MAX_ID_LEN);
		if (!CheckDepartmentExist(newDoctor.deptId)) {
			PRINT_WARN("科室ID不存在，请重新输入！");
			continue;
		}
		break;
	}

	while (1) {
		SafeStrInput("请输入医生职称", newDoctor.title, MAX_NAME_LEN);
		if (HasNumber(newDoctor.title)) {
			PRINT_WARN("职称不能包含数字，请重新输入！");
			continue;
		}
		break;
	}

	while (1) {
		SafeStrInput("请输入医生联系电话", newDoctor.phone, MAX_ID_LEN);
		if (!IsNumber(newDoctor.phone)) {
			PRINT_WARN("联系电话必须为纯数字，请重新输入！");
			continue;
		}
		break;
	}

	SafeIntInput("请输入医生每日挂号限额", &newDoctor.registerLimit, 1, 100);
	newDoctor.todayRegisterCount = 0;
	newDoctor.next = NULL;

	printf("\n");
	PRINT_TIP("新增医生信息预览");
	char info1[80], info2[80], info3[80], info4[80], info5[80], info6[80];
	snprintf(info1, sizeof(info1), "医生ID：%s", newDoctor.doctorId);
	snprintf(info2, sizeof(info2), "医生姓名：%s", newDoctor.name);
	snprintf(info3, sizeof(info3), "所属科室ID：%s", newDoctor.deptId);
	snprintf(info4, sizeof(info4), "职称：%s", newDoctor.title);
	snprintf(info5, sizeof(info5), "联系电话：%s", newDoctor.phone);
	snprintf(info6, sizeof(info6), "每日挂号限额：%d", newDoctor.registerLimit);
	const char* lines[7] = { info1, info2, info3, info4, info5, info6 };
	print_content_box(lines, 6);

	int confirm = 0;
	SafeIntInput("确认新增该医生？输入1=确认，输入0=取消", &confirm, 0, 1);
	if (confirm == 0) {
		PRINT_TIP("已取消新增医生操作！");
		WaitEnter();
		return;
	}

	if (AddDoctor(newDoctor, 0)) {
		PRINT_OK("医生新增成功！");
		WriteLog(LOG_LEVEL_INFO, currentOperator, "管理员新增医生", newDoctor.doctorId);
	}
	else {
		PRINT_ERR("医生新增失败！");
	}
}