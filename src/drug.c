#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * 模块：药品信息（Drug）
 * 数据结构：双向链表，表头为 g_drug_head
 * 关键字段：库存 stock、单价 price、关联科室 department
 * 持久化：由 file.c 负责 load_drugs/save_drugs
 */

Drug *g_drug_head = NULL;

/**
 * 创建药品节点。
 * @param id         药品ID（唯一）
 * @param name       通用名
 * @param alias      别名
 * @param stock      库存
 * @param price      单价
 * @param department 关联科室
 * @return 成功返回新节点指针；失败返回 NULL
 */
Drug* drug_create(int id, const char *name, const char *alias, int stock, float price, const char *department) {
    Drug *d = (Drug*)malloc(sizeof(Drug));
    if (!d) return NULL;
    d->id = id;
    strcpy(d->name, name);
    strcpy(d->alias, alias);
    d->stock = stock;
    d->price = price;
    strcpy(d->department, department);
    d->prev = d->next = NULL;
    return d;
}

/**
 * 插入药品节点到链表头部。
 * @return 成功返回 1；失败返回 0
 */
int drug_insert(Drug **head, Drug *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

/**
 * 按药品ID删除节点。
 * @return 成功返回 1；未找到/参数无效返回 0
 */
int drug_delete(Drug **head, int id) {
    if (!head || !*head) return 0;
    Drug *cur = *head;
    while (cur && cur->id != id) cur = cur->next;
    if (!cur) return 0;

    if (cur->prev) cur->prev->next = cur->next;
    else *head = cur->next;

    if (cur->next) cur->next->prev = cur->prev;
    free(cur);
    return 1;
}

/**
 * 按药品ID查找。
 * @return 找到返回指针；否则返回 NULL
 */
Drug* drug_find_by_id(Drug *head, int id) {
    Drug *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

/**
 * 按通用名查找（要求唯一）。
 * @note 若存在重名，会提示并返回 NULL。
 */
Drug* drug_find_by_name(Drug *head, const char *name) {
    Drug *cur = head;
    Drug *result = NULL;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            if (!result) {
                result = cur;
            } else {
                printf("警告：找到多个同名药品（ID: %d, ID: %d），请使用药品ID进行唯一查找。\n", result->id, cur->id);
                return NULL;
            }
        }
        cur = cur->next;
    }
    return result;
}

/**
 * 更新库存。
 * @param d     药品节点
 * @param delta 库存变化量（可正可负）；下限截断为 0
 * @return 成功返回 1；失败返回 0
 */
int drug_update_stock(Drug *d, int delta) {
    if (!d) return 0;
    int new_stock = d->stock + delta;
    if (new_stock < 0) new_stock = 0;   // 防止负数
    d->stock = new_stock;
    return 1;
}

/**
 * 打印药品列表。
 * @return 打印的记录条数
 */
int drug_print_all(Drug *head) {
    Drug *cur = head;
    int count = 0;
    printf("药品列表：\n");
    while (cur) {
        printf("ID: %d, 名称: %s, 别名: %s, 库存: %d, 单价: %.2f, 科室: %s\n",
               cur->id, cur->name, cur->alias, cur->stock, cur->price, cur->department);
        cur = cur->next;
        count++;
    }
    return count;
}

/**
 * 释放药品链表全部节点。
 * @return 释放的节点数量
 */
int drug_free_all(Drug **head) {
    if (!head) return 0;
    Drug *cur = *head;
    int count = 0;
    while (cur) {
        Drug *next = cur->next;
        free(cur);
        cur = next;
        count++;
    }
    *head = NULL;
    return count;
}