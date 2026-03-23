#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Inpatient *g_inpatient_head = NULL;

Inpatient* inpatient_create(int id, const char *patient, int ward_id, int bed_num,
                            float deposit, const char *admit_date, const char *discharge_date) {
    Inpatient *ip = (Inpatient*)malloc(sizeof(Inpatient));
    if (!ip) return NULL;
    ip->id = id;
    strcpy(ip->patient_name, patient);
    ip->ward_id = ward_id;
    ip->bed_num = bed_num;
    ip->deposit = deposit;
    strcpy(ip->admit_date, admit_date);
    if (discharge_date && strlen(discharge_date) > 0)
        strcpy(ip->discharge_date, discharge_date);
    else
        ip->discharge_date[0] = '\0';   // 空串表示未出院
    ip->prev = ip->next = NULL;
    return ip;
}

int inpatient_insert(Inpatient **head, Inpatient *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

int inpatient_delete(Inpatient **head, int id) {
    if (!head || !*head) return 0;
    Inpatient *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;
    if (cur->prev) cur->prev->next = cur->next;
    else *head = cur->next;
    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

Inpatient* inpatient_find_by_id(Inpatient *head, int id) {
    Inpatient *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

Inpatient* inpatient_find_by_patient(Inpatient *head, const char *patient) {
    Inpatient *cur = head;
    while (cur) {
        if (strcmp(cur->patient_name, patient) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

int inpatient_update_discharge(Inpatient *ip, const char *discharge_date) {
    if (!ip || !discharge_date) return 0;
    strcpy(ip->discharge_date, discharge_date);
    return 1;
}

int inpatient_print_all(Inpatient *head) {
    Inpatient *cur = head;
    int count = 0;
    printf("住院记录列表：\n");
    while (cur) {
        printf("ID: %d, 患者: %s, 病房ID: %d, 床位: %d, 押金: %.2f, 入院: %s, 出院: %s\n",
               cur->id, cur->patient_name, cur->ward_id, cur->bed_num, cur->deposit,
               cur->admit_date, cur->discharge_date[0] ? cur->discharge_date : "未出院");
        cur = cur->next;
        count++;
    }
    return count;
}

int inpatient_free_all(Inpatient **head) {
    if (!head) return 0;
    Inpatient *cur = *head;
    int count = 0;
    while (cur) {
        Inpatient *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}