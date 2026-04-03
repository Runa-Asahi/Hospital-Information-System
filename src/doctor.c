#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 模块：医生信息（Doctor）
 * 数据结构：双向链表，表头为 g_doctor_head
 * 关键约束：工号ID唯一；姓名可能重名，建议按工号查询
 * 持久化：由 file.c 负责 load_doctors/save_doctors
 */

Doctor *g_doctor_head = NULL;

/**
 * 创建医生节点。
 * @param id         工号ID（唯一）
 * @param name       姓名
 * @param age        年龄
 * @param gender     性别（0女/1男）
 * @param department 所属科室（名称字符串）
 * @param title      职称
 * @param schedule   出诊时间描述
 * @return 成功返回新节点指针；失败返回 NULL
 */
Doctor* doctor_create(int id, const char *name, int age, bool gender,
                      const char *department, const char *title,
                      const char *schedule) {
    Doctor *d = (Doctor*)malloc(sizeof(Doctor));
    if (!d) return NULL;
    d->id = id;
    strcpy(d->name, name);
    d->age = age;
    d->gender = gender;
    strcpy(d->department, department);
    strcpy(d->title, title);
    strcpy(d->schedule, schedule);
    d->prev = d->next = NULL;
    return d;
}

/**
 * 插入医生节点到链表头部。
 * @param head 链表头指针地址
 * @param node 待插入节点
 * @return 成功返回 1；失败返回 0
 */
int doctor_insert(Doctor **head, Doctor *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

/**
 * 按工号ID删除医生节点。
 * @param head 链表头指针地址
 * @param id   工号ID
 * @return 成功返回 1；未找到/参数无效返回 0
 */
int doctor_delete(Doctor **head, int id) {
    if (!head || !*head) return 0;
    Doctor *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;

    if (cur->prev) cur->prev->next = cur->next;
    else *head = cur->next;

    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

/**
 * 按工号ID查找医生。
 * @param head 链表头
 * @param id   工号ID
 * @return 找到返回指针；否则返回 NULL
 */
Doctor* doctor_find_by_id(Doctor *head, int id) {
    Doctor *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * 按姓名查找医生（要求唯一）。
 * @note 若存在重名，会提示并返回 NULL，避免歧义。
 * @param head 链表头
 * @param name 医生姓名
 * @return 找到且唯一返回指针；否则返回 NULL
 */
Doctor* doctor_find_by_name(Doctor *head, const char *name) {
    Doctor *cur = head;
    Doctor *result = NULL;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            if (!result) {
                result = cur;
            } else {
                printf("警告：找到多个同名医生（ID: %d, ID: %d），请使用工号ID进行唯一查找。\n", result->id, cur->id);
                return NULL;
            }
        }
        cur = cur->next;
    }
    return result;
}

/**
 * 修改医生基础字段。
 * @return 成功返回 1；失败返回 0
 */
int doctor_update(Doctor *d, const char *name, int age, bool gender,
                   const char *department, const char *title,
                   const char *schedule) {
    if (!d || !name || !department || !title || !schedule) return 0;
    strcpy(d->name, name);
    d->age = age;
    d->gender = gender;
    strcpy(d->department, department);
    strcpy(d->title, title);
    strcpy(d->schedule, schedule);
    return 1;
}

/**
 * 打印医生列表。
 * @param head 链表头
 * @return 打印的记录条数
 */
int doctor_print_all(Doctor *head) {
    Doctor *cur = head;
    int count = 0;
    printf("医生列表：\n");
    while (cur) {
        printf("ID: %d, 姓名: %s, 年龄: %d, 性别: %s, 科室: %s, 职称: %s, 出诊: %s\n",
               cur->id, cur->name, cur->age, cur->gender ? "男" : "女",
               cur->department, cur->title, cur->schedule);
        cur = cur->next;
        count++;
    }
    return count;
}

/**
 * 释放医生链表全部节点。
 * @param head 链表头指针地址
 * @return 释放的节点数量
 */
int doctor_free_all(Doctor **head) {
    if (!head) return 0;
    Doctor *cur = *head;
    int count = 0;
    while (cur) {
        Doctor *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}