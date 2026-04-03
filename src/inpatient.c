#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 模块：住院记录（Inpatient）
 * 数据结构：双向链表，表头为 g_inpatient_head
 * 题签计费：押金余额 deposit 作为“余额字段”；具体缴费/扣费/退费落在计费流水 Billing
 * 关键规则：
 *  - 入院办理：押金需为 100 的整数倍，且不低于 200*N（N 为拟住院天数）
 *  - 出院结算：00:00-08:00 不收当天费；08:00 后计入当天（由 billing.c 负责）
 * 持久化：由 file.c 负责 load_inpatients/save_inpatients
 */

Inpatient *g_inpatient_head = NULL;

/**
 * 创建住院记录节点。
 * @param id             住院ID（唯一）
 * @param patient        患者姓名
 * @param ward_id        病房ID
 * @param bed_num        床位号（1..total_beds）
 * @param deposit        押金余额
 * @param admit_date     入院日期/时间字符串（YYYY-MM-DD 或带时分）
 * @param discharge_date 出院日期/时间字符串；为空串表示未出院
 * @return 成功返回新节点指针；失败返回 NULL
 */
Inpatient* inpatient_create(int id, const char *patient, int ward_id, int bed_num,
                            float deposit, const char *admit_date, const char *discharge_date) {
    Inpatient *ip = (Inpatient*)malloc(sizeof(Inpatient));
    if (!ip) return NULL;
    ip->id = id;
    strcpy(ip->patient_name, patient);
    ip->ward_id = ward_id;
    ip->bed_num = bed_num;
    ip->deposit = deposit;
    strcpy(ip->admit_date, admit_date);
    if (discharge_date && strlen(discharge_date) > 0)
        strcpy(ip->discharge_date, discharge_date);
    else
        ip->discharge_date[0] = '\0';   // 空串表示未出院
    ip->prev = ip->next = NULL;
    return ip;
}

/**
 * 生成下一住院ID（当前最大ID + 1）。
 */
int inpatient_next_id(Inpatient *head) {
    int max_id = 0;
    for (Inpatient *cur = head; cur; cur = cur->next) {
        if (cur->id > max_id) max_id = cur->id;
    }
    return max_id + 1;
}

static int inpatient_find_free_bed_num(Inpatient *head, int ward_id, int total_beds) {
    if (total_beds <= 0) return 0;

    // used[1..total_beds]
    unsigned char *used = (unsigned char*)calloc((size_t)total_beds + 1, 1);
    if (!used) return 0;

    for (Inpatient *cur = head; cur; cur = cur->next) {
        if (cur->ward_id != ward_id) continue;
        if (cur->discharge_date[0] != '\0') continue; // 已出院不占床
        if (cur->bed_num >= 1 && cur->bed_num <= total_beds) {
            used[cur->bed_num] = 1;
        }
    }

    int bed = 0;
    for (int i = 1; i <= total_beds; i++) {
        if (!used[i]) {
            bed = i;
            break;
        }
    }

    free(used);
    return bed;
}

int inpatient_admit_auto(Inpatient **ihead, Billing **bhead, Ward *ward_head,
                         int inpatient_id, const char *patient_name, const char *ward_type,
                         float deposit, int planned_days, const char *admit_datetime) {
    // 题签“一键办理住院”：择优选有空床的病房、自动分配床位号、押金规则校验、生成押金流水。
    if (!ihead || !bhead || !ward_head || !patient_name || !ward_type || !admit_datetime) return 0;

    if (!billing_validate_initial_deposit(deposit, planned_days)) {
        printf("办理住院失败：押金不符合规则（需为100元整数倍且不低于200*N）。\n");
        return 0;
    }

    Ward *w = ward_find_available_by_type(ward_head, ward_type, 1);
    if (!w) {
        printf("办理住院失败：未找到类型=%s 且有空床的病房。\n", ward_type);
        return 0;
    }

    int bed_num = inpatient_find_free_bed_num(*ihead, w->id, w->total_beds);
    if (bed_num <= 0) {
        printf("办理住院失败：病房ID=%d 已无可分配床位号。\n", w->id);
        return 0;
    }

    if (inpatient_id <= 0) inpatient_id = inpatient_next_id(*ihead);

    // Inpatient 的 admit_date 字段长度较短，这里截断保证安全。
    char admit_buf[20];
    strncpy(admit_buf, admit_datetime, sizeof(admit_buf) - 1);
    admit_buf[sizeof(admit_buf) - 1] = '\0';

    Inpatient *ip = inpatient_create(inpatient_id, patient_name, w->id, bed_num,
                                     0.0f, admit_buf, "");
    if (!ip) return 0;

    if (!inpatient_insert(ihead, ip)) {
        free(ip);
        return 0;
    }

    // 入院押金入账（同时会把押金余额加到住院记录上）
    if (!billing_add_deposit(bhead, ip, 0, deposit, admit_datetime, "admit_deposit")) {
        // 回滚：删除住院记录，并释放其占用床位
        inpatient_delete(ihead, inpatient_id);
        fprintf(stderr, "办理住院失败：押金流水创建失败，已回滚住院记录。\n");
        return 0;
    }

    printf("办理住院成功：住院ID=%d 患者=%s 病房ID=%d 类型=%s 床位=%d 押金=%.2f\n",
           ip->id, ip->patient_name, ip->ward_id, w->type, ip->bed_num, deposit);
    return 1;
}

