#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * 模块：住院计费与押金流水（Billing）
 * 数据结构：双向链表，表头为 g_billing_head；节点类型为 Billing
 * 题签关键规则（实现约定）：
 *  - 每日扣费时间点：08:00（在 08:00 之前不扣“当天”费用）
 *  - 出院结算：若出院时间在 00:00-08:00，则不收当天费用（通过 cutoff_ord 处理）
 *  - 押金校验：押金需为 100 的整数倍，且不低于 200*N（N 为拟住院天数）
 *  - 日床位费：按病房类型基础费率 + 科室差异化费率（示例：外科/儿科/急诊上浮）
 *  - 押金余额：记录在 Inpatient.deposit；流水操作会同步更新该字段
 * 持久化：由 file.c 负责 load_billings/save_billings
 */

Billing *g_billing_head = NULL;

/**
 * 生成下一条计费流水ID（当前最大ID + 1）。
 */
static int billing_next_id(Billing *head) {
    int max_id = 0;
    for (Billing *cur = head; cur; cur = cur->next) {
        if (cur->id > max_id) max_id = cur->id;
    }
    return max_id + 1;
}

/**
 * 计费类型枚举转字符串（用于展示与落盘）。
 */
static const char* billing_type_to_string(BillingType t) {
    switch (t) {
        case BILL_DEPOSIT: return "DEPOSIT";
        case BILL_TOPUP:   return "TOPUP";
        case BILL_CHARGE:  return "CHARGE";
        case BILL_REFUND:  return "REFUND";
        default:           return "UNKNOWN";
    }
}

/**
 * 解析日期时间字符串。
 * @param s        输入字符串
 * @param y,m,d    输出年月日
 * @param hh,mm    输出时分（若无时间则默认 23:59）
 * @param has_time 输出：是否包含时间部分
 * @return 成功返回 1；失败返回 0
 * @note 支持：YYYY-MM-DD、YYYY-MM-DDTHH:MM、YYYY-MM-DD HH:MM
 */
static int parse_datetime(const char *s, int *y, int *m, int *d, int *hh, int *mm, int *has_time) {
    if (!s || !y || !m || !d || !hh || !mm || !has_time) return 0;

    *y = *m = *d = 0;
    *hh = 23;
    *mm = 59;
    *has_time = 0;

    // 支持：YYYY-MM-DD
    if (sscanf(s, "%d-%d-%d", y, m, d) != 3) return 0;

    // 支持：YYYY-MM-DDTHH:MM 或 YYYY-MM-DD HH:MM
    const char *p = strchr(s, 'T');
    if (!p) p = strchr(s, ' ');
    if (p) {
        int th = 0, tm = 0;
        if (sscanf(p + 1, "%d:%d", &th, &tm) == 2) {
            *hh = th;
            *mm = tm;
            *has_time = 1;
        }
    }

    return 1;
}

/**
 * 根据年月日构造 struct tm（时间固定 00:00:00）。
 */
static int make_tm_ymd(int y, int m, int d, struct tm *out) {
    if (!out) return 0;
    memset(out, 0, sizeof(*out));
    out->tm_year = y - 1900;
    out->tm_mon = m - 1;
    out->tm_mday = d;
    out->tm_hour = 0;
    out->tm_min = 0;
    out->tm_sec = 0;
    out->tm_isdst = -1;
    return 1;
}

/**
 * 将 YYYY-MM-DD 转为 ordinal（自 1970-01-01 起的天数），便于比较/累加。
 * @return 成功返回 1；失败返回 0
 */
static int ymd_to_ordinal(int y, int m, int d, int *ordinal) {
    if (!ordinal) return 0;
    if (y < 1970 || m < 1 || m > 12 || d < 1 || d > 31) return 0;
    struct tm t;
    if (!make_tm_ymd(y, m, d, &t)) return 0;
    time_t tt = mktime(&t);
    if (tt == (time_t)-1) return 0;
    *ordinal = (int)(tt / 86400);
    return 1;
}

/**
 * 将 ordinal 反解为 YYYY-MM-DD。
 */
