#ifndef COMMON_H
#define COMMON_H

/* 字段长度定义 */
#define MAX_ID_LEN        20
#define MAX_NAME_LEN      50
#define MAX_DETAIL_LEN    500
#define MAX_DATA_LEN      50
#define MAX_FILE_PATH_LEN 100
#define MAX_LINE_LEN      1024
#define MAX_DATE_LEN      30

/* 业务状态常量定义 */
#define PATIENT_OUTPATIENT  1
#define PATIENT_INPATIENT   2
#define BED_FREE            0
#define BED_OCCUPIED        1
#define RECORD_REGISTER     0
#define RECORD_CONSULT      1
#define RECORD_EXAMINE      2
#define RECORD_HOSPITALIZE  3

// 患者类型枚举
#define PATIENT_OUTPATIENT 1  // 门诊患者
#define PATIENT_INPATIENT  2  // 住院患者
// 住院状态枚举
#define INHOSPITAL_STATUS_UNPAID 0    // 待缴费
#define INHOSPITAL_STATUS_PAID 1      // 已缴费待入院
#define INHOSPITAL_STATUS_INHOSPITAL 2 // 住院中
#define INHOSPITAL_STATUS_DISCHARGED 3 // 已出院
// 住院押金默认金额
#define DEFAULT_DEPOSIT_FEE 2000.0f

// 挂号状态
typedef enum {
    REG_WAITING = 0,
    REG_FINISHED = 1
} RegStatus;

// 处方状态枚举
typedef enum {
    PRESC_UNPAY,    // 未缴费
    PRESC_PAID,      // 已缴费待发药
    PRESC_FINISHED,  // 已发药完成
    PRESC_CANCELED   // 已作废（冲账回退）
} PrescStatus;

/* 通用工具函数声明 */
int IsNumber(char* str);
int HasNumber(char* str);
int IsAlphaNumber(char* str);
// 新增：全系统通用回车等待函数声明
void WaitEnter(void);

/* 界面美化宏 */
#ifdef _WIN32
#define CLEAR_SCREEN system("cls")
#else
#define CLEAR_SCREEN system("clear")
#endif

#define PRINT_LINE(width, ch) do { \
    for (int i = 0; i < width; i++) putchar(ch); \
    putchar('\n'); \
} while(0)

#define PRINT_CENTERED_TITLE(title, width) do { \
    const char* _t = title; \
    int charCnt = 0, chineseCnt = 0, totalW = 0, pad = 0; \
    if (_t == NULL) break; \
    charCnt = (int)strlen(_t); \
    for (int i = 0; _t[i] != '\0'; i++) { \
        if ((unsigned char)_t[i] > 0x80) chineseCnt++; \
    } \
    totalW = charCnt + chineseCnt; \
    pad = (width - totalW) / 2; \
    printf("|"); \
    for (int i = 0; i < pad - 1; i++) putchar(' '); \
    printf("%s", _t); \
    for (int i = 0; i < (width - totalW - pad - 1); i++) putchar(' '); \
    printf("|\n"); \
} while(0)

// ==================== 全局统一打印格式宏定义（全系统唯一标准） ====================
#define BOX_WIDTH 80                // 全局统一：标题框/表格总宽度
#define RECORD_BOX_WIDTH 120        // 医疗记录专用：打印边框宽度
#define SCREEN_WIDTH 100            // 屏幕基准宽度，全局居中对齐

// 对齐方式枚举
typedef enum { ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT } AlignType;

// 全局打印函数声明（所有.c文件通用，禁止重复定义）
void print_screen_center(void);
void print_screen_center_ex(int boxWidth); // 自定义宽度居中
void print_centered(const char* str, int width);
void print_title_box(const char* title);
void print_title_box_ex(const char* title, int boxWidth); // 医疗记录专用
void print_content_box(const char** lines, int line_count);
void print_content_box_ex(const char** lines, int line_count, int boxWidth);
void print_status_box(const char* type, const char* msg);
void print_table_sep(const int* cols, int n_cols);
void print_table_sep_ex(const int* cols, int n_cols, int boxWidth);
void print_table_row(const int* cols, const AlignType* aligns, const char** data, int n_cols);
void print_table_row_ex(const int* cols, const AlignType* aligns, const char** data, int n_cols, int boxWidth);

// 全局状态提示宏（全系统统一，禁止重复定义）
#define PRINT_OK(msg)  print_status_box("【操作成功】", msg)
#define PRINT_ERR(msg) print_status_box("【操作失败】", msg)
#define PRINT_WARN(msg) print_status_box("【警告】", msg)
#define PRINT_TIP(msg)  print_status_box("【提示】", msg)

#endif
