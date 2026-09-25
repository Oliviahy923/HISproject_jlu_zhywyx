#include <stdio.h>
#include <string.h>
#include "common.h"

// 全局居中函数（标准80宽度）
void print_screen_center(void) {
    int pad = (SCREEN_WIDTH - BOX_WIDTH) / 2;
    for (int i = 0; i < pad; i++) printf(" ");
}

// 自定义宽度居中函数（医疗记录专用）
void print_screen_center_ex(int boxWidth) {
    int pad = (SCREEN_WIDTH - boxWidth) / 2;
    for (int i = 0; i < pad; i++) printf(" ");
}

// 打印居中字符串
void print_centered(const char* str, int width) {
    int len = (int)strlen(str);
    int left = (width - len) / 2;
    int right = width - len - left;
    printf("%*s%s%*s", left, "", str, right, "");
}

// 标准标题框（80宽度）
void print_title_box(const char* title) {
    print_screen_center();
    printf("+");
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
    printf("+\n");
    print_screen_center();
    printf("|");
    print_centered(title, BOX_WIDTH - 2);
    printf("|\n");
    print_screen_center();
    printf("+");
    for (int i = 0; i < BOX_WIDTH - 2; i++) printf("-");
    printf("+\n");
}

// 自定义宽度标题框（医疗记录120宽度专用）
void print_title_box_ex(const char* title, int boxWidth) {
    print_screen_center_ex(boxWidth);
    printf("+");
    for (int i = 0; i < boxWidth - 2; i++) printf("-");
    printf("+\n");
    print_screen_center_ex(boxWidth);
    printf("|");
    print_centered(title, boxWidth - 2);
    printf("|\n");
    print_screen_center_ex(boxWidth);
    printf("+");
    for (int i = 0; i < boxWidth - 2; i++) printf("-");
    printf("+\n");
}

// 标准内容框（80宽度）
void print_content_box(const char** lines, int line_count) {
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

// 自定义宽度内容框
void print_content_box_ex(const char** lines, int line_count, int boxWidth) {
    int contentWidth = boxWidth - 4;
    print_screen_center_ex(boxWidth);
    printf("+");
    for (int i = 0; i < boxWidth - 2; i++) printf("-");
    printf("+\n");
    for (int i = 0; i < line_count; i++) {
        print_screen_center_ex(boxWidth);
        printf("| %-*s |\n", contentWidth, lines[i]);
    }
    print_screen_center_ex(boxWidth);
    printf("+");
    for (int i = 0; i < boxWidth - 2; i++) printf("-");
    printf("+\n");
}

// 状态提示框（全系统统一）
void print_status_box(const char* type, const char* msg) {
    const char* lines[2] = { type, msg };
    print_content_box(lines, 2);
}

// 计算表格总宽度
static int sum_table_width(const int* cols, int n_cols) {
    int total = 0;
    for (int i = 0; i < n_cols; i++) {
        total += cols[i];
    }
    total += n_cols + 1;
    return total;
}

// 标准表格分隔线（80宽度）
void print_table_sep(const int* cols, int n_cols) {
    int table_width = sum_table_width(cols, n_cols);
    int center_pad = (BOX_WIDTH - table_width) / 2;
    print_screen_center();
    for (int i = 0; i < center_pad; i++) printf(" ");
    printf("+");
    for (int i = 0; i < n_cols; i++) {
        for (int j = 0; j < cols[i]; j++) printf("-");
        printf("+");
    }
    printf("\n");
}

// 自定义宽度表格分隔线
void print_table_sep_ex(const int* cols, int n_cols, int boxWidth) {
    int table_width = sum_table_width(cols, n_cols);
    int center_pad = (boxWidth - table_width) / 2;
    print_screen_center_ex(boxWidth);
    for (int i = 0; i < center_pad; i++) printf(" ");
    printf("+");
    for (int i = 0; i < n_cols; i++) {
        for (int j = 0; j < cols[i]; j++) printf("-");
        printf("+");
    }
    printf("\n");
}

// 标准表格行（80宽度）
void print_table_row(const int* cols, const AlignType* aligns, const char** data, int n_cols) {
    int table_width = sum_table_width(cols, n_cols);
    int center_pad = (BOX_WIDTH - table_width) / 2;
    print_screen_center();
    for (int i = 0; i < center_pad; i++) printf(" ");
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

// 自定义宽度表格行
void print_table_row_ex(const int* cols, const AlignType* aligns, const char** data, int n_cols, int boxWidth) {
    int table_width = sum_table_width(cols, n_cols);
    int center_pad = (boxWidth - table_width) / 2;
    print_screen_center_ex(boxWidth);
    for (int i = 0; i < center_pad; i++) printf(" ");
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
