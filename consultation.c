#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Consultation *g_consultation_head = NULL;

Consultation* consultation_create(int id, const char *patient, const char *doctor,
                                  const char *date, const char *diagnosis, const char *prescription) {
    Consultation *c = (Consultation*)malloc(sizeof(Consultation));
    if (!c) return NULL;
    c->id = id;
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