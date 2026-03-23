#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Register *g_register_head = NULL;

Register* register_create(int id, const char *patient, const char *doctor, const char *date) {
    Register *r = (Register*)malloc(sizeof(Register));
    if (!r) return NULL;
    r->id = id;
    strcpy(r->patient_name, patient);
    strcpy(r->doctor_name, doctor);
    strcpy(r->date, date);
    r->prev = r->next = NULL;
    return r;
}

int register_insert(Register **head, Register *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

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

Register* register_find_by_id(Register *head, int id) {
    Register *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

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