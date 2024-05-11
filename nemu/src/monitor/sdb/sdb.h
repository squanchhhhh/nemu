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

#ifndef __SDB_H__
#define __SDB_H__
#define MAX_EXPR_LEN 100  // 设置最长的表达式为100个字符
#include <common.h>
// 监视点结构体
typedef struct watchpoint {
    int NO;                   // 监视点编号
    struct watchpoint *next;  // 指向下一个监视点的指针
    char expr_[MAX_EXPR_LEN];  // 存储监视的表达式
    uint32_t old_value;       // 存储上一次表达式的值
    uint32_t new_value;       // 存储当前表达式的值
} WP;
word_t expr(char *e, bool *success);
void step_watchpoint();
WP* new_wp(char * watch_expr);
#endif
