#include "his.h"
#include <stdio.h>
#include <string.h>

/*
 * 模块：扩展查询与统计报表（Report）
 * 作用：按题签要求提供患者维度历史、医生/科室维度汇总、时间范围全院汇总等输出
 * 时间口径：仅比较日期前 10 位（YYYY-MM-DD），忽略时分秒，便于兼容 YYYY-MM-DDTHH:MM
 * 依赖：使用各模块全局链表表头（g_*_head）进行只读统计，不修改业务数据
 */

/**
 * 截取日期字符串前 10 位（YYYY-MM-DD）。
 * @param src  输入日期/时间字符串
 * @param out10 输出缓冲区，长度至少 11（含结尾 \0）
 */
static void date10(const char *src, char out10[11]) {
    if (!out10) return;
    out10[0] = '\0';
    if (!src || src[0] == '\0') return;
    strncpy(out10, src, 10);
    out10[10] = '\0';
}

/**
 * 判断 date_str 是否落在 [start_ymd, end_ymd] 区间（按 YYYY-MM-DD 字符串比较）。
 * @note start/end 为空串或 NULL 表示不限制。
 */
static int date_in_range_ymd(const char *date_str, const char *start_ymd, const char *end_ymd) {
    char d[11];
    char s[11];
    char e[11];
    date10(date_str, d);
    date10(start_ymd, s);
    date10(end_ymd, e);

    if (d[0] == '\0') return 0;
    if (s[0] != '\0' && strncmp(d, s, 10) < 0) return 0;
    if (e[0] != '\0' && strncmp(d, e, 10) > 0) return 0;
    return 1;
}

/**
 * 打印“患者维度历史”：基础信息 + 挂号/看诊/检查 + 住院与计费流水。
 * @param patient_name 患者姓名
 * @return 成功返回 1；失败返回 0
 */
int his_print_patient_history(const char *patient_name) {
    if (!patient_name || patient_name[0] == '\0') return 0;

    printf("========== 患者维度历史 =========\n");
    printf("患者：%s\n", patient_name);

    Patient *p = patient_find_by_name(g_patient_head, patient_name);
    if (p) {
        printf("基础信息：ID=%d 年龄=%d 性别=%s\n", p->id, p->age, p->gender ? "男" : "女");
    } else {
        printf("提示：未找到该患者的基础信息（可能重名或未录入）。\n");
    }

    (void)register_print_by_patient(g_register_head, patient_name);
    (void)consultation_print_by_patient(g_consultation_head, patient_name);
    (void)examination_print_by_patient(g_examination_head, patient_name);

    int has_inpatient = 0;
    printf("患者 %s 的住院记录：\n", patient_name);
    for (Inpatient *ip = g_inpatient_head; ip; ip = ip->next) {
        if (strcmp(ip->patient_name, patient_name) != 0) continue;
        printf("住院ID=%d 病房ID=%d 床位=%d 押金余额=%.2f 入院=%s 出院=%s\n",
               ip->id, ip->ward_id, ip->bed_num, ip->deposit,
               ip->admit_date,
               ip->discharge_date[0] ? ip->discharge_date : "未出院");
        (void)billing_print_by_inpatient(g_billing_head, ip->id);
        has_inpatient = 1;
    }
    if (!has_inpatient) {
        printf("未找到住院记录。\n");
    }

    printf("================================\n");
    return 1;
}

/**
 * 打印“医生维度汇总”：挂号数量 + 看诊数量（可选按单日）。
 * @param doctor_id 医生工号
 * @param date_opt  可选日期（YYYY-MM-DD）；为空表示统计全部
 * @return 成功返回 1；失败返回 0
 */
int his_summary_by_doctor_id(int doctor_id, const char *date_opt) {
    if (doctor_id <= 0) return 0;
    Doctor *d = doctor_find_by_id(g_doctor_head, doctor_id);
    if (!d) {
        printf("未找到医生工号=%d\n", doctor_id);
        return 0;
    }

    int reg_count = 0;
    int cons_count = 0;

    if (date_opt && date_opt[0]) {
        reg_count = register_count_by_doctor_date(g_register_head, d->name, date_opt);
        for (Consultation *c = g_consultation_head; c; c = c->next) {
            if (strcmp(c->doctor_name, d->name) != 0) continue;
            if (strcmp(c->date, date_opt) != 0) continue;
            cons_count++;
        }
    } else {
        for (Register *r = g_register_head; r; r = r->next) {
            if (strcmp(r->doctor_name, d->name) == 0) reg_count++;
        }
        for (Consultation *c = g_consultation_head; c; c = c->next) {
            if (strcmp(c->doctor_name, d->name) == 0) cons_count++;
        }
    }

    printf("====== 医生维度汇总 ======\n");
    printf("医生：工号=%d 姓名=%s 科室=%s 职称=%s\n", d->id, d->name, d->department, d->title);
    if (date_opt && date_opt[0]) {
        printf("日期：%s\n", date_opt);
    } else {
        printf("日期：全部\n");
    }
    printf("挂号数量：%d\n", reg_count);
    printf("看诊数量：%d\n", cons_count);
    printf("==========================\n");

    return 1;
}

