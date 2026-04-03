#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 模块：挂号记录（Register）
 * 数据结构：双向链表，表头为 g_register_head
 * 关键用途：患者业务档案、医生维度统计（某日挂号量）
 * 持久化：由 file.c 负责 load_registers/save_registers
 */

Register *g_register_head = NULL;

/**
 * 创建挂号记录节点。
 * @param id      挂号ID（唯一）
 * @param patient 患者姓名
 * @param doctor  医生姓名
 * @param date    挂号日期（YYYY-MM-DD）
 * @return 成功返回新节点指针；失败返回 NULL
 */
Register* register_create(int id, const char *patient,
    const char *doctor, const char *date) {
    Register *r = (Register*)malloc(sizeof(Register));
    if (!r) return NULL;
    r->id = id;
    strcpy(r->patient_name, patient);
    strcpy(r->doctor_name, doctor);
    strcpy(r->date, date);
    r->prev = r->next = NULL;
    return r;
}

/**
 * 插入挂号记录节点到链表头部。
 * @return 成功返回 1；失败返回 0
 */
int register_insert(Register **head, Register *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

/**
 * 按挂号ID删除记录。
 * @return 成功返回 1；未找到/参数无效返回 0
 */
int register_delete(Register **head, int id) {
    if (!head || !*head) return 0;
    Register *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;
    if (cur->prev) cur->prev->next = cur->next;
    else *head = cur->next;
    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

/**
 * 按挂号ID查找。
 * @return 找到返回指针；否则返回 NULL
 */
Register* register_find_by_id(Register *head, int id) {
    Register *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * 统计某医生在某日期的挂号数量。
 * @param doctor 医生姓名
 * @param date   日期（YYYY-MM-DD）
 * @return 挂号数量
 */
int register_count_by_doctor_date(Register *head, const char *doctor, const char *date) {
    int count = 0;
    Register *cur = head;
    while (cur) {
        if (strcmp(cur->doctor_name, doctor) == 0 && strcmp(cur->date, date) == 0) {
            count++;
        }
        cur = cur->next;
    }
    return count;
}

/**
 * 打印某患者的挂号记录。
 * @return 若找到至少一条返回 1；否则返回 0
 */
int register_print_by_patient(Register *head, const char *patient) {
    if (!patient) return 0;
    Register *cur = head;
    int found = 0;
    printf("患者 %s 的挂号记录：\n", patient);
    while (cur) {
        if (strcmp(cur->patient_name, patient) == 0) {
            printf("ID: %d, 医生: %s, 日期: %s\n", cur->id, cur->doctor_name, cur->date);
            found = 1;
        }
        cur = cur->next;
    }
    if (!found) printf("未找到挂号记录。\n");
    return found;
}

/**
 * 打印全部挂号记录。
 * @return 打印的记录条数
 */
int register_print_all(Register *head) {
    Register *cur = head;
    int count = 0;
    printf("挂号记录列表：\n");
    while (cur) {
        printf("ID: %d, 患者: %s, 医生: %s, 日期: %s\n",
               cur->id, cur->patient_name, cur->doctor_name, cur->date);
        cur = cur->next;
        count++;
    }
    return count;
}

/**
 * 释放挂号记录链表全部节点。
 * @return 释放的节点数量
 */
int register_free_all(Register **head) {
    if (!head) return 0;
    Register *cur = *head;
    int count = 0;
    while (cur) {
        Register *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}