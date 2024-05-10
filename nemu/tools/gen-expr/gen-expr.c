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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#include <time.h>
// this should be enough
static char buf[65536] = {};
static char code_buf[65536 + 128] = {}; // a little larger than `buf`
static char *code_format =
"#include <stdio.h>\n"
"int main() { "
"  unsigned result = %s; "
"  printf(\"%%u\", result); "
"  return 0; "
"}";
static void gen_rand_op(){
    const char ops[] = {'+', '-', '*', '/'};
    int index = rand() % 4;
    char op_str[2] = {ops[index], '\0'};
    strcat(buf, op_str);
  }

static int choose(int n){
  return rand()%n;
}
static void gen_num(){
  char num_str[20];
  int num = rand() % 100;  // 生成 0 到 99 之间的随机数
  sprintf(num_str, "%d", num);
  strcat(buf, num_str);
}
static void gen(char p){
  char p_str[2] = {p, '\0'};
  strcat(buf, p_str);
}
static void gen_rand_expr() {
  int complete = 0;  // 标志变量,用于跟踪表达式是否完整
  
  while (!complete) {
    if (strlen(buf) >= 100) {
      break;  // 如果达到长度限制,退出循环
    }
    
    switch (choose(3)) {
      case 0:
        gen_num();
        complete = 1;  // 生成数字后,表达式完整
        break;
      case 1:
        gen('(');
        gen_rand_expr();
        if (strlen(buf) < 100) {
          gen(')');
          complete = 1;  // 生成括号表达式后,表达式完整
        }
        break;
      default:
        gen_rand_expr();
        if (strlen(buf) < 100) {
          gen_rand_op();
          gen_rand_expr();
          complete = 1;  // 生成二元运算表达式后,表达式完整
        }
        break;
    }
  }
}

int main(int argc, char *argv[]) {
  int seed = time(0);
  srand(seed);
  int loop = 1;
  if (argc > 1) {
    sscanf(argv[1], "%d", &loop);
  }
  int i;
  for (i = 0; i < loop; i ++) {
    buf[0] = '\0';
    gen_rand_expr();

    sprintf(code_buf, code_format, buf);

    FILE *fp = fopen("/tmp/.code.c", "w");
    assert(fp != NULL);
    fputs(code_buf, fp);
    fclose(fp);

    int ret = system("gcc /tmp/.code.c -o /tmp/.expr");
    if (ret != 0) continue;

    fp = popen("/tmp/.expr", "r");
    assert(fp != NULL);

    int result;
    ret = fscanf(fp, "%d", &result);
    pclose(fp);

    printf("%u %s\n", result, buf);
  }
  return 0;
}
