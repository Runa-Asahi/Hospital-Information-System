
#define NK_IMPLEMENTATION
#define NK_RAYLIB_IMPLEMENTATION
#include "include/nuklear_raylib.h"

#include <string.h>
#include <stdio.h>
// 患者信息结构
typedef struct {
    char name[32];
    int age;
    char diagnosis[64];
    char status[16];
} Patient;

// 患者数据
Patient patients[] = {
    {"张三", 25, "上呼吸道感染", "待入院"},
    {"李四", 30, "急性肠胃炎", "住院中"},
    {"王五", 45, "高血压", "已出院"},
    {"赵六", 22, "过敏性皮炎", "待诊"}
};
int patient_count = 4;

// 图表数据
float in_data[] = {70, 52, 40, 20, 38, 22, 40};
float out_data[] = {22, 42, 45, 40, 52, 35, 50};
const char* x_label[] = {"1", "2", "3", "4", "5", "6", "7"};

// 科室与时间选项
const char* dept[] = {"内科", "外科", "儿科", "骨科", "急诊科"};
const char* time[] = {"08:00", "09:00", "10:00", "14:00", "15:00"};
int sel_dept = 0;
int sel_time = 0;

int main() {
    const int screen_width = 1600;
    const int screen_height = 900;
    const int menu_width = 220;
    const int top_bar_height = 60;

    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screen_width, screen_height, "医院综合管理系统");
    SetTargetFPS(60);

    struct nk_context* ctx = nk_raylib_init(24);
    if (!ctx) {
        CloseWindow();
        return -1;
    }

    // 加载中文字体
    Font font = LoadFontEx("AlibabaPuHuiTi-3-115-Black.ttf", 24, NULL, 0);
    if (!font.texture.id) font = GetFontDefault();
    nk_raylib_set_font(ctx, font);

    int current_page = 0;
    char patient_name[32] = {0};

    while (!WindowShouldClose()) {
        nk_raylib_input(ctx);
        BeginDrawing();

        // 顶部栏
        nk_panel_begin(ctx, "top_bar", nk_rect(0, 0, screen_width, top_bar_height), NK_WINDOW_NO_SCROLLBAR);
        nk_layout_row_dynamic(ctx, top_bar_height, 1);
        nk_widget_fill_color(ctx, nk_rect(0, 0, menu_width, top_bar_height), 
                            nk_rgba(22, 119, 255, 255), nk_rgba(22, 119, 255, 255));
        nk_label(ctx, "医院综合管理系统", NK_TEXT_CENTERED);
        nk_panel_end(ctx);

        // 左侧导航栏
        nk_panel_begin(ctx, "menu", nk_rect(0, top_bar_height, menu_width, screen_height - top_bar_height), NK_WINDOW_NO_SCROLLBAR);
        nk_layout_row_dynamic(ctx, 60, 1);

        if (nk_button_label(ctx, "  首页")) current_page = 0;
        if (current_page == 0)
            nk_widget_fill_color(ctx, nk_rect(0, 0, menu_width, 60), 
                                nk_rgba(22, 119, 255, 255), nk_rgba(64, 158, 255, 255));

        if (nk_button_label(ctx, "  患者管理")) current_page = 1;
        if (current_page == 1)
            nk_widget_fill_color(ctx, nk_rect(0, 60, menu_width, 60), 
                                nk_rgba(22, 119, 255, 255), nk_rgba(64, 158, 255, 255));

        if (nk_button_label(ctx, "  挂号服务")) current_page = 2;
        if (current_page == 2)
            nk_widget_fill_color(ctx, nk_rect(0, 120, menu_width, 60), 
                                nk_rgba(22, 119, 255, 255), nk_rgba(64, 158, 255, 255));

        if (nk_button_label(ctx, "  统计分析")) current_page = 3;
        if (current_page == 3)
            nk_widget_fill_color(ctx, nk_rect(0, 180, menu_width, 60), 
                                nk_rgba(22, 119, 255, 255), nk_rgba(64, 158, 255, 255));

        nk_panel_end(ctx);

        // 主界面
        nk_panel_begin(ctx, "main", nk_rect(menu_width, top_bar_height, screen_width - menu_width, screen_height - top_bar_height), NK_WINDOW_NO_SCROLLBAR);

        if (current_page == 0 || current_page == 1) {
            // 患者列表区域
            nk_layout_space_begin(ctx, NK_DYNAMIC, screen_height - top_bar_height - 20, 2);
            nk_layout_space_push(ctx, nk_rect(10, 10, 780, screen_height - top_bar_height - 40));

            nk_panel_begin(ctx, "patient_list", nk_rect(0, 0, 780, screen_height - top_bar_height - 40), NK_WINDOW_BORDER);
            nk_layout_row_dynamic(ctx, 50, 2);
            nk_text_heading(ctx, "患者信息");
            if (nk_button_label(ctx, "新增挂号")) current_page = 2;

            // 表头
            nk_layout_row_dynamic(ctx, 40, 4);
            nk_text_label(ctx, "姓名");
            nk_text_label(ctx, "年龄");
            nk_text_label(ctx, "诊断");
            nk_text_label(ctx, "状态");

            // 患者列表
            static int selected[4] = {0};
            for (int i = 0; i < patient_count; i++) {
                char age_buf[8];
                sprintf(age_buf, "%d", patients[i].age);
                nk_layout_row_dynamic(ctx, 45, 6);
                nk_checkbox_label(ctx, "", &selected[i]);
                nk_text_label(ctx, patients[i].name);
                nk_text_label(ctx, age_buf);
                nk_text_label(ctx, patients[i].diagnosis);
                nk_text_label(ctx, patients[i].status);
                nk_button_label(ctx, "编辑");
                nk_button_label(ctx, "查看");
            }
            nk_panel_end(ctx);

            // 右侧表单与图表
            nk_layout_space_push(ctx, nk_rect(800, 10, 540, screen_height - top_bar_height - 40));

            // 挂号表单
            nk_panel_begin(ctx, "register_form", nk_rect(0, 0, 540, 240), NK_WINDOW_BORDER);
            nk_layout_row_dynamic(ctx, 40, 2);
            nk_text_label(ctx, "姓名");
            nk_edit_string(ctx, NK_EDIT_SIMPLE, patient_name, 32, 0);
            nk_text_label(ctx, "科室");
            nk_combo(ctx, dept, 5, &sel_dept, 40, 180);
            nk_text_label(ctx, "时间");
            nk_combo(ctx, time, 5, &sel_time, 40, 180);
            nk_layout_row_dynamic(ctx, 45, 2);
            nk_button_label(ctx, "提交");
            nk_button_label(ctx, "重置");
            nk_panel_end(ctx);

            // 图表区域
            nk_panel_begin(ctx, "chart_area", nk_rect(0, 250, 540, screen_height - top_bar_height - 300), NK_WINDOW_BORDER);
            nk_layout_row_dynamic(ctx, 40, 1);
            nk_text_heading(ctx, "住院统计");
            nk_panel_end(ctx);

            nk_layout_space_end(ctx);
        }
        else if (current_page == 2) {
            nk_layout_row_dynamic(ctx, 50, 1);
            nk_text_heading(ctx, "挂号服务");
            nk_layout_row_dynamic(ctx, 40, 2);
            nk_text_label(ctx, "姓名");
            nk_edit_string(ctx, NK_EDIT_SIMPLE, patient_name, 32, 0);
            nk_text_label(ctx, "科室");
            nk_combo(ctx, dept, 5, &sel_dept, 40, 200);
            nk_text_label(ctx, "时间");
            nk_combo(ctx, time, 5, &sel_time, 40, 200);
            nk_layout_row_dynamic(ctx, 45, 1);
            nk_button_label(ctx, "确认挂号");
        }
        else if (current_page == 3) {
            nk_layout_row_dynamic(ctx, 50, 1);
            nk_text_heading(ctx, "统计分析");
            nk_layout_row_dynamic(ctx, 300, 1);
            nk_chart_floating(ctx, NK_CHART_LINE, out_data, 7, 0, 80);
        }

        nk_panel_end(ctx);
        nk_raylib_render(ctx, nk_rgba(245, 248, 252, 255));
        EndDrawing();
    }

    // 资源释放
    UnloadFont(font);
    nk_raylib_shutdown(ctx);
    CloseWindow();
    return 0;
}