static int ordinal_to_ymd(int ordinal, int *y, int *m, int *d) {
    if (!y || !m || !d) return 0;
    if (ordinal < 0) return 0;
    time_t tt = (time_t)ordinal * 86400;
    struct tm *pt = localtime(&tt);
    if (!pt) return 0;
    *y = pt->tm_year + 1900;
    *m = pt->tm_mon + 1;
    *d = pt->tm_mday;
    return 1;
}

/**
 * 格式化每日扣费的时间点字符串（固定为 08:00）。
 */
static void format_charge_datetime(int ordinal, char *buf, size_t buflen) {
    int y, m, d;
    if (!buf || buflen == 0) return;
    if (!ordinal_to_ymd(ordinal, &y, &m, &d)) {
        buf[0] = '\0';
        return;
    }
    // 计费点固定为 08:00
    snprintf(buf, buflen, "%04d-%02d-%02dT08:00", y, m, d);
}

/**
 * 将一条计费流水作用到住院记录余额（Inpatient.deposit）。
 * @note 本实现约定：DEPOSIT/TOPUP 增加余额；CHARGE/REFUND 减少余额。
 */
static int billing_apply_to_inpatient(Inpatient *ip, BillingType type, float amount) {
    if (!ip) return 0;
    if (amount < 0) return 0;

    switch (type) {
        case BILL_DEPOSIT:
        case BILL_TOPUP:
            ip->deposit += amount;
            return 1;
        case BILL_CHARGE:
        case BILL_REFUND:
            ip->deposit -= amount;
            return 1;
        default:
            return 0;
    }
}

/**
 * 创建计费流水节点。
 * @param id           流水ID
 * @param inpatient_id 住院ID
 * @param patient_name 患者姓名
 * @param ward_id      病房ID
 * @param type         流水类型（DEPOSIT/TOPUP/CHARGE/REFUND）
 * @param amount       金额（非负）
 * @param datetime     时间字符串（建议 YYYY-MM-DDTHH:MM）
 * @param note         备注
 * @return 成功返回新节点；失败返回 NULL
 */
Billing* billing_create(int id, int inpatient_id, const char *patient_name,
                        int ward_id, BillingType type, float amount,
                        const char *datetime, const char *note) {
    Billing *b = (Billing*)malloc(sizeof(Billing));
    if (!b) return NULL;
    b->id = id;
    b->inpatient_id = inpatient_id;
    if (patient_name) strcpy(b->patient_name, patient_name);
    else b->patient_name[0] = '\0';
    b->ward_id = ward_id;
    b->type = type;
    b->amount = amount;
    if (datetime) strncpy(b->datetime, datetime, sizeof(b->datetime) - 1);
    else b->datetime[0] = '\0';
    b->datetime[sizeof(b->datetime) - 1] = '\0';
    if (note) strncpy(b->note, note, sizeof(b->note) - 1);
    else b->note[0] = '\0';
    b->note[sizeof(b->note) - 1] = '\0';
    b->prev = b->next = NULL;
    return b;
}

/**
 * 插入计费流水节点到链表头部。
 */
