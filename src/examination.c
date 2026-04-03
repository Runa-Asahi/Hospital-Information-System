#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 模块：检查记录（Examination）
 * 数据结构：双向链表，表头为 g_examination_head
 * 字段：检查项目 item、费用 cost、日期 date
 * 持久化：由 file.c 负责 load_examinations/save_examinations
 */

Examination *g_examination_head = NULL;

/**
 * 创建检查记录节点。
 * @param id      记录ID（唯一）
 * @param patient 患者姓名
 * @param item    检查项目
 * @param cost    检查费用
 * @param date    检查日期（YYYY-MM-DD）
 * @return 成功返回新节点指针；失败返回 NULL
 */
Examination* examination_create(int id, const char *patient, const char *item, float cost, const char *date) {
    Examination *e = (Examination*)malloc(sizeof(Examination));
    if (!e) return NULL;
    e->id = id;
    strcpy(e->patient_name, patient);
    strcpy(e->item, item);
    e->cost = cost;
    strcpy(e->date, date);
    e->prev = e->next = NULL;
    return e;
}

/**
 * 插入检查记录节点到链表头部。
 * @return 成功返回 1；失败返回 0
 */
int examination_insert(Examination **head, Examination *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

/**
 * 按记录ID删除检查记录。
 * @return 成功返回 1；未找到/参数无效返回 0
 */
int examination_delete(Examination **head, int id) {
    if (!head || !*head) return 0;
    Examination *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;
    if (cur->prev) cur->prev->next = cur->next;
    else *head = cur->next;
    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

/**
 * 按记录ID查找。
 * @return 找到返回指针；否则返回 NULL
 */
Examination* examination_find_by_id(Examination *head, int id) {
    Examination *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * 打印某患者的检查记录。
 * @return 若找到至少一条返回 1；否则返回 0
 */
int examination_print_by_patient(Examination *head, const char *patient) {
    if (!patient) return 0;
    Examination *cur = head;
    int found = 0;
    printf("患者 %s 的检查记录：\n", patient);
    while (cur) {
        if (strcmp(cur->patient_name, patient) == 0) {
            printf("ID: %d, 项目: %s, 费用: %.2f, 日期: %s\n",
                   cur->id, cur->item, cur->cost, cur->date);
            found = 1;
        }
        cur = cur->next;
    }
    if (!found) printf("未找到检查记录。\n");
    return found;
}

/**
 * 打印全部检查记录。
 * @return 打印的记录条数
 */
int examination_print_all(Examination *head) {
    int count = 0;
    printf("检查记录列表：\n");
    for (Examination *cur = head; cur; cur = cur->next) {
        printf("ID: %d, 患者: %s, 项目: %s, 费用: %.2f, 日期: %s\n",
               cur->id, cur->patient_name, cur->item, cur->cost, cur->date);
        count++;
    }
    return count;
}

/**
 * 释放检查记录链表全部节点。
 * @return 释放的节点数量
 */
int examination_free_all(Examination **head) {
    if (!head) return 0;
    Examination *cur = *head;
    int count = 0;
    while (cur) {
        Examination *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}