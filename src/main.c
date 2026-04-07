#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/*
 * 模块：控制台入口与菜单（main）
 * 作用：提供题签要求的交互式菜单，串联各模块增删改查、住院计费、撤销+补录、统计报表、保存退出
 * 数据来源：his_init("data") 从 data/ 目录下各类 .txt 文件加载到全局链表（g_*_head）
 * 保存方式：his_save("data") 写回 data/ 目录下各类 .txt 文件；退出时释放所有链表节点
 */

/**
 * 去除 fgets 读入的行尾换行符（\r/\n）。
 */
static void trim_newline(char *s) {// 从字符串末尾开始，逐个检查并去除 \r 或 \n
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

/**
 * 读取一行字符串输入。
 * @param prompt 提示语（可为 NULL）
 * @param buf    输出缓冲区
 * @param buflen 缓冲区长度
 * @return 成功返回 1；失败返回 0
 */
static int read_line(const char *prompt, char *buf, size_t buflen) {
    if (!buf || buflen == 0) return 0;
    if (prompt) printf("%s", prompt);
    if (!fgets(buf, (int)buflen, stdin)) return 0;
    trim_newline(buf);
    return 1;
}

/**
 * 读取一个 int（基于 read_line + strtol）。
 */
static int read_int(const char *prompt, int *out) {
    char line[64];
    if (!out) return 0;
    if (!read_line(prompt, line, sizeof(line))) return 0;
    char *end = NULL;
    long v = strtol(line, &end, 10);
    if (end == line) return 0;
    *out = (int)v;
    return 1;
}

/**
 * 读取一个 float（基于 read_line + strtod）。
 */
static int read_float(const char *prompt, float *out) {
    char line[64];
    if (!out) return 0;
    if (!read_line(prompt, line, sizeof(line))) return 0;
    char *end = NULL;
    float v = (float)strtod(line, &end);
    if (end == line) return 0;
    *out = v;
    return 1;
}

/**
 * 生成下一患者ID（当前最大ID + 1）。
 */
static int next_patient_id(void) {
    int max_id = 0;
    for (Patient *p = g_patient_head; p; p = p->next) if (p->id > max_id) max_id = p->id;
    return max_id + 1;
}

/**
 * 生成下一医生工号（当前最大ID + 1）。
 */
static int next_doctor_id(void) {
    int max_id = 0;
    for (Doctor *d = g_doctor_head; d; d = d->next) if (d->id > max_id) max_id = d->id;
    return max_id + 1;
}

/**
 * 生成下一科室ID（当前最大ID + 1）。
 */
static int next_department_id(void) {
    int max_id = 0;
    for (Department *d = g_department_head; d; d = d->next) if (d->id > max_id) max_id = d->id;
    return max_id + 1;
}

/**
 * 生成下一药品ID（当前最大ID + 1）。
 */
static int next_drug_id(void) {
    int max_id = 0;
    for (Drug *d = g_drug_head; d; d = d->next) if (d->id > max_id) max_id = d->id;
    return max_id + 1;
}

/**
 * 生成下一病房ID（当前最大ID + 1）。
 */
static int next_ward_id(void) {
    int max_id = 0;
    for (Ward *w = g_ward_head; w; w = w->next) if (w->id > max_id) max_id = w->id;
    return max_id + 1;
}

/**
 * 生成下一挂号记录ID（当前最大ID + 1）。
 */
static int next_register_id(void) {
    int max_id = 0;
    for (Register *r = g_register_head; r; r = r->next) if (r->id > max_id) max_id = r->id;
    return max_id + 1;
}

/**
 * 生成下一看诊记录ID（当前最大ID + 1）。
 */
static int next_consultation_id(void) {
    int max_id = 0;
    for (Consultation *c = g_consultation_head; c; c = c->next) if (c->id > max_id) max_id = c->id;
    return max_id + 1;
}

/**
 * 生成下一检查记录ID（当前最大ID + 1）。
 */
static int next_examination_id(void) {
    int max_id = 0;
    for (Examination *e = g_examination_head; e; e = e->next) if (e->id > max_id) max_id = e->id;
    return max_id + 1;
}

/**
 * 打印主菜单。
 */
static void menu_print(void) {
    printf("\n====== HIS 菜单 ======\n");
    printf("1. 打印全部基础信息\n");
    printf("2. 打印全部业务记录\n");
    printf("3. 新增基础信息（患者/医生/科室/药品/病房）\n");
    printf("4. 新增业务记录（挂号/看诊/检查/住院办理）\n");
    printf("5. 修改（患者/医生/出院结算）\n");
    printf("6. 查询（按ID/按名称/患者维度历史）\n");
    printf("7. 汇总统计（医生/科室/时间范围）\n");
    printf("8. 住院计费操作（补扣/补缴/结算预览/流水）\n");
    printf("9. 保存数据到 data/\n");
    printf("0. 退出\n");
    printf("======================\n");
}

/**
 * 新增基础信息子菜单：患者/医生/科室/药品/病房。
 */
static void add_basic(void) {
    int sub = 0;
    printf("\n-- 新增基础信息 --\n");
    printf("1) 患者  2) 医生  3) 科室  4) 药品  5) 病房  0) 返回\n");
    if (!read_int("选择：", &sub)) return;

    if (sub == 1) {
        char name[50];
        int age = 0;
        int gender = 0;
        if (!read_line("姓名：", name, sizeof(name))) return;
        if (!read_int("年龄：", &age)) return;
        if (!read_int("性别(0女1男)：", &gender)) return;
        int id = next_patient_id();
        Patient *p = patient_create(id, name, age, gender ? 1 : 0);
        if (!p) {
            printf("新增失败：内存不足\n");
            return;
        }
        patient_insert(&g_patient_head, p);
        printf("新增患者成功：ID=%d\n", id);
    } else if (sub == 2) {
        char name[50], dept[50], title[20], schedule[100];
        int age = 0;
        int gender = 0;
        if (!read_line("姓名：", name, sizeof(name))) return;
        if (!read_int("年龄：", &age)) return;
        if (!read_int("性别(0女1男)：", &gender)) return;
        if (!read_line("所属科室：", dept, sizeof(dept))) return;
        if (!read_line("职称：", title, sizeof(title))) return;
        if (!read_line("出诊时间：", schedule, sizeof(schedule))) return;
        int id = next_doctor_id();
        Doctor *d = doctor_create(id, name, age, gender ? 1 : 0, dept, title, schedule);
        if (!d) {
            printf("新增失败：内存不足\n");
            return;
        }
        doctor_insert(&g_doctor_head, d);
        printf("新增医生成功：工号=%d\n", id);
    } else if (sub == 3) {
        char name[50];
        if (!read_line("科室名称：", name, sizeof(name))) return;
        int id = next_department_id();
        Department *d = department_create(id, name);
        if (!d) {
            printf("新增失败：内存不足\n");
            return;
        }
        department_insert(&g_department_head, d);
        printf("新增科室成功：ID=%d\n", id);
    } else if (sub == 4) {
        char name[50], alias[50], dept[50];
        int stock = 0;
        float price = 0.0f;
        if (!read_line("通用名：", name, sizeof(name))) return;
        if (!read_line("别名：", alias, sizeof(alias))) return;
        if (!read_int("库存：", &stock)) return;
        if (!read_float("单价：", &price)) return;
        if (!read_line("关联科室：", dept, sizeof(dept))) return;
        int id = next_drug_id();
        Drug *dr = drug_create(id, name, alias, stock, price, dept);
        if (!dr) {
            printf("新增失败：内存不足\n");
            return;
        }
        drug_insert(&g_drug_head, dr);
        printf("新增药品成功：ID=%d\n", id);
    } else if (sub == 5) {
        char type[20], dept[50];
        int total_beds = 0;
        int can_convert = 0;
        if (!read_line("病房类型(Single/Double/Triple/ICU)：", type, sizeof(type))) return;
        if (!read_int("总床位：", &total_beds)) return;
        if (!read_line("关联科室：", dept, sizeof(dept))) return;
        if (!read_int("是否可转换(0否1是)：", &can_convert)) return;
        int id = next_ward_id();
        Ward *w = ward_create(id, type, total_beds, dept, can_convert);
        if (!w) {
            printf("新增失败：内存不足\n");
            return;
        }
        ward_insert(&g_ward_head, w);
        printf("新增病房成功：ID=%d\n", id);
    }
}

/**
 * 新增业务记录子菜单：挂号/看诊/检查/一键办理住院。
 */
static void add_business(void) {
    int sub = 0;
    printf("\n-- 新增业务记录 --\n");
    printf("1) 挂号  2) 看诊  3) 检查  4) 办理住院(一键)  0) 返回\n");
    if (!read_int("选择：", &sub)) return;

    if (sub == 1) {
        char patient[50], doctor[50], date[20];
        if (!read_line("患者姓名：", patient, sizeof(patient))) return;
        if (!read_line("医生姓名：", doctor, sizeof(doctor))) return;
        if (!read_line("挂号日期(YYYY-MM-DD)：", date, sizeof(date))) return;
        int id = next_register_id();
        Register *r = register_create(id, patient, doctor, date);
        if (!r) {
            printf("新增失败：内存不足\n");
            return;
        }
        register_insert(&g_register_head, r);
        printf("新增挂号成功：ID=%d\n", id);
    } else if (sub == 2) {
        char patient[50], doctor[50], date[20], diag[200], pres[200];
        if (!read_line("患者姓名：", patient, sizeof(patient))) return;
        if (!read_line("医生姓名：", doctor, sizeof(doctor))) return;
        if (!read_line("看诊日期(YYYY-MM-DD)：", date, sizeof(date))) return;
        if (!read_line("诊断结果(建议无空格或用下划线)：", diag, sizeof(diag))) return;
        if (!read_line("处方信息(建议无空格或用下划线)：", pres, sizeof(pres))) return;
        int id = next_consultation_id();
        Consultation *c = consultation_create(id, patient, doctor, date, diag, pres);
        if (!c) {
            printf("新增失败：内存不足\n");
            return;
        }
        consultation_insert(&g_consultation_head, c);
        printf("新增看诊成功：ID=%d\n", id);
    } else if (sub == 3) {
        char patient[50], item[50], date[20];
        float cost = 0.0f;
        if (!read_line("患者姓名：", patient, sizeof(patient))) return;
        if (!read_line("检查项目：", item, sizeof(item))) return;
        if (!read_float("检查费用：", &cost)) return;
        if (!read_line("检查日期(YYYY-MM-DD)：", date, sizeof(date))) return;
        int id = next_examination_id();
        Examination *e = examination_create(id, patient, item, cost, date);
        if (!e) {
            printf("新增失败：内存不足\n");
            return;
        }
        examination_insert(&g_examination_head, e);
        printf("新增检查成功：ID=%d\n", id);
    } else if (sub == 4) {
        char patient[50], ward_type[20], admit_dt[24];
        float deposit = 0.0f;
        int planned_days = 0;
        if (!read_line("患者姓名：", patient, sizeof(patient))) return;
        if (!read_line("病房类型(Single/Double/Triple/ICU)：", ward_type, sizeof(ward_type))) return;
        if (!read_int("拟住院天数N：", &planned_days)) return;
        if (!read_float("押金金额：", &deposit)) return;
        if (!read_line("入院时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", admit_dt, sizeof(admit_dt))) return;
        (void)inpatient_admit_auto(&g_inpatient_head, &g_billing_head, g_ward_head,
                                  0, patient, ward_type, deposit, planned_days, admit_dt);
    }
}

/**
 * 修改子菜单：患者/医生/办理出院结算/撤销+补录看诊/删除记录。
 */
static void modify_menu(void) {
    int sub = 0;
    printf("\n-- 修改 --\n");
    printf("1) 修改患者  2) 修改医生  3) 办理出院(结算)\n");
    printf("4) 撤销+补录看诊  5) 删除记录  0) 返回\n");
    if (!read_int("选择：", &sub)) return;

    if (sub == 1) {
        int id = 0;
        if (!read_int("患者ID：", &id)) return;
        Patient *p = patient_find_by_id(g_patient_head, id);
        if (!p) {
            printf("未找到患者\n");
            return;
        }
        char name[50];
        int age = 0;
        int gender = 0;
        if (!read_line("新姓名：", name, sizeof(name))) return;
        if (!read_int("新年龄：", &age)) return;
        if (!read_int("新性别(0女1男)：", &gender)) return;
        patient_update(p, name, age, gender ? 1 : 0);
        printf("修改成功\n");
    } else if (sub == 2) {
        int id = 0;
        if (!read_int("医生工号：", &id)) return;
        Doctor *d = doctor_find_by_id(g_doctor_head, id);
        if (!d) {
            printf("未找到医生\n");
            return;
        }
        char name[50], dept[50], title[20], schedule[100];
        int age = 0;
        int gender = 0;
        if (!read_line("新姓名：", name, sizeof(name))) return;
        if (!read_int("新年龄：", &age)) return;
        if (!read_int("新性别(0女1男)：", &gender)) return;
        if (!read_line("新科室：", dept, sizeof(dept))) return;
        if (!read_line("新职称：", title, sizeof(title))) return;
        if (!read_line("新出诊时间：", schedule, sizeof(schedule))) return;
        doctor_update(d, name, age, gender ? 1 : 0, dept, title, schedule);
        printf("修改成功\n");
    } else if (sub == 3) {
        int inpatient_id = 0;
        char discharge_dt[24];
        if (!read_int("住院ID：", &inpatient_id)) return;
        Inpatient *ip = inpatient_find_by_id(g_inpatient_head, inpatient_id);
        if (!ip) {
            printf("未找到住院记录\n");
            return;
        }
        if (!read_line("出院时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", discharge_dt, sizeof(discharge_dt))) return;
        (void)inpatient_update_discharge(ip, discharge_dt);
    } else if (sub == 4) {
        /* 题签：规范化修改流程——先撤销旧记录，再补录新记录。 */
        int old_id = 0;
        char reason[80];
        if (!read_int("撤销的看诊记录ID：", &old_id)) return;
        if (!read_line("撤销原因：", reason, sizeof(reason))) return;
        if (!consultation_revoke(g_consultation_head, old_id, reason)) {
            printf("撤销失败：未找到记录或输入无效\n");
            return;
        }
        printf("撤销完成，请继续补录新记录。\n");

        char patient[50], doctor[50], date[20], diag[200], pres[200];
        if (!read_line("患者姓名：", patient, sizeof(patient))) return;
        if (!read_line("医生姓名：", doctor, sizeof(doctor))) return;
        if (!read_line("看诊日期(YYYY-MM-DD)：", date, sizeof(date))) return;
        if (!read_line("诊断结果(建议无空格或用下划线)：", diag, sizeof(diag))) return;
        if (!read_line("处方信息(建议无空格或用下划线)：", pres, sizeof(pres))) return;

        int new_id = next_consultation_id();
        Consultation *c = consultation_create(new_id, patient, doctor, date, diag, pres);
        if (!c) {
            printf("补录失败：内存不足\n");
            return;
        }
        consultation_insert(&g_consultation_head, c);
        printf("补录成功：新看诊记录ID=%d\n", new_id);
    } else if (sub == 5) {
        int del = 0;
        printf("\n-- 删除记录 --\n");
        printf("1) 患者 2) 医生 3) 科室 4) 药品 5) 病房\n");
        printf("6) 挂号 7) 看诊 8) 检查 9) 住院 0) 返回\n");
        if (!read_int("选择：", &del)) return;
        if (del == 0) return;
        int id = 0;
        if (!read_int("ID：", &id)) return;

        int ok = 0;
        switch (del) {
            case 1: ok = patient_delete(&g_patient_head, id); break;
            case 2: ok = doctor_delete(&g_doctor_head, id); break;
            case 3: ok = department_delete(&g_department_head, id); break;
            case 4: ok = drug_delete(&g_drug_head, id); break;
            case 5: ok = ward_delete(&g_ward_head, id); break;
            case 6: ok = register_delete(&g_register_head, id); break;
            case 7: ok = consultation_delete(&g_consultation_head, id); break;
            case 8: ok = examination_delete(&g_examination_head, id); break;
            case 9:
                ok = inpatient_delete(&g_inpatient_head, id);
                if (ok) {
                    printf("提示：相关计费流水不会自动删除，可按住院ID查询核对。\n");
                }
                break;
            default: break;
        }
        printf(ok ? "删除成功\n" : "删除失败（未找到或条件不满足）\n");
    }
}

/**
 * 查询子菜单：按ID/按名称查询 + 患者维度历史。
 */
static void query_menu(void) {
    int sub = 0;
    printf("\n-- 查询 --\n");
    printf("1) 按ID查患者  2) 按ID查医生  3) 按ID查科室  4) 按ID查药品\n");
    printf("5) 按ID查病房  6) 按ID查挂号  7) 按ID查看诊  8) 按ID查检查\n");
    printf("9) 按ID查住院  10) 按ID查计费流水\n");
    printf("11) 按名称查患者(唯一)  12) 按名称查医生(唯一)  13) 按名称查科室(唯一)  14) 按名称查药品(唯一)\n");
    printf("15) 患者维度历史  0) 返回\n");
    if (!read_int("选择：", &sub)) return;

    if (sub == 1) {
        int id = 0;
        if (!read_int("患者ID：", &id)) return;
        Patient *p = patient_find_by_id(g_patient_head, id);
        if (!p) printf("未找到\n");
        else printf("ID=%d 姓名=%s 年龄=%d 性别=%s\n", p->id, p->name, p->age, p->gender ? "男" : "女");
    } else if (sub == 2) {
        int id = 0;
        if (!read_int("医生工号：", &id)) return;
        Doctor *d = doctor_find_by_id(g_doctor_head, id);
        if (!d) printf("未找到\n");
        else printf("工号=%d 姓名=%s 科室=%s 职称=%s 出诊=%s\n", d->id, d->name, d->department, d->title, d->schedule);
    } else if (sub == 3) {
        int id = 0;
        if (!read_int("科室ID：", &id)) return;
        Department *dep = department_find_by_id(g_department_head, id);
        if (!dep) printf("未找到\n");
        else printf("ID=%d 名称=%s\n", dep->id, dep->name);
    } else if (sub == 4) {
        int id = 0;
        if (!read_int("药品ID：", &id)) return;
        Drug *dr = drug_find_by_id(g_drug_head, id);
        if (!dr) printf("未找到\n");
        else printf("ID=%d 名称=%s 别名=%s 库存=%d 单价=%.2f 科室=%s\n",
                    dr->id, dr->name, dr->alias, dr->stock, dr->price, dr->department);
    } else if (sub == 5) {
        int id = 0;
        if (!read_int("病房ID：", &id)) return;
        Ward *w = ward_find_by_id(g_ward_head, id);
        if (!w) printf("未找到\n");
        else printf("ID=%d 类型=%s 总床位=%d 已占=%d 科室=%s 可转换=%s\n",
                    w->id, w->type, w->total_beds, w->occupied_beds, w->department, w->can_convert ? "是" : "否");
    } else if (sub == 6) {
        int id = 0;
        if (!read_int("挂号ID：", &id)) return;
        Register *r = register_find_by_id(g_register_head, id);
        if (!r) printf("未找到\n");
        else printf("ID=%d 患者=%s 医生=%s 日期=%s\n", r->id, r->patient_name, r->doctor_name, r->date);
    } else if (sub == 7) {
        int id = 0;
        if (!read_int("看诊ID：", &id)) return;
        Consultation *c = consultation_find_by_id(g_consultation_head, id);
        if (!c) printf("未找到\n");
        else printf("ID=%d 患者=%s 医生=%s 日期=%s 诊断=%s 处方=%s\n",
                    c->id, c->patient_name, c->doctor_name, c->date, c->diagnosis, c->prescription);
    } else if (sub == 8) {
        int id = 0;
        if (!read_int("检查ID：", &id)) return;
        Examination *e = examination_find_by_id(g_examination_head, id);
        if (!e) printf("未找到\n");
        else printf("ID=%d 患者=%s 项目=%s 费用=%.2f 日期=%s\n", e->id, e->patient_name, e->item, e->cost, e->date);
    } else if (sub == 9) {
        int id = 0;
        if (!read_int("住院ID：", &id)) return;
        Inpatient *ip = inpatient_find_by_id(g_inpatient_head, id);
        if (!ip) printf("未找到\n");
        else printf("住院ID=%d 患者=%s 病房ID=%d 床位=%d 押金余额=%.2f 入院=%s 出院=%s\n",
                    ip->id, ip->patient_name, ip->ward_id, ip->bed_num, ip->deposit,
                    ip->admit_date, ip->discharge_date[0] ? ip->discharge_date : "未出院");
    } else if (sub == 10) {
        int id = 0;
        if (!read_int("计费流水ID：", &id)) return;
        Billing *b = billing_find_by_id(g_billing_head, id);
        if (!b) printf("未找到\n");
        else printf("ID=%d 住院ID=%d 患者=%s 病房ID=%d 金额=%.2f 时间=%s 备注=%s\n",
                    b->id, b->inpatient_id, b->patient_name, b->ward_id, b->amount,
                    b->datetime, b->note[0] ? b->note : "-");
    } else if (sub == 11) {
        char name[50];
        if (!read_line("患者姓名：", name, sizeof(name))) return;
        Patient *p = patient_find_by_name(g_patient_head, name);
        if (!p) printf("未找到或重名（请用ID）\n");
        else printf("ID=%d 姓名=%s 年龄=%d 性别=%s\n", p->id, p->name, p->age, p->gender ? "男" : "女");
    } else if (sub == 12) {
        char name[50];
        if (!read_line("医生姓名：", name, sizeof(name))) return;
        Doctor *d = doctor_find_by_name(g_doctor_head, name);
        if (!d) printf("未找到或重名（请用工号）\n");
        else printf("工号=%d 姓名=%s 科室=%s 职称=%s\n", d->id, d->name, d->department, d->title);
    } else if (sub == 13) {
        char name[50];
        if (!read_line("科室名称：", name, sizeof(name))) return;
        Department *dep = department_find_by_name(g_department_head, name);
        if (!dep) printf("未找到或重名（请用ID）\n");
        else printf("ID=%d 名称=%s\n", dep->id, dep->name);
    } else if (sub == 14) {
        char name[50];
        if (!read_line("药品通用名：", name, sizeof(name))) return;
        Drug *dr = drug_find_by_name(g_drug_head, name);
        if (!dr) printf("未找到或重名（请用ID）\n");
        else printf("ID=%d 名称=%s 别名=%s 库存=%d 单价=%.2f 科室=%s\n",
                    dr->id, dr->name, dr->alias, dr->stock, dr->price, dr->department);
    } else if (sub == 15) {
        char name[50];
        if (!read_line("患者姓名：", name, sizeof(name))) return;
        (void)his_print_patient_history(name);
    }
}

/**
 * 汇总统计子菜单：医生维度/科室维度/时间范围全院。
 */
static void summary_menu(void) {
    int sub = 0;
    printf("\n-- 汇总统计 --\n");
    printf("1) 医生维度(按工号)  2) 科室维度  3) 时间范围全院  0) 返回\n");
    if (!read_int("选择：", &sub)) return;

    if (sub == 1) {
        int id = 0;
        char date[20];
        if (!read_int("医生工号：", &id)) return;
        if (!read_line("日期(YYYY-MM-DD，留空统计全部)：", date, sizeof(date))) return;
        (void)his_summary_by_doctor_id(id, date[0] ? date : NULL);
    } else if (sub == 2) {
        char dept[50], start[20], end[20];
        if (!read_line("科室名称：", dept, sizeof(dept))) return;
        if (!read_line("开始日期(YYYY-MM-DD，留空不限制)：", start, sizeof(start))) return;
        if (!read_line("结束日期(YYYY-MM-DD，留空不限制)：", end, sizeof(end))) return;
        (void)his_summary_by_department(dept, start[0] ? start : NULL, end[0] ? end : NULL);
    } else if (sub == 3) {
        char start[20], end[20];
        if (!read_line("开始日期(YYYY-MM-DD)：", start, sizeof(start))) return;
        if (!read_line("结束日期(YYYY-MM-DD)：", end, sizeof(end))) return;
        (void)his_summary_by_date_range(start, end);
    }
}

/**
 * 住院计费操作子菜单：补扣/补缴/结算预览/流水打印。
 */
static void billing_menu(void) {
    int sub = 0;
    printf("\n-- 住院计费操作 --\n");
    printf("1) 补扣到时间点  2) 补缴押金  3) 结算预览  4) 打印计费流水(全部/按住院)  0) 返回\n");
    if (!read_int("选择：", &sub)) return;

    if (sub == 1) {
        int inpatient_id = 0;
        char now_dt[24];
        if (!read_int("住院ID：", &inpatient_id)) return;
        Inpatient *ip = inpatient_find_by_id(g_inpatient_head, inpatient_id);
        if (!ip) {
            printf("未找到住院记录\n");
            return;
        }
        Ward *w = ward_find_by_id(g_ward_head, ip->ward_id);
        if (!w) {
            printf("未找到病房\n");
            return;
        }
        if (!read_line("截至时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", now_dt, sizeof(now_dt))) return;
        int days = 0;
        float total = 0.0f;
        if (billing_auto_charge_until(&g_billing_head, ip, w, now_dt, &days, &total)) {
            printf("补扣完成：新增扣费天数=%d 新增费用=%.2f 当前押金余额=%.2f\n", days, total, ip->deposit);
        } else {
            printf("补扣失败\n");
        }
    } else if (sub == 2) {
        int inpatient_id = 0;
        char dt[24];
        float amount = 0.0f;
        if (!read_int("住院ID：", &inpatient_id)) return;
        Inpatient *ip = inpatient_find_by_id(g_inpatient_head, inpatient_id);
        if (!ip) {
            printf("未找到住院记录\n");
            return;
        }
        if (!read_float("补缴金额：", &amount)) return;
        if (!read_line("时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", dt, sizeof(dt))) return;
        if (billing_topup(&g_billing_head, ip, 0, amount, dt, "manual_topup")) {
            printf("补缴成功：当前押金余额=%.2f\n", ip->deposit);
        } else {
            printf("补缴失败\n");
        }
    } else if (sub == 3) {
        int inpatient_id = 0;
        char asof[24];
        if (!read_int("住院ID：", &inpatient_id)) return;
        Inpatient *ip = inpatient_find_by_id(g_inpatient_head, inpatient_id);
        if (!ip) {
            printf("未找到住院记录\n");
            return;
        }
        Ward *w = ward_find_by_id(g_ward_head, ip->ward_id);
        if (!w) {
            printf("未找到病房\n");
            return;
        }
        if (!read_line("截至时间(YYYY-MM-DD 或 YYYY-MM-DDTHH:MM)：", asof, sizeof(asof))) return;
        (void)billing_print_settlement(g_billing_head, ip, w, asof);
    } else if (sub == 4) {
        int which = 0;
        printf("1) 打印全部流水  2) 按住院ID打印\n");
        if (!read_int("选择：", &which)) return;
        if (which == 1) {
            (void)billing_print_all(g_billing_head);
        } else if (which == 2) {
            int inpatient_id = 0;
            if (!read_int("住院ID：", &inpatient_id)) return;
            (void)billing_print_by_inpatient(g_billing_head, inpatient_id);
        }
    }
}

static void release_all_lists(void) {
    patient_free_all(&g_patient_head);
    doctor_free_all(&g_doctor_head);
    department_free_all(&g_department_head);
    drug_free_all(&g_drug_head);
    ward_free_all(&g_ward_head);
    register_free_all(&g_register_head);
    consultation_free_all(&g_consultation_head);
    examination_free_all(&g_examination_head);
    inpatient_free_all(&g_inpatient_head);
    billing_free_all(&g_billing_head);
}

void his_release_all(void) {
    release_all_lists();
}

int his_console_run_operation(int op) {
    switch (op) {
        case 1:
            (void)patient_print_all(g_patient_head);
            (void)doctor_print_all(g_doctor_head);
            (void)department_print_all(g_department_head);
            (void)drug_print_all(g_drug_head);
            (void)ward_print_all(g_ward_head);
            return 1;
        case 2:
            (void)register_print_all(g_register_head);
            (void)consultation_print_all(g_consultation_head);
            (void)examination_print_all(g_examination_head);
            (void)inpatient_print_all(g_inpatient_head);
            (void)billing_print_all(g_billing_head);
            return 1;
        case 3:
            add_basic();
            return 1;
        case 4:
            add_business();
            return 1;
        case 5:
            modify_menu();
            return 1;
        case 6:
            query_menu();
            return 1;
        case 7:
            summary_menu();
            return 1;
        case 8:
            billing_menu();
            return 1;
        case 9:
            if (!his_save("data")) printf("保存失败\n");
            else printf("保存完成\n");
            return 1;
        default:
            printf("无效选择\n");
            return 0;
    }
}

int his_console_main(void) {
    for (;;) {
        int op = -1;
        menu_print();
        if (!read_int("选择：", &op)) continue;

        if (op == 0) {
            char ans[8];
            if (read_line("退出前是否保存数据？(y/n)：", ans, sizeof(ans)) && (ans[0] == 'y' || ans[0] == 'Y')) {
                if (!his_save("data")) printf("保存失败\n");
                else printf("保存完成\n");
            }
            break;
        }

        (void)his_console_run_operation(op);
    }
    return 0;
}

/**
 * 程序入口：默认启动图形化主界面；如需原控制台菜单，使用参数 --console。
 */
int main(int argc, char **argv) {
    int run_console = 0;
    if (argc >= 2 && argv && argv[1] && (strcmp(argv[1], "--console") == 0 || strcmp(argv[1], "-c") == 0)) {
        run_console = 1;
    }

#ifdef _WIN32
    if (run_console && !GetConsoleWindow()) {
        (void)AllocConsole();
        (void)freopen("CONIN$", "r", stdin);
        (void)freopen("CONOUT$", "w", stdout);
        (void)freopen("CONOUT$", "w", stderr);
    }
#endif

    if (run_console) {
        (void)his_console_init();
    }

    int init_ok = his_init("data");
    if (!init_ok) {
        if (run_console) {
            printf("提示：初始化加载未完全成功（可能存在缺失文件），可继续运行。\n");
        }
#ifdef _WIN32
        else {
            MessageBoxA(NULL, "Init loading was not fully successful.\nSome data files may be missing.", "HIS", MB_OK | MB_ICONWARNING);
        }
#endif
    }

    int rc = 0;
    if (run_console) {
        rc = his_console_main();
    } else {
        rc = his_gui_main();
    }

    release_all_lists();
    return rc;
}