/**
 * 打印“科室维度汇总”：区间内挂号数量 + 看诊数量。
 * @param department_name 科室名称
 * @param start_date_opt  可选起始日期（YYYY-MM-DD）；为空表示不限制
 * @param end_date_opt    可选结束日期（YYYY-MM-DD）；为空表示不限制
 * @return 成功返回 1；失败返回 0
 */
int his_summary_by_department(const char *department_name, const char *start_date_opt, const char *end_date_opt) {
    if (!department_name || department_name[0] == '\0') return 0;

    int reg_count = 0;
    int cons_count = 0;

    for (Register *r = g_register_head; r; r = r->next) {
        if (!date_in_range_ymd(r->date, start_date_opt, end_date_opt)) continue;
        Doctor *doc = doctor_find_by_name(g_doctor_head, r->doctor_name);
        if (!doc) continue;
        if (strcmp(doc->department, department_name) != 0) continue;
        reg_count++;
    }

    for (Consultation *c = g_consultation_head; c; c = c->next) {
        if (!date_in_range_ymd(c->date, start_date_opt, end_date_opt)) continue;
        Doctor *doc = doctor_find_by_name(g_doctor_head, c->doctor_name);
        if (!doc) continue;
        if (strcmp(doc->department, department_name) != 0) continue;
        cons_count++;
    }

    printf("====== 科室维度汇总 ======\n");
    printf("科室：%s\n", department_name);
    if ((start_date_opt && start_date_opt[0]) || (end_date_opt && end_date_opt[0])) {
        printf("范围：%s ~ %s\n", (start_date_opt && start_date_opt[0]) ? start_date_opt : "-", (end_date_opt && end_date_opt[0]) ? end_date_opt : "-");
    } else {
        printf("范围：全部\n");
    }
    printf("挂号数量：%d\n", reg_count);
    printf("看诊数量：%d\n", cons_count);
    printf("==========================\n");

    return 1;
}

/**
 * 打印“时间范围全院汇总”：挂号/看诊/检查/入院/计费流水数量。
 * @param start_date 起始日期（YYYY-MM-DD）
 * @param end_date   结束日期（YYYY-MM-DD）
 * @return 成功返回 1；失败返回 0
 */
int his_summary_by_date_range(const char *start_date, const char *end_date) {
    if (!start_date || !end_date || start_date[0] == '\0' || end_date[0] == '\0') return 0;

    int reg_count = 0;
    int cons_count = 0;
    int exam_count = 0;
    int inpatient_admit_count = 0;
    int billing_count = 0;

    for (Register *r = g_register_head; r; r = r->next) {
        if (date_in_range_ymd(r->date, start_date, end_date)) reg_count++;
    }
    for (Consultation *c = g_consultation_head; c; c = c->next) {
        if (date_in_range_ymd(c->date, start_date, end_date)) cons_count++;
    }
    for (Examination *e = g_examination_head; e; e = e->next) {
        if (date_in_range_ymd(e->date, start_date, end_date)) exam_count++;
    }
    for (Inpatient *ip = g_inpatient_head; ip; ip = ip->next) {
        if (date_in_range_ymd(ip->admit_date, start_date, end_date)) inpatient_admit_count++;
    }
    for (Billing *b = g_billing_head; b; b = b->next) {
        if (date_in_range_ymd(b->datetime, start_date, end_date)) billing_count++;
    }

    printf("====== 时间范围全院汇总 ======\n");
    printf("范围：%s ~ %s\n", start_date, end_date);
    printf("挂号记录：%d\n", reg_count);
    printf("看诊记录：%d\n", cons_count);
    printf("检查记录：%d\n", exam_count);
    printf("入院记录：%d\n", inpatient_admit_count);
    printf("计费流水：%d\n", billing_count);
    printf("==============================\n");

    return 1;
}
