#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Ward *g_ward_head = NULL;

Ward* ward_create(int id, const char *type, int total_beds, const char *department, int can_convert) {
    Ward *w = (Ward*)malloc(sizeof(Ward));
    if (!w) return NULL;
    w->id = id;
    strcpy(w->type, type);
    w->total_beds = total_beds;
    w->occupied_beds = 0;       // 初始均为空闲
    strcpy(w->department, department);
    w->can_convert = can_convert;
    w->prev = w->next = NULL;
    return w;
}

int ward_insert(Ward **head, Ward *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

int ward_delete(Ward **head, int id) {
    if (!head || !*head) return 0;
    Ward *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;
    if (cur->prev) cur->prev->next = cur->next;
    else *head = cur->next;
    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

Ward* ward_find_by_id(Ward *head, int id) {
    Ward *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

Ward* ward_find_by_type(Ward *head, const char *type) {
    Ward *cur = head;
    while (cur) {
        if (strcmp(cur->type, type) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

int ward_has_free_bed(Ward *w) {
    if (!w) return 0;
    return w->occupied_beds < w->total_beds;
}

int ward_occupy_bed(Ward *w) {
    if (!w) return 0;
    if (!ward_has_free_bed(w)) return 0;
    w->occupied_beds++;
    return 1;
}

int ward_release_bed(Ward *w) {
    if (!w) return 0;
    if (w->occupied_beds <= 0) return 0;
    w->occupied_beds--;
    return 1;
}

int ward_print_all(Ward *head) {
    Ward *cur = head;
    int count = 0;
    printf("病房列表：\n");
    while (cur) {
        printf("ID: %d, 类型: %s, 总床位: %d, 已占: %d, 科室: %s, 可转换: %s\n",
               cur->id, cur->type, cur->total_beds, cur->occupied_beds,
               cur->department, cur->can_convert ? "是" : "否");
        cur = cur->next;
        count++;
    }
    return count;
}

int ward_free_all(Ward **head) {
    if (!head) return 0;
    Ward *cur = *head;
    int count = 0;
    while (cur) {
        Ward *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}