/**
 * 插入住院记录节点到链表头部，并同步病房 occupied_beds。
 * @note 仅“未出院记录”会占用床位。
 */
int inpatient_insert(Inpatient **head, Inpatient *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;

    // 同步病房占用床位：未出院的住院记录会占用1张床。
    if (node->discharge_date[0] == '\0') {
        Ward *w = ward_find_by_id(g_ward_head, node->ward_id);
        if (w) {
            if (!ward_occupy_bed(w)) {
                fprintf(stderr, "警告：病房ID=%d 已无空床，无法为住院记录ID=%d 占床\n", node->ward_id, node->id);
            }
        } else {
            fprintf(stderr, "警告：未找到病房ID=%d，无法为住院记录ID=%d 占床\n", node->ward_id, node->id);
        }
    }
    return 1;
}

/**
 * 按住院ID删除记录，并在必要时释放床位。
 * @note 仅删除“未出院记录”会触发释放床位。
 */
int inpatient_delete(Inpatient **head, int id) {
    if (!head || !*head) return 0;
    Inpatient *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;

    // 删除未出院记录时，释放其占用的床位。
    if (cur->discharge_date[0] == '\0') {
        Ward *w = ward_find_by_id(g_ward_head, cur->ward_id);
        if (w) {
            if (!ward_release_bed(w)) {
                fprintf(stderr, "警告：病房ID=%d 释放床位失败（occupied_beds可能已为0）\n", cur->ward_id);
            }
        }
    }

    if (cur->prev) cur->prev->next = cur->next;
    else *head = cur->next;
    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

/**
 * 按住院ID查找。
 */
Inpatient* inpatient_find_by_id(Inpatient *head, int id) {
    Inpatient *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * 按患者姓名查找住院记录（返回第一个匹配）。
 */
Inpatient* inpatient_find_by_patient(Inpatient *head, const char *patient) {
    Inpatient *cur = head;
    while (cur) {
        if (strcmp(cur->patient_name, patient) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * 更新出院日期/时间。
 * @note 若从“未出院”变为“已出院”，会释放床位并触发出院结算（billing_settle_on_discharge）。
 */
int inpatient_update_discharge(Inpatient *ip, const char *discharge_date) {
    if (!ip || !discharge_date || discharge_date[0] == '\0') return 0;

    // 只有从“未出院”变为“已出院”时才释放床位，避免重复释放。
    int was_in_hospital = (ip->discharge_date[0] == '\0');
    strcpy(ip->discharge_date, discharge_date);
    if (was_in_hospital) {
        Ward *w = ward_find_by_id(g_ward_head, ip->ward_id);
        if (w) {
            if (!ward_release_bed(w)) {
                fprintf(stderr, "警告：病房ID=%d 释放床位失败（occupied_beds可能已为0）\n", ip->ward_id);
            }

            // 出院结算：按题签规则扣费到出院时点，并生成退费/欠费信息与流水。
            float total_cost = 0.0f;
            float refund = 0.0f;
            float due = 0.0f;
            if (billing_settle_on_discharge(&g_billing_head, ip, w, discharge_date, &total_cost, &refund, &due)) {
                printf("出院结算完成：住院ID=%d 费用合计=%.2f 退费=%.2f 欠费=%.2f\n",
                       ip->id, total_cost, refund, due);
            } else {
                fprintf(stderr, "警告：住院ID=%d 出院结算失败（计费模块）\n", ip->id);
            }
        }
    }
    return 1;
}

/**
 * 打印住院记录列表。
 * @return 打印的记录条数
 */
int inpatient_print_all(Inpatient *head) {
    Inpatient *cur = head;
    int count = 0;
    printf("住院记录列表：\n");
    while (cur) {
        printf("ID: %d, 患者: %s, 病房ID: %d, 床位: %d, 押金: %.2f, 入院: %s, 出院: %s\n",
               cur->id, cur->patient_name, cur->ward_id, cur->bed_num, cur->deposit,
               cur->admit_date, cur->discharge_date[0] ? cur->discharge_date : "未出院");
        cur = cur->next;
        count++;
    }
    return count;
}

/**
 * 释放住院记录链表全部节点。
 * @return 释放的节点数量
 */
int inpatient_free_all(Inpatient **head) {
    if (!head) return 0;
    Inpatient *cur = *head;
    int count = 0;
    while (cur) {
        Inpatient *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}