int billing_insert(Billing **head, Billing *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

/**
 * 按流水ID查找。
 */
Billing* billing_find_by_id(Billing *head, int id) {
    for (Billing *cur = head; cur; cur = cur->next) {
        if (cur->id == id) return cur;
    }
    return NULL;
}

/**
 * 打印全部计费流水。
 * @return 打印条数
 */
int billing_print_all(Billing *head) {
    int count = 0;
    printf("计费流水列表：\n");
    for (Billing *cur = head; cur; cur = cur->next) {
        printf("ID:%d 住院ID:%d 患者:%s 病房ID:%d 类型:%s 金额:%.2f 时间:%s 备注:%s\n",
               cur->id, cur->inpatient_id, cur->patient_name, cur->ward_id,
               billing_type_to_string(cur->type), cur->amount, cur->datetime,
               cur->note[0] ? cur->note : "-");
        count++;
    }
    return count;
}

/**
 * 打印指定住院ID的计费流水。
 * @return 找到任一记录返回 1；否则返回 0
 */
int billing_print_by_inpatient(Billing *head, int inpatient_id) {
    int found = 0;
    printf("住院ID=%d 的计费流水：\n", inpatient_id);
    for (Billing *cur = head; cur; cur = cur->next) {
        if (cur->inpatient_id != inpatient_id) continue;
        printf("ID:%d 类型:%s 金额:%.2f 时间:%s 备注:%s\n",
               cur->id, billing_type_to_string(cur->type), cur->amount,
               cur->datetime, cur->note[0] ? cur->note : "-");
        found = 1;
    }
    return found;
}

/**
 * 释放计费流水链表。
 * @return 释放节点数
 */
int billing_free_all(Billing **head) {
    if (!head) return 0;
    int count = 0;
    Billing *cur = *head;
    while (cur) {
        Billing *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}

/**
 * 校验办理住院时的初始押金。
 * @param deposit      押金金额
 * @param planned_days 拟住院天数 N（若 <=0 则按 1）
 * @return 合法返回 1；否则返回 0
 * @note 题签规则：押金为 100 的整数倍，且不低于 200*N。
 */
int billing_validate_initial_deposit(float deposit, int planned_days) {
    if (planned_days <= 0) planned_days = 1;
    if (deposit <= 0) return 0;

    int dep_int = (int)(deposit + 0.5f);
    if ((float)dep_int != deposit) return 0; // 不是整数元
    if (dep_int % 100 != 0) return 0;
    if (dep_int < 200 * planned_days) return 0;
    return 1;
}

/**
 * 计算某病房的床位日费用（基础费率 + 科室差异化）。
 * @return 日费用（元/天）；失败返回 0
 */
float billing_daily_rate_by_ward(const Ward *w) {
    if (!w) return 0.0f;
    
    float base_rate = 180.0f;
    if (strcmp(w->type, "ICU") == 0) base_rate = 800.0f;
    else if (strcmp(w->type, "Single") == 0) base_rate = 300.0f;
    else if (strcmp(w->type, "Double") == 0) base_rate = 200.0f;
    else if (strcmp(w->type, "Triple") == 0) base_rate = 150.0f;

    // 科室差异化费率
    if (strstr(w->department, "外科") || strstr(w->department, "Surgery")) {
        base_rate *= 1.2f; // 外科上浮20%
    } else if (strstr(w->department, "儿科") || strstr(w->department, "Pediatrics")) {
        base_rate *= 1.1f; // 儿科上浮10%
    } else if (strstr(w->department, "急诊") || strstr(w->department, "Emergency")) {
        base_rate *= 1.5f; // 急诊上浮50%
    }

    return base_rate;
}

/**
 * 查找指定住院ID最后一次 CHARGE 流水对应的日期 ordinal。
 * @param out_ordinal 输出：最后扣费日的 ordinal
 * @return 找到返回 1；否则返回 0
 */
static int billing_find_last_charge_ordinal(Billing *head, int inpatient_id, int *out_ordinal) {
    if (!out_ordinal) return 0;
    int found = 0;
    int best = 0;

    for (Billing *cur = head; cur; cur = cur->next) {
        if (cur->inpatient_id != inpatient_id) continue;
        if (cur->type != BILL_CHARGE) continue;

        int y, m, d, hh, mm, has_time;
        if (!parse_datetime(cur->datetime, &y, &m, &d, &hh, &mm, &has_time)) continue;
        int ord;
        if (!ymd_to_ordinal(y, m, d, &ord)) continue;

        if (!found || ord > best) {
            best = ord;
            found = 1;
        }
    }

    if (!found) return 0;
    *out_ordinal = best;
    return 1;
}

/**
 * 住院办理押金入账（生成 DEPOSIT 流水并更新余额）。
 */
int billing_add_deposit(Billing **bhead, Inpatient *ip, int bill_id,
                        float amount, const char *datetime, const char *note) {
    if (!bhead || !ip || amount <= 0) return 0;
    if (bill_id <= 0) bill_id = billing_next_id(*bhead);
    Billing *b = billing_create(bill_id, ip->id, ip->patient_name, ip->ward_id,
                                BILL_DEPOSIT, amount, datetime, note);
    if (!b) return 0;
    if (!billing_insert(bhead, b)) {
        free(b);
        return 0;
    }
    (void)billing_apply_to_inpatient(ip, BILL_DEPOSIT, amount);
    return 1;
}

/**
 * 住院补缴押金（生成 TOPUP 流水并更新余额）。
 */
int billing_topup(Billing **bhead, Inpatient *ip, int bill_id,
                  float amount, const char *datetime, const char *note) {
    if (!bhead || !ip || amount <= 0) return 0;
    if (bill_id <= 0) bill_id = billing_next_id(*bhead);
    Billing *b = billing_create(bill_id, ip->id, ip->patient_name, ip->ward_id,
                                BILL_TOPUP, amount, datetime, note);
    if (!b) return 0;
    if (!billing_insert(bhead, b)) {
        free(b);
        return 0;
    }
    (void)billing_apply_to_inpatient(ip, BILL_TOPUP, amount);
    return 1;
}

/**
 * 自动补扣：从上次扣费日的下一天起，按“每日 08:00”规则一直扣到 now_datetime。
 * @param now_datetime 当前时间/结算截止时间
 * @param charged_days 输出：本次新增扣费天数（可为 NULL）
 * @param total_charged 输出：本次新增扣费金额（可为 NULL）
 * @return 成功返回 1；失败返回 0
 * @note 若 now_datetime 早于 08:00，则不扣当天费用（cutoff_ord=now_ord-1）。
 */
int billing_auto_charge_until(Billing **bhead, Inpatient *ip, const Ward *w,
                              const char *now_datetime, int *charged_days, float *total_charged) {
    if (charged_days) *charged_days = 0;
    if (total_charged) *total_charged = 0.0f;
    if (!bhead || !ip || !w || !now_datetime) return 0;

    int ny, nm, nd, nh, nmin, has_time;
    if (!parse_datetime(now_datetime, &ny, &nm, &nd, &nh, &nmin, &has_time)) return 0;

    int now_ord;
    if (!ymd_to_ordinal(ny, nm, nd, &now_ord)) return 0;

    // 每日08:00扣费：如果当前时间早于08:00，则不扣今天。
    int cutoff_ord = now_ord;
    if (has_time && nh < 8) cutoff_ord = now_ord - 1;

    int ay, am, ad, ah, amin, a_has_time;
    if (!parse_datetime(ip->admit_date, &ay, &am, &ad, &ah, &amin, &a_has_time)) return 0;

    int admit_ord;
    if (!ymd_to_ordinal(ay, am, ad, &admit_ord)) return 0;

    int start_ord = admit_ord;
    int last_charge_ord;
    if (billing_find_last_charge_ordinal(*bhead, ip->id, &last_charge_ord)) {
        start_ord = last_charge_ord + 1;
    }

    if (cutoff_ord < start_ord) return 1; // 无需扣费

    float daily_rate = billing_daily_rate_by_ward(w);
    if (daily_rate <= 0.0f) return 0;

    for (int ord = start_ord; ord <= cutoff_ord; ord++) {
        char dtbuf[24];
        format_charge_datetime(ord, dtbuf, sizeof(dtbuf));

        int bill_id = billing_next_id(*bhead);
        Billing *b = billing_create(bill_id, ip->id, ip->patient_name, ip->ward_id,
                                    BILL_CHARGE, daily_rate, dtbuf, "daily_charge");
        if (!b) return 0;

        if (!billing_insert(bhead, b)) {
            free(b);
            return 0;
        }

        (void)billing_apply_to_inpatient(ip, BILL_CHARGE, daily_rate);

        if (charged_days) (*charged_days)++;
        if (total_charged) (*total_charged) += daily_rate;

        if (ip->deposit < 0.0f) {
            fprintf(stderr, "提示：住院ID=%d 押金已不足（当前余额=%.2f），请补缴。\n", ip->id, ip->deposit);
        } else if (ip->deposit < 1000.0f) {
            fprintf(stderr, "提示：住院ID=%d 押金余额低于1000（当前=%.2f），建议补缴。\n", ip->id, ip->deposit);
        }
    }

    return 1;
}

/**
 * 出院结算：先自动补扣到出院时刻，再根据余额生成退费流水或给出欠费金额。
 * @param discharge_datetime 出院时间
 * @param total_cost 输出：本次结算新增扣费金额（可为 NULL）
 * @param refund    输出：退费金额（可为 NULL）
 * @param due       输出：欠费金额（可为 NULL）
 * @return 成功返回 1；失败返回 0
 */
int billing_settle_on_discharge(Billing **bhead, Inpatient *ip, const Ward *w,
                                const char *discharge_datetime, float *total_cost, float *refund, float *due) {
    if (total_cost) *total_cost = 0.0f;
    if (refund) *refund = 0.0f;
    if (due) *due = 0.0f;
    if (!bhead || !ip || !w || !discharge_datetime) return 0;

    int charged_days = 0;
    float charged_total = 0.0f;
    if (!billing_auto_charge_until(bhead, ip, w, discharge_datetime, &charged_days, &charged_total)) return 0;

    if (total_cost) *total_cost = charged_total;

    if (ip->deposit > 0.0f) {
        float r = ip->deposit;
        int bill_id = billing_next_id(*bhead);
        Billing *b = billing_create(bill_id, ip->id, ip->patient_name, ip->ward_id,
                                    BILL_REFUND, r, discharge_datetime, "refund_on_discharge");
        if (!b) return 0;
        if (!billing_insert(bhead, b)) {
            free(b);
            return 0;
        }
        (void)billing_apply_to_inpatient(ip, BILL_REFUND, r);
        if (refund) *refund = r;
    } else if (ip->deposit < 0.0f) {
        if (due) *due = -ip->deposit;
    }

    return 1;
}

/**
 * 结算预览（只读）：根据截至时间计算“预计还会扣多少天/多少费用”，不修改余额、不新增流水。
 * @return 成功返回 1；失败返回 0
 */
int billing_print_settlement(Billing *bhead, const Inpatient *ip, const Ward *w,
                             const char *as_of_datetime) {
    if (!ip || !w || !as_of_datetime) return 0;

    // 这里做一个“只读式”结算展示：不修改余额、不新增流水。
    int ny, nm, nd, nh, nmin, has_time;
    if (!parse_datetime(as_of_datetime, &ny, &nm, &nd, &nh, &nmin, &has_time)) return 0;

    int now_ord;
    if (!ymd_to_ordinal(ny, nm, nd, &now_ord)) return 0;

    int cutoff_ord = now_ord;
    if (has_time && nh < 8) cutoff_ord = now_ord - 1;

    int ay, am, ad, ah, amin, a_has_time;
    if (!parse_datetime(ip->admit_date, &ay, &am, &ad, &ah, &amin, &a_has_time)) return 0;

    int admit_ord;
    if (!ymd_to_ordinal(ay, am, ad, &admit_ord)) return 0;

    int last_charge_ord;
    int start_ord = admit_ord;
    if (billing_find_last_charge_ordinal(bhead, ip->id, &last_charge_ord)) {
        start_ord = last_charge_ord + 1;
    }

    int uncharged_days = 0;
    if (cutoff_ord >= start_ord) uncharged_days = cutoff_ord - start_ord + 1;

    float daily_rate = billing_daily_rate_by_ward(w);
    float will_charge = daily_rate * (float)uncharged_days;

    printf("====== 住院结算预览 ======\n");
    printf("住院ID: %d  患者: %s\n", ip->id, ip->patient_name);
    printf("病房ID: %d  病房类型: %s  日费用: %.2f\n", ip->ward_id, w->type, daily_rate);
    printf("入院时间: %s  结算截至: %s\n", ip->admit_date, as_of_datetime);
    printf("预计新增计费天数: %d  预计新增费用: %.2f\n", uncharged_days, will_charge);
    printf("当前押金余额: %.2f\n", ip->deposit);

    float after = ip->deposit - will_charge;
    if (after >= 0.0f) {
        printf("预计结算后余额: %.2f（可退费/留存）\n", after);
    } else {
        printf("预计结算后欠费: %.2f（需补缴）\n", -after);
    }
    printf("==========================\n");

    return 1;
}
