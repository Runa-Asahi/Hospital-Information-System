#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Register *g_register_head = NULL;// 全局头指针（分配内存）

Register* register_create(int id, const char *patient, 
    const char *doctor, const char *date) {// 创建挂号记录节点
    Register *r = (Register*)malloc(sizeof(Register));// 内存分配失败
    if (!r) return NULL;
    r->id = id;
    strcpy(r->patient_name, patient);
    strcpy(r->doctor_name, doctor);
    strcpy(r->date, date);
    r->prev = r->next = NULL;
    return r;
}

int register_insert(Register **head, Register *node) {// 插入挂号记录节点到链表头部
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

int register_delete(Register **head, int id) {// 根据ID删除挂号记录
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

Register* register_find_by_id(Register *head, int id) {// 根据ID查找挂号记录
    Register *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

int register_count_by_doctor_date(Register *head, const char *doctor, const char *date) {// 统计指定医生在特定日期的挂号数量
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

int register_print_by_patient(Register *head, const char *patient) {// 打印指定患者的挂号记录
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

int register_print_all(Register *head) {// 打印所有挂号记录，返回记录总数
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

int register_free_all(Register **head) {// 释放挂号记录链表所有节点，返回释放的节点数量
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