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

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
int check_parentheses(char *p, char *q);
char *get_main_op_site(char *p, char *q);
int eval(char *p,char*q);
enum {
  TK_NOTYPE = 256, 
  TK_PLUS,
  TK_MINUS,
  TK_DIV,
  TK_EQ,
  TK_NUM,
  TK_LEFTP,
  TK_RIGHTP,
  TK_MULT
  /* TODO: Add more token types */
};

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},    // spaces
  {"\\+", TK_PLUS},     // plus
  {"==", TK_EQ},        // equal
  {"[0-9]+", TK_NUM},     // digits
  {"-", TK_MINUS},      // minus
  {"/", TK_DIV},        // division
  {"\\*",TK_MULT}
};


#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        tokens[nr_token].type = rules[i].token_type;
        strncpy(tokens[nr_token].str,substr_start,substr_len);
        tokens[nr_token].str[substr_len] = '\0';
        nr_token++;
        /*       
        switch (rules[i].token_type) {
          default: TODO();
        }
        */

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // 分配足够的内存来存储连接后的字符串
  char *final_expr = (char *)malloc(nr_token * 5 * sizeof(char));
  final_expr[0] = '\0';  // 将字符串初始化为空字符串

  // 使用 strcat 函数连接 token 字符串
  for (int i = 0; i < nr_token; i++) {
    printf("%d-%s\n", tokens[i].type, tokens[i].str);
    strcat(final_expr, tokens[i].str);
  }

  /* TODO: Insert codes to evaluate the expression. */
  word_t result = eval(final_expr, final_expr+ sizeof(final_expr) - 2);

  free(final_expr);  // 释放动态分配的内存
  return result;
}

int eval(char *p, char *q) {
    if (p > q) {
        printf("illegal\n");
        assert(0);
    } else if (p == q) {
        if (*p >= '0' && *p <= '9') {
            return *p - '0';
        } else {
            printf("illegal\n");
            assert(0);
        }
    } else if (check_parentheses(p, q) == 1) {
        return eval(p + 1, q - 1);
    } else {
        char *op = get_main_op_site(p, q);
        if (op == NULL) {
            printf("No valid operator found.\n");
            assert(0);
        }
        int val1 = eval(p, op - 1);
        int val2 = eval(op + 1, q);
        switch (*op) {
            case '+': return val1 + val2;
            case '-': return val1 - val2;
            case '*': return val1 * val2;
            case '/':
                if (val2 == 0) {
                    printf("Division by zero.\n");
                    assert(0);
                }
                return val1 / val2;
            default:
                printf("Unsupported operator '%c'.\n", *op);
                assert(0);
        }
    }
    // Should never reach here
    return 0;
}

int check_parentheses(char *p, char *q) {
    if (*p != '(') return 0;
    p++;
    q--;
    int position = 0;
    while (p <= q) {
        if (*p == '(') {
            position++;
        }
        else if (*p == ')') {
            if (position == 0) return 0;  // 没有与之匹配的左括号
            position--;  // 先减少位置
        }
        p++;
    }
    return position == 0;  // 如果栈空，返回1，表示匹配
}
char *get_main_op_site(char *p, char *q){
    char stack[32];
    char* op_position[32];
    int position = 0;
    while(p<=q){
        if (*p == '(' || *p =='+' || *p == '*'||*p == '/'|| *p == '-'){
            op_position[position] = p;
            stack[position] = *p;
            position ++;
        }
        else if (*p == ')'){
            while (stack[--position]!='('){
            }
        }
        p++;
    }
    if (position > 0) {
        return op_position[position - 1];  // 返回最后一个运算符的位置
    }
    return NULL;
}