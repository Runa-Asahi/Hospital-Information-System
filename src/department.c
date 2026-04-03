#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 模块：科室信息（Department）
 * 数据结构：双向链表，表头为 g_department_head
 * 持久化：由 file.c 负责 load_departments/save_departments
 */

Department *g_department_head = NULL;

/**
 * 创建科室节点。
 * @param id   科室ID（唯一）
 * @param name 科室名称
 * @return 成功返回新节点指针；失败返回 NULL（内存不足）
 */
Department* department_create(int id, const char *name) {
    Department *d = (Department*)malloc(sizeof(Department));
    if (!d) return NULL;
    d->id = id;
    // 注意：这里不检查 name 长度，调用者需保证不超过结构体字段容量。
    strcpy(d->name, name);
    d->prev = d->next = NULL;
    return d;
}

/**
 * 插入科室节点到链表头部。
 * @param head 链表头指针地址
 * @param node 待插入节点
 * @return 成功返回 1；失败返回 0
 */
int department_insert(Department **head, Department *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

/**
 * 按科室ID删除节点。
 * @param head 链表头指针地址
 * @param id   科室ID
 * @return 成功返回 1；未找到/参数无效返回 0
 */
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

/**
 * 按科室ID查找。
 * @param head 链表头
 * @param id   科室ID
 * @return 找到返回指针；否则返回 NULL
 */
Department* department_find_by_id(Department *head, int id) {
    Department *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * 按科室名称查找（要求唯一）。
 * @param head 链表头
 * @param name 科室名称
 * @return 找到且唯一返回指针；未找到返回 NULL；重名返回 NULL 并给出提示
 */
Department* department_find_by_name(Department *head, const char *name) {
    Department *cur = head;
    Department *result = NULL;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            if (!result) {
                result = cur;
            } else {
                printf("警告：找到多个同名科室（ID: %d, ID: %d），请使用科室ID进行唯一查找。\n", result->id, cur->id);
                return NULL;
            }
        }
        cur = cur->next;
    }
    return result;
}

/**
 * 打印科室列表。
 * @param head 链表头
 * @return 打印的记录条数
 */
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

/**
 * 释放科室链表全部节点。
 * @param head 链表头指针地址
 * @return 释放的节点数量
 */
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