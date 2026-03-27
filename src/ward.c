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
    w->total_beds = total_beds; // 总床位数
    w->occupied_beds = 0;       // 初始均为空闲
    strcpy(w->department, department);// 关联科室
    w->can_convert = can_convert;// 是否可转换
    w->prev = w->next = NULL;
    return w;
}

int ward_insert(Ward **head, Ward *node) {// 插入病房节点到链表头部
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

int ward_delete(Ward **head, int id) {// 根据ID删除病房
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

Ward* ward_find_by_id(Ward *head, int id) {// 根据ID查找病房
    Ward *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

int ward_free_beds(const Ward *w) {// 返回空床数量；如果病房不存在或数据异常则返回0。
    if (!w) return 0;
    if (w->total_beds <= 0) return 0;
    if (w->occupied_beds < 0) return w->total_beds;
    if (w->occupied_beds >= w->total_beds) return 0;
    return w->total_beds - w->occupied_beds;
}

Ward* ward_find_available_by_type(Ward *head, const char *type, int required_free_beds) {// 在同类型病房中优先选择空闲床位最多的；如果没有满足条件的病房则返回NULL。
    if (!head || !type) return NULL;
    if (required_free_beds <= 0) required_free_beds = 1;

    Ward *best = NULL;
    int best_free = -1;

    for (Ward *cur = head; cur; cur = cur->next) {
        if (strcmp(cur->type, type) != 0) continue;
        int free_beds = ward_free_beds(cur);
        if (free_beds < required_free_beds) continue;

        if (!best || free_beds > best_free || (free_beds == best_free && cur->id < best->id)) {
            best = cur;
            best_free = free_beds;
        }
    }

    return best;
}

Ward* ward_find_by_type(Ward *head, const char *type) {
    // 兼容旧接口：同类型中优先选择“空闲床位最多”的病房；没有空床则返回NULL。
    return ward_find_available_by_type(head, type, 1);
}

int ward_has_free_bed(Ward *w) {// 是否至少有1张空床
    return ward_free_beds(w) > 0;
}

int ward_occupy_bed(Ward *w) {// 占用1张床
    if (!w) return 0;
    if (!ward_has_free_bed(w)) return 0;
    w->occupied_beds++;
    return 1;
}

int ward_release_bed(Ward *w) {// 释放1张床
    if (!w) return 0;
    if (w->occupied_beds <= 0) return 0;
    w->occupied_beds--;
    return 1;
}

int ward_print_all(Ward *head) {// 打印病房列表，返回病房总数
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

int ward_free_all(Ward **head) {// 释放病房链表所有节点，返回释放的节点数量
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