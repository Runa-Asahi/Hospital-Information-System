#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 模块：病房信息（Ward）
 * 数据结构：双向链表，表头为 g_ward_head
 * 关键字段：type（Single/Double/Triple/ICU 等）、床位占用 occupied_beds
 * @note occupied_beds 会在住院模块 inpatient.c 中随入院/出院自动同步更新。
 * 持久化：由 file.c 负责 load_wards/save_wards
 */

Ward *g_ward_head = NULL;

/**
 * 创建病房节点。
 * @param id         病房ID（唯一）
 * @param type       病房类型（Single/Double/Triple/ICU 等）
 * @param total_beds 总床位数
 * @param department 关联科室
 * @param can_convert 是否可转换（0/1）
 * @return 成功返回新节点指针；失败返回 NULL
 */
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

/**
 * 插入病房节点到链表头部。
 * @return 成功返回 1；失败返回 0
 */
int ward_insert(Ward **head, Ward *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

/**
 * 按病房ID删除节点。
 * @return 成功返回 1；未找到/参数无效返回 0
 */
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

/**
 * 按病房ID查找。
 * @return 找到返回指针；否则返回 NULL
 */
Ward* ward_find_by_id(Ward *head, int id) {
    Ward *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * 计算空床数量。
 * @return 空床数；若 w 无效/数据异常则返回 0
 */
int ward_free_beds(const Ward *w) {
    if (!w) return 0;
    if (w->total_beds <= 0) return 0;
    if (w->occupied_beds < 0) return w->total_beds;
    if (w->occupied_beds >= w->total_beds) return 0;
    return w->total_beds - w->occupied_beds;
}

/**
 * 在同类型病房中择优选择空床最多的病房。
 * @param required_free_beds 至少需要的空床数（<=0 视为 1）
 * @return 找到返回病房指针；否则返回 NULL
 */
Ward* ward_find_available_by_type(Ward *head, const char *type, int required_free_beds) {
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

/**
 * 是否至少有 1 张空床。
 */
int ward_has_free_bed(Ward *w) {
    return ward_free_beds(w) > 0;
}

/**
 * 占用 1 张床。
 * @return 成功返回 1；无空床/参数无效返回 0
 */
int ward_occupy_bed(Ward *w) {
    if (!w) return 0;
    if (!ward_has_free_bed(w)) return 0;
    w->occupied_beds++;
    return 1;
}

/**
 * 释放 1 张床。
 * @return 成功返回 1；occupied_beds 已为 0 或参数无效返回 0
 */
int ward_release_bed(Ward *w) {
    if (!w) return 0;
    if (w->occupied_beds <= 0) return 0;
    w->occupied_beds--;
    return 1;
}

/**
 * 打印病房列表。
 * @return 打印的记录条数
 */
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

/**
 * 释放病房链表全部节点。
 * @return 释放的节点数量
 */
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