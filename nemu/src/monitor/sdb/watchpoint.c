/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include "sdb.h"


#define NR_WP 32          // 设置监视点的最大数量


void free_wp(WP *wp);
static WP wp_pool[NR_WP] = {};  // 监视点结构体的池,用于存储所有监视点
static WP *head = NULL, *free_ = NULL;  // head指向使用中的监视点链表头,free_指向空闲的监视点链表头

// 初始化监视点池
void init_wp_pool() {
    int i;
    for (i = 0; i < NR_WP; i++) {
        wp_pool[i].NO = i;  // 为每个监视点结构体分配编号
        wp_pool[i].next = (i == NR_WP - 1 ? NULL : &wp_pool[i + 1]);  // 将所有监视点结构体链接成一个链表
    }

    head = NULL;  // 初始时,使用中的监视点链表为空
    free_ = wp_pool;  // 初始时,所有监视点结构体都在空闲链表中
}
//从free节点中取出一个watchpoint
WP* new_wp(char * watch_expr){
    if(free_ == NULL){
        printf("No free watchpoint!\n");
        return NULL;
    }
    WP *wp = free_;
    free_ = free_->next;
    wp->next = head;
    strncpy(wp->expr_, watch_expr, MAX_EXPR_LEN);
    bool success = 0;
    wp->old_value = expr(wp->expr_,&success);
    if (!success){
        printf("Invalid expression!\n");
        free_wp(wp);
        return NULL;
    }
    head = wp;
    return wp;
}
//将一个watchpoint放回free节点中
void free_wp(WP *wp){
    wp->next = free_;
    free_ = wp;
}
void step_watchpoint(){
    struct watchpoint* temp=head;
    while (temp->next!=NULL){
        bool s = 1;
        temp->new_value = expr(temp->expr_,&s);
        if(!s){
            printf("Invalid expression!\n");
            return;
        }
        printf("watchpoint %d: %s\n",temp->NO,temp->expr_);
        printf("old value: %d\n",temp->old_value);
        printf("new value: %d\n",temp->new_value);
}}

