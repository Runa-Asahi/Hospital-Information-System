#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Doctor *g_doctor_head = NULL;

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

int doctor_insert(Doctor **head, Doctor *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

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

Doctor* doctor_find_by_id(Doctor *head, int id) {
    Doctor *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

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