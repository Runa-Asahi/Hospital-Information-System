#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Department *g_department_head = NULL;

Department* department_create(int id, const char *name) {
    Department *d = (Department*)malloc(sizeof(Department));
    if (!d) return NULL;
    d->id = id;
    strcpy(d->name, name);
    d->prev = d->next = NULL;
    return d;
}

int department_insert(Department **head, Department *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

int department_delete(Department **head, int id) {
    if (!head || !*head) return 0;
    Department *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;
    if (cur->prev) cur->prev->next = cur->next;
    else *head = cur->next;
    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

Department* department_find_by_id(Department *head, int id) {
    Department *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

Department* department_find_by_name(Department *head, const char *name) {
    Department *cur = head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

int department_print_all(Department *head) {
    Department *cur = head;
    int count = 0;
    printf("科室列表：\n");
    while (cur) {
        printf("ID: %d, 名称: %s\n", cur->id, cur->name);
        cur = cur->next;
        count++;
    }
    return count;
}

int department_free_all(Department **head) {
    if (!head) return 0;
    Department *cur = *head;
    int count = 0;
    while (cur) {
        Department *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}