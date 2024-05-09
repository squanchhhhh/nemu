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
#include <cpu/cpu.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "sdb.h"
#include <memory/paddr.h>

static int is_batch_mode = false;

void init_regex();
void init_wp_pool();

/* We use the `readline' library to provide more flexibility to read from stdin. */
static char* rl_gets() {
  static char *line_read = NULL;

  if (line_read) {
    free(line_read);
    line_read = NULL;
  }

  line_read = readline("(nemu) ");

  if (line_read && *line_read) {
    add_history(line_read);
  }

  return line_read;
}

static int cmd_c(char *args) {
  cpu_exec(-1);
  return 0;
}


static int cmd_q(char *args) {
  return -1;
}
/*
static int cmd_si(char *args){
  if (args){
    cpu_exec(*(uint64_t *)args);
  }
  else{cpu_exec(1);}
  return 0;
}
*/
static int cmd_si(char *args) {
  if (args && *args) {
    // 确保 args不是一个空指针
    char *endptr;
    uint64_t num = strtoull(args, &endptr, 10); // 假设是十进制数字
    if (*endptr == '\0') { // 确保整个字符串都是数字
      cpu_exec(num);
      return 0;
    }
  }
  // 如果 args 为 NULL，或者转换失败，或者 args 不是纯数字
  cpu_exec(1);
  return 0;
}
static int cmd_x(char *args) {
    char *length_str = strtok(NULL, " ");
    if (length_str == NULL) {
        printf("Error: No length specified.\n");
        return -1;
    }
    int l = atoi(length_str);

    char *offset_str = strtok(NULL, " ");
    if (offset_str == NULL) {
        printf("Error: No offset specified.\n");
        return -1;
    }
    char *endptr;
    paddr_t offset = strtoul(offset_str, &endptr, 16);
    if (*endptr != '\0') {
        printf("Error: Invalid offset format.\n");
        return -1;
    }

    // 执行内存读取和打印
    for (; l > 0; l--) {
        uint32_t value = paddr_read(offset, 4);
        printf("%x: %08x\n", offset, value);
        offset += 4;
    }
    return 0;
}
static int cmd_info(char* args){
  if (args&&*args){
    if (strcmp(args,"r")==0){
      isa_reg_display();
      return 0;
    }
  }else{
    printf("usage info [r]");
    return 0;
  }
  return 0;
}
static int cmd_p(char * args){
    bool success = true;
    expr(args,&success);
    return 0;
}
static int cmd_help(char *args);

static struct {
  const char *name;
  const char *description;
  int (*handler) (char *);
} cmd_table [] = {
  { "help", "Display information about all supported commands", cmd_help },
  { "c", "Continue the execution of the program", cmd_c },
  { "q", "Exit NEMU", cmd_q },
  { "si","program excutes n steps", cmd_si},
  {"info","r to show regs,w to show monitor",cmd_info},
  {"x","usage x [N] [EXPR]",cmd_x},
  {"p","p EXPR",cmd_p},
  /* TODO: Add more commands */
};

#define NR_CMD ARRLEN(cmd_table)

static int cmd_help(char *args) {
  /* extract the first argument */
  char *arg = strtok(NULL, " ");
  int i;

  if (arg == NULL) {
    /* no argument given */
    for (i = 0; i < NR_CMD; i ++) {
      printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
    }
  }
  else {
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(arg, cmd_table[i].name) == 0) {
        printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        return 0;
      }
    }
    printf("Unknown command '%s'\n", arg);
  }
  return 0;
}

void sdb_set_batch_mode() {
  is_batch_mode = true;
}

void sdb_mainloop() {
  if (is_batch_mode) {
    cmd_c(NULL);
    return;
  }

  for (char *str; (str = rl_gets()) != NULL; ) {
    char *str_end = str + strlen(str);

    /* extract the first token as the command */
    char *cmd = strtok(str, " ");
    if (cmd == NULL) { continue; }

    /* treat the remaining string as the arguments,
     * which may need further parsing
     */
    char *args = cmd + strlen(cmd) + 1;
    if (args >= str_end) {
      args = NULL;
    }

#ifdef CONFIG_DEVICE
    extern void sdl_clear_event_queue();
    sdl_clear_event_queue();
#endif

    int i;
    for (i = 0; i < NR_CMD; i ++) {
      if (strcmp(cmd, cmd_table[i].name) == 0) {
        if (cmd_table[i].handler(args) < 0) { return; }
        break;
      }
    }

    if (i == NR_CMD) { printf("Unknown command '%s'\n", cmd); }
  }
}

void init_sdb() {
  /* Compile the regular expressions. */
  init_regex();

  /* Initialize the watchpoint pool. */
  init_wp_pool();
}
