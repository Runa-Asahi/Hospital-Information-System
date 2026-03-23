#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Examination *g_examination_head = NULL;

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

int examination_insert(Examination **head, Examination *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

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

Examination* examination_find_by_id(Examination *head, int id) {
    Examination *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

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