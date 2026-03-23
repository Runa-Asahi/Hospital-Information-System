#include "his.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Drug *g_drug_head = NULL;

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

int drug_insert(Drug **head, Drug *node) {
    if (!head || !node) return 0;
    node->next = *head;
    if (*head) (*head)->prev = node;
    *head = node;
    return 1;
}

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

Drug* drug_find_by_id(Drug *head, int id) {
    Drug *cur = head;
    while (cur) {
        if (cur->id == id) return cur;
        cur = cur->next;
    }
    return NULL;
}

Drug* drug_find_by_name(Drug *head, const char *name) {
    Drug *cur = head;
    while (cur) {
        if (strcmp(cur->name, name) == 0) return cur;
        cur = cur->next;
    }
    return NULL;
}

int drug_update_stock(Drug *d, int delta) {// delta可正可负
    if (!d) return 0;
    int new_stock = d->stock + delta;
    if (new_stock < 0) new_stock = 0;   // 防止负数
    d->stock = new_stock;
    return 1;
}

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