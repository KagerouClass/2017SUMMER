//
// Created by mashiro on 2017/7/29.
//

#ifndef PJ3_5_MAIN_H
#define PJ3_5_MAIN_H
struct parseInfo
{
    int flag;
    int numberOfParameter;
    int stream;
    char *inFile;
    char *outFile;
    char *prompt;
    char *normalPrompt;
    char *command;
    char *command2;
    char **parameter;
    char **parameter2;
};
struct background
{
    char name[32];
    int pid;
};
struct valueNode
{
    struct valueNode *next;
    char name[32];
    char *value;
};
void init();
void setpath(char *newpath);
void valueLinkList(int shiftNum, char *newName, char *newValue);
void cd_command(char *address);
void clr_command();
void dir_command();
void echo_command(char *content);
void environ_command();
void help_command();
void jobs_command();
void ls_command(char *address);//执行ls指令，列出当前路径下的文件
void pwd_command();//执行pwd，获取当前的工作目录
void set_command(struct parseInfo *info);
void time_command();
int test_command(struct parseInfo *info);
void umask_command(struct parseInfo *info);
int unset_command(struct parseInfo *info);
int normal_cmd(struct parseInfo *info, char* command,int cmdlen,int infd, int outfd, int fork);
int is_internal_cmd(struct parseInfo *info, char* command);
int readcommand(struct parseInfo *info); //读取并分析指令
void parseInfoInitialize(struct parseInfo *info);
void parcing(struct parseInfo *info);
void printPrompt(char *argv[]);
#endif //PJ3_5_MAIN_H
