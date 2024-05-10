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
#include <memory/paddr.h>

enum {
    TK_NOTYPE = 256, // " "
    TK_PLUS = 240,  // '+'
    TK_MINUS = 241, // "-"
    TK_NEGATIVE = 243, // "-"
    TK_DIV = 253, // "/"
    TK_EQ = 252, // "=="
    TK_NUM = 251, // "\d+"
    TK_LEFTP = 250, // '('
    TK_RIGHTP = 249, // ')'
    TK_MULT = 248, // '*'
    TK_NOTEQ = 254, // "!="
    TK_AND = 255, // "&&"
    TK_REG = 247,// "\$[rsgta][0-9]"
    TK_DEREF = 244, // "*\w+"
    TK_HEX_NUM = 246  // "0x\d+"
};

static struct rule {
    const char *regex;
    int token_type;
} rules[] = {
          {"0x[0-9A-Fa-f]+", TK_HEX_NUM},
        {" +",              TK_NOTYPE},    // spaces
        {"\\+",             TK_PLUS},     // plus
        {"==",              TK_EQ},        // equal
        {"[0-9]+",          TK_NUM},     // digits
        {"-",               TK_MINUS},      // minus
        {"/",               TK_DIV},        // division
        {"\\*",             TK_MULT},
        {"\\(",             TK_LEFTP},     // left parentheses
        {"\\)",             TK_RIGHTP},     // right parentheses
        {"!=",              TK_NOTEQ},
        {"&&",              TK_AND},
        {"\\$[rsgta][0-9]", TK_REG}

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
uint32_t eval(Token *p, Token *q);
bool parentheses(Token *p, Token *q);
Token *get_main_op_position(Token *p, Token *q);
uint32_t get_reg(char * reg_name){
  bool flag = 0;
  return isa_reg_str2val(reg_name,&flag);
}
uint32_t get_mem(uint32_t addr){
  uint32_t value = paddr_read(addr, 4);
  return value;
}


static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
    int position = 0;
    int i;
    regmatch_t pmatch;
    //token计数器
    nr_token = 0;
    while (e[position] != '\0') {
        for (i = 0; i < NR_REGEX; i++) {
            if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
                char *substr_start = e + position;
                int substr_len = pmatch.rm_eo;
                position += substr_len;
                tokens[nr_token].type = rules[i].token_type;
                //如果是空格直接跳过
                if (tokens[nr_token].type == TK_NOTYPE) {
                    break;
                }
                //如果token是*
                //如果是第一个token，或者前面的token是+—*/ == != && 就认定该token是解引用
                if (tokens[nr_token].type == TK_MULT && (nr_token == 0 ||
                                                         tokens[nr_token - 1].type == TK_PLUS ||
                                                         tokens[nr_token - 1].type == TK_MINUS ||
                                                         tokens[nr_token - 1].type == TK_MULT ||
                                                         tokens[nr_token - 1].type == TK_DIV ||
                                                         tokens[nr_token - 1].type == TK_EQ ||
                                                         tokens[nr_token - 1].type == TK_NOTEQ ||
                                                         tokens[nr_token - 1].type == TK_AND
                )) {
                    tokens[nr_token].type = TK_DEREF;
                }
                //如果token是-
                //如果是第一个token，或者前面的token是+—*/ == != && 就认定该token是负数
                if (tokens[nr_token].type == TK_MINUS && (nr_token == 0 ||
                                                          tokens[nr_token - 1].type == TK_PLUS ||
                                                          tokens[nr_token - 1].type == TK_MINUS ||
                                                          tokens[nr_token - 1].type == TK_MULT ||
                                                          tokens[nr_token - 1].type == TK_DIV ||
                                                          tokens[nr_token - 1].type == TK_EQ ||
                                                          tokens[nr_token - 1].type == TK_NOTEQ ||
                                                          tokens[nr_token - 1].type == TK_AND
                )) {
                    tokens[nr_token].type = TK_NEGATIVE;
                }
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                tokens[nr_token].str[substr_len] = '\0';
                //如果token是16进制数，则提前转为10进制
                if (tokens[nr_token].type == TK_HEX_NUM) {
                    uint32_t num;
                    // 将十六进制字符串转换为整数
                    sscanf(tokens[nr_token].str, "0x%x", &num);
                    // 将整数转换为十进制字符串
                    sprintf(tokens[nr_token].str, "%d", num);
                    tokens[nr_token].type = TK_NUM;
                }
                nr_token++;
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
    word_t result = eval(tokens, tokens + nr_token - 1);
    printf("%d", result);
    return result;
}

uint32_t eval(Token *p, Token *q) {
    if (p > q) {
        printf("illegal\n");
        assert(0);
    } else if (p == q) {
        if (p->type == TK_REG) {
            return get_reg(p->str);
        }
         else {
            uint32_t num = 0;
            sscanf(p->str, "%u", &num);
            return num;
        }
    } else if (parentheses(p, q) == 1) {
        return eval(p + 1, q - 1);
    } else if (p->type == TK_REG) {
        //取出寄存器的值
        uint32_t num = get_reg(p->str);
        sprintf(p->str, "%d", num);
        p->type = TK_NUM;
        return eval(p, q);
    } else {
        int neg_flag = 0;
        int deref_flag = 0;
        Token *op;
        if (p->type == TK_NEGATIVE) {
            op = get_main_op_position(p + 1, q);
            neg_flag = 1;
        } else if (p->type == TK_DEREF) {
            op = get_main_op_position(p + 1, q);
            deref_flag = 1;
        } else {
            op = get_main_op_position(p, q);
        }
        if (op == NULL) {
            printf("No valid operator found.\n");
            assert(0);
        }
        uint32_t val1;
        if (neg_flag) {
            val1 = -eval(p + 1, op - 1);
        } else if (deref_flag) {
            uint32_t *num = 0;
            sscanf((p + 1)->str, "%u", num);
            val1 = get_mem(*num);

        } else {
            val1 = eval(p, op - 1);
        }
        uint32_t val2 = eval(op + 1, q);
        switch (op->type) {
            case TK_PLUS:
                return val1 + val2;
            case TK_MINUS:
                return val1 - val2;
            case TK_MULT:
                return val1 * val2;
            case TK_DIV:
                if (val2 == 0) {
                    printf("Division by zero.\n");
                    assert(0);
                }
                return val1 / val2;
            default:
                printf("illegal\n");
                assert(0);
        }
    }

}

Token *get_main_op_position(Token *p, Token *q) {
    //TOKEN栈，假设操作符最多32个
    Token *stack[32];
    int count = 0;
    while (p <= q) {
        // + - * / ( 进栈
        if (p->type == TK_MINUS || p->type == TK_PLUS || p->type == TK_MULT || p->type == TK_DIV ||
            p->type == TK_LEFTP) {
            stack[count++] = p;
        } else if (p->type == TK_RIGHTP) {
            while (stack[--count]->type != TK_LEFTP) {
            }
        }
        p++;
    }
    //todo 寻找优先级最低的符号
    int index = --count;
    while (--count >= 0) {
        if (stack[index]->type > stack[count]->type) {
            index = count;
        }
    }
    return stack[index];
}


bool parentheses(Token *p, Token *q) {
    if (p->type != TK_LEFTP || q->type != TK_RIGHTP) {
        return 0;
    }
    p++;
    q--;
    int count = 0;
    while (p <= q) {
        if (p->type == TK_LEFTP) {
            count++;
        } else if (p->type == TK_RIGHTP) {
            if (count == 0) return 0;
            count--;
        }
        p++;
    }
    return count == 0;
}