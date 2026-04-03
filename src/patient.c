#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 模块：患者信息（Patient）
 * 数据结构：双向链表，表头为 g_patient_head
 * 关键约束：患者ID唯一；姓名可能重名，建议优先按ID查询
 * 持久化：由 file.c 负责 load_patients/save_patients
 */

// 全局表头指针（链表存储的起点）
Patient *g_patient_head = NULL;

/**
 * 创建患者节点。
 * @param id     患者ID（唯一）
 * @param name   姓名
 * @param age    年龄
 * @param gender 性别（0女/1男）
 * @return 成功返回新节点指针；失败返回 NULL
 */
Patient* patient_create(int id, const char *name, int age, bool gender) {
    Patient *p = (Patient*)malloc(sizeof(Patient));
    if (!p) return NULL;
    p->id = id;
    strcpy(p->name, name);
    p->age = age;
    p->gender = gender;
    p->prev = p->next = NULL;
    return p;
}

/**
 * 插入患者节点到链表头部。
 * @param head 链表头指针地址
 * @param node 待插入节点
 * @return 成功返回 1；失败返回 0
 */
int patient_insert(Patient **head, Patient *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

/**
 * 按患者ID删除节点。
 * @param head 链表头指针地址
 * @param id   患者ID
 * @return 成功返回 1；未找到/参数无效返回 0
 */
int patient_delete(Patient **head, int id) {
    if (!head || !*head) return 0;
    Patient *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;

    if (cur->prev) cur->prev->next = cur->next;// 删除中间节点
    else *head = cur->next;// 删除头节点

    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

/**
 * 按患者ID查找。
 * @param head 链表头
 * @param id   患者ID
 * @return 找到返回指针；否则返回 NULL
 */
Patient* patient_find_by_id(Patient *head, int id) {
    Patient *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * 按姓名查找（要求唯一）。
 * @note 若存在重名，会提示并返回 NULL，避免歧义。
 * @param head 链表头
 * @param name 姓名
 * @return 找到且唯一返回指针；否则返回 NULL
 */
Patient* patient_find_by_name(Patient *head, const char *name) {
    Patient *cur = head;
    Patient *result = NULL;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            if (!result) {
                result = cur;// 找到匹配的患者
            } else {
                printf("警告：找到多个同名患者（ID: %d, ID: %d），请使用ID进行唯一查找。\n", result->id, cur->id);
                return NULL;// 返回NULL表示有多个同名患者，无法确定唯一结果
            }
        }
        cur = cur->next;
    }
    return result;// 返回找到的患者或NULL
}

/**
 * 修改患者基础字段。
 * @param p      患者节点
 * @param name   新姓名
 * @param age    新年龄
 * @param gender 新性别
 * @return 成功返回 1；失败返回 0
 */
int patient_update(Patient *p, const char *name, int age, bool gender) {
    if (!p || !name) return 0;
    strcpy(p->name, name);
    p->age = age;
    p->gender = gender;
    return 1;
}

/**
 * 打印患者列表。
 * @param head 链表头
 * @return 打印的记录条数
 */
int patient_print_all(Patient *head) {
    Patient *cur = head;
    int count = 0;
    printf("患者列表：\n");
    while (cur) {
        printf("ID: %d, 姓名: %s, 年龄: %d, 性别: %s\n",
               cur->id, cur->name, cur->age, cur->gender ? "男" : "女");
        cur = cur->next;
        count++;
    }
    return count;
}

/**
 * 释放患者链表全部节点。
 * @param head 链表头指针地址
 * @return 释放的节点数量
 */
int patient_free_all(Patient **head) {
    if (!head) return 0;
    Patient *cur = *head;
    int count = 0;
    while (cur) {
        Patient *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}