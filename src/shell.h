#ifndef SHELL_H
#define SHELL_H

// Kendi typedef'lerimiz
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

// Forward declaration
struct banner;

// Shell constants
#define MAX_COMMAND_LENGTH 256
#define MAX_ARGS 16

// Shell fonksiyonları
void shell_init();
void shell_run();
void shell_print_prompt();
void shell_execute_command(char* command);
int shell_readline(char* buf, int maxlen);

// Banner management
void shell_set_banner(struct banner* banner);

// Built-in commands
void cmd_help();
void cmd_clear();
void cmd_echo(char* args);
void cmd_ls();
void cmd_cat(char* filename);
void cmd_pwd();
void cmd_whoami();
void cmd_date();
void cmd_uname();
void cmd_save();
void cmd_disktest();
void cmd_mkdir(char* dirname);
void cmd_cd(char* dirname);
void cmd_rm(char* filename);
void cmd_touch(char* filename);
void cmd_cp(char* args);
void cmd_mv(char* args);
void cmd_ls_param(char* param);
void cmd_mountfat32();
void cmd_lsfat32();
void cmd_catfat32(char* filename);
void cmd_rmfat32(char* filename);
void cmd_mkdirfat32(char* dirname);
void cmd_touchfat32(char* filename);
void cmd_cdfat32(char* dirname);
void cmd_banner();

#endif 