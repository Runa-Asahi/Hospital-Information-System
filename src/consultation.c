#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 模块：看诊记录（Consultation）
 * 数据结构：双向链表，表头为 g_consultation_head
 * 关键字段：诊断 diagnosis、处方 prescription
 * 题签扩展：支持“撤销+补录”流程（consultation_revoke）
 * 持久化：由 file.c 负责 load_consultations/save_consultations
 */

Consultation *g_consultation_head = NULL;

/* ==================== 基础功能：增删改查 ==================== */

/**
 * 创建看诊记录节点。
 * @param id           记录ID（唯一）
 * @param patient      患者姓名
 * @param doctor       医生姓名
 * @param date         看诊日期（YYYY-MM-DD）
 * @param diagnosis    诊断结果（建议不含空格，或使用下划线）
 * @param prescription 处方信息（建议不含空格，或使用下划线）
 * @return 成功返回新节点指针；失败返回 NULL
 */
Consultation* consultation_create(int id, const char *patient, const char *doctor,
                                  const char *date, const char *diagnosis, const char *prescription) {
                                    //diagnosis: 诊断结果；prescription：处方信息
    Consultation *c = (Consultation*)malloc(sizeof(Consultation));
    if (!c) return NULL;
    c->id = id;
    //strcpy函数用于字符串复制，确保目标数组有足够的空间来存储源字符串，并且源字符串以 null 结尾。
    //strcpy函数原型为：char *strcpy(char *dest, const char *src);
    strcpy(c->patient_name, patient);
    strcpy(c->doctor_name, doctor);
    strcpy(c->date, date);
    strcpy(c->diagnosis, diagnosis);
    strcpy(c->prescription, prescription);
    c->prev = c->next = NULL;
    return c;
}

int consultation_insert(Consultation **head, Consultation *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

int consultation_delete(Consultation **head, int id) {
    if (!head || !*head) return 0;
    Consultation *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;
    if (cur->prev) cur->prev->next = cur->next;
    else *head = cur->next;
    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

Consultation* consultation_find_by_id(Consultation *head, int id) {
    Consultation *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

int consultation_print_by_patient(Consultation *head, const char *patient) {
    if (!patient) return 0;
    Consultation *cur = head;
    int found = 0;
    printf("患者 %s 的看诊记录：\n", patient);
    while (cur) {
        if (strcmp(cur->patient_name, patient) == 0) {
            printf("ID: %d, 医生: %s, 日期: %s, 诊断: %s, 处方: %s\n",
                   cur->id, cur->doctor_name, cur->date, cur->diagnosis, cur->prescription);
            found = 1;
        }
        cur = cur->next;
    }
    if (!found) printf("未找到看诊记录。\n");
    return found;
}

/**
 * 打印全部看诊记录。
 * @return 打印的记录条数
 */
int consultation_print_all(Consultation *head) {
    int count = 0;
    printf("看诊记录列表：\n");
    for (Consultation *cur = head; cur; cur = cur->next) {
        printf("ID: %d, 患者: %s, 医生: %s, 日期: %s, 诊断: %s, 处方: %s\n",
               cur->id, cur->patient_name, cur->doctor_name, cur->date,
               cur->diagnosis, cur->prescription);
        count++;
    }
    return count;
}

/**
 * 撤销看诊记录（题签：撤销+补录）。
 * @note 本函数不物理删除旧记录，而是在字段中打上撤销标记，便于追溯。
 * @param head   链表头
 * @param id     需要撤销的记录ID
 * @param reason 撤销原因（非空）
 * @return 成功返回 1；失败返回 0
 */
int consultation_revoke(Consultation *head, int id, const char *reason) {
    if (!head || id <= 0 || !reason || reason[0] == '\0') return 0;
    Consultation *c = consultation_find_by_id(head, id);
    if (!c) return 0;

    char diag_buf[200];
    char pres_buf[200];

    // 预留空间，避免溢出；若内容过长则截断。
    snprintf(diag_buf, sizeof(diag_buf), "[REVOKED:%.64s] %.120s", reason, c->diagnosis);
    snprintf(pres_buf, sizeof(pres_buf), "[REVOKED] %.180s", c->prescription);
    strncpy(c->diagnosis, diag_buf, sizeof(c->diagnosis) - 1);
    c->diagnosis[sizeof(c->diagnosis) - 1] = '\0';
    strncpy(c->prescription, pres_buf, sizeof(c->prescription) - 1);
    c->prescription[sizeof(c->prescription) - 1] = '\0';
    return 1;
}

/**
 * 释放看诊记录链表全部节点。
 * @return 释放的节点数量
 */
int consultation_free_all(Consultation **head) {
    if (!head) return 0;
    Consultation *cur = *head;
    int count = 0;
    while (cur) {
        Consultation *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}