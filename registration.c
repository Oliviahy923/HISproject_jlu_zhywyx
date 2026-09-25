#include "registration.h"
#include "doctor.h"
#include "patient.h"
#include "safe_utils.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ==================== 通用打印辅助函数（与system/prescription模块100%统一，80宽度）====================
#define BOX_WIDTH 80
#define SCREEN_WIDTH 100

static void print_screen_center(void) {
    int pad = (SCREEN_WIDTH - BOX_WIDTH) / 2;
    for (int i = 0; i < pad; i++) printf(" ");
}

static void print_title_box(const char* title) {
    print_screen_center();
    printf("+");
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
    printf("+\n");
    print_screen_center();
    printf("|");
    int len = (int)strlen(title);
    int left = (BOX_WIDTH - 2 - len) / 2;
    int right = BOX_WIDTH - 2 - len - left;
    printf("%*s%s%*s", left, "", title, right, "");
    printf("|\n");
    print_screen_center();
    printf("+");
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
    printf("+\n");
}

static void print_content_box(const char** lines, int line_count) {
    print_screen_center();
    printf("+");
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
    printf("+\n");
    for (int i = 0; i < line_count; i++) {
        print_screen_center();
        printf("| %-76s |\n", lines[i]);
    }
    print_screen_center();
    printf("+");
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
    printf("+\n");
}


static RegistrationList regHead = NULL;
#define REG_FILE "registration.txt"

void InitRegistrationList() {
    regHead = (RegistrationList)malloc(sizeof(RegistrationNode));
    if (!regHead) return;
    regHead->next = NULL;
}

int AddRegistration(RegistrationNode reg) {
    RegistrationList n = (RegistrationList)malloc(sizeof(RegistrationNode));
    if (!n) return 0;
    *n = reg;
    n->next = regHead->next;
    regHead->next = n;
    return 1;
}

// 【修改】PrintDoctorRegList：显示患者姓名、格式统一80宽度、参数加const
void PrintDoctorRegList(const char* doctorId) {
    if (regHead == NULL || regHead->next == NULL || doctorId == NULL) {
        PRINT_TIP("暂无待诊患者！");
        return;
    }

    char title[80];
    snprintf(title, sizeof(title), "医生%s待诊患者列表", doctorId);
    print_title_box(title);

    printf("\n");
    print_screen_center();
    printf("+------------+------------+--------------------+------------+\n");
    print_screen_center();
    printf("|  挂号编号  |  患者编号  |      患者姓名      |    状态    |\n");
    print_screen_center();
    printf("+------------+------------+--------------------+------------+\n");

    int cnt = 0;
    RegistrationList p = regHead->next;
    while (p) {
        if (strcmp(p->doctorId, doctorId) == 0 && p->status == REG_WAITING) {
            cnt++;
            // 【新增】通过患者ID获取患者姓名
            PatientNode* pat = FindPatientById(p->patientId);
            const char* patientName = (pat != NULL) ? pat->name : "未知患者";
            const char* statusStr = "待诊";

            print_screen_center();
            printf("| %-10s | %-10s | %-18s | %-10s |\n",
                p->regId, p->patientId, patientName, statusStr);
        }
        p = p->next;
    }

    print_screen_center();
    printf("+------------+------------+--------------------+------------+\n");

    if (cnt == 0) {
        PRINT_TIP("暂无待诊患者！");
    }
    else {
        char msg[80];
        snprintf(msg, sizeof(msg), "共%d名待诊患者", cnt);
        PRINT_TIP(msg);
    }
}

// 【新增】按医生ID获取下一个待诊患者（按挂号时间排序，状态为Waiting）
RegistrationList GetNextWaitingRegByDoctorId(const char* doctorId) {
    if (regHead == NULL || regHead->next == NULL || doctorId == NULL) {
        return NULL;
    }

    RegistrationList nextReg = NULL;
    RegistrationList p = regHead->next;

    // 遍历找到第一个（按链表顺序，即挂号顺序）待诊患者
    while (p != NULL) {
        if (strcmp(p->doctorId, doctorId) == 0 && p->status == REG_WAITING) {
            nextReg = p;
            break;
        }
        p = p->next;
    }

    return nextReg;
}

void UpdateRegStatus(char* regId, RegStatus status) {
    RegistrationList p = regHead->next;
    while (p) {
        if (strcmp(p->regId, regId) == 0) {
            p->status = status;
            break;
        }
        p = p->next;
    }
}

void SaveRegistrationToFile(char* filename) {
    FILE* fp = fopen(filename, "w");
    if (!fp) return;
    RegistrationList p = regHead->next;
    while (p) {
        fprintf(fp, "%s,%s,%s,%s,%s,%.2f,%d\n",
            p->regId, p->patientId, p->doctorId, p->deptId,
            p->createTime, p->fee, p->status);
        p = p->next;
    }
    fclose(fp);
}

int LoadRegistrationFromFile(char* filename) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        return 0;
    }
    RegistrationNode r;
    int loadCount = 0;
    while (fscanf(fp, "%[^,],%[^,],%[^,],%[^,],%[^,],%f,%d\n",
        r.regId, r.patientId, r.doctorId, r.deptId,
        r.createTime, &r.fee, (int*)&r.status) == 7) {
        if (AddRegistration(r)) {
            loadCount++;
        }
    }
    fclose(fp);
    return loadCount;
}

void FreeRegistrationList() {
    RegistrationList p = regHead, next;
    while (p) {
        next = p->next;
        free(p);
        p = next;
    }
    regHead = NULL;
}

RegistrationList FindRegByPatient(char* patientId) {
    RegistrationList p = regHead->next;
    while (p != NULL) {
        if (strcmp(p->patientId, patientId) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

int CheckRegExist(char* regId) {
    if (regHead == NULL || regHead->next == NULL) {
        return 0;
    }
    RegistrationList p = regHead->next;
    while (p != NULL) {
        if (strcmp(p->regId, regId) == 0) {
            return 1;
        }
        p = p->next;
    }
    return 0;
}

RegistrationList FindRegById(char* regId) {
    if (regHead == NULL || regId == NULL) {
        return NULL;
    }
    RegistrationList p = regHead->next;
    while (p != NULL) {
        if (strcmp(p->regId, regId) == 0) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}