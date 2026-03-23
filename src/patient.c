#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 定义全局头指针（分配内存）
Patient *g_patient_head = NULL;

Patient* patient_create(int id, const char *name, int age, bool gender) {
    Patient *p = (Patient*)malloc(sizeof(Patient));
    if (!p) return NULL;// 内存分配失败
    p->id = id;// 唯一ID
    strcpy(p->name, name);// 姓名
    p->age = age;// 年龄
    p->gender = gender;// 性别
    p->prev = p->next = NULL;
    return p;
}

int patient_insert(Patient **head, Patient *node) {
    if (!head || !node) return 0;// 参数检查
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

int patient_delete(Patient **head, int id) {
    if (!head || !*head) return 0;
    Patient *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;// 未找到

    if (cur->prev) cur->prev->next = cur->next;// 删除中间节点
    else *head = cur->next;// 删除头节点

    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

Patient* patient_find_by_id(Patient *head, int id) {
    Patient *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;// 未找到
}

Patient* patient_find_by_name(Patient *head, const char *name) {
    Patient *cur = head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) return cur;
        cur = cur->next;
    }
    return NULL;// 未找到
}

int patient_update(Patient *p, const char *name, int age, bool gender) {
    if (!p || !name) return 0;// 参数检查
    strcpy(p->name, name);
    p->age = age;
    p->gender = gender;
    return 1;
}

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