#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */
//以下是flag的bit掩码值，意义见宏定义
//the follows are the mask of flag. The meaning of the mask are the name of Macro definition
#define BACKGROUND 1
#define IN_REDIRECT 2
#define OUT_REDIRECT_OVERWRITE 4
#define OUT_REDIRECT_ADDITION 8
#define IS_PIPE 16
#define FRONTGROUND 32

#define MAX_NUMBER_OF_ARG 10
#define STANDARDINPUT 0
#define STANDARDOUTPUT 1
#define STANDARDERROR 2
#define INFINITY 100000000

enum bool {true = 1, false = 0};

extern char **environ;//外部全局变量环境指针
                      //External global variable environment pointer

static struct valueNode *mylinkList = NULL; //负责管理myshell中$1-$9与自定义变量的存储链表
                                            //Responsible for managing the linked list which storages $1-$9 and custom variables in myshell
static struct valueNode *currentEnd = NULL; //上面提及的链表的结尾
                                            //The end of the linked list mentioned above
struct background bgTable[80];  //负责记录在后台运行的程序有哪些
                                //Responsible for recording which programs are running in the background
char currentShellAddress[1000] = { '\0' }; //负责记录当前myshell的执行文件所处的路径
                                           //Responsible for recording the path of the current myshell's position
int cnt; //负责记录当前myshell执行文件所处的路径的字符串长度
        //Responsible for recording the length of the path string(from home to the current position)
int backGroundNum = 0; //负责记录有多少个后台程序在运行
                       //Responsible for recording how many background programs are running
void init()//进行myshell的初始化 Initialize myshell
{
    readlink("/proc/self/exe", currentShellAddress, 1000); //读取当前myshell所处的路径
                                                           //Read the current myshell's position
    setpath(currentShellAddress);//设置默认初始工作目录
                                 //Set the default initial working directory
    setenv("shell", currentShellAddress, 1);//写入环境变量
                                            //Write into environment variables
    mylinkList = malloc(sizeof(struct valueNode));
    struct valueNode *current = mylinkList;
    int i = 1;
    for(; i <= 9; ++i)//创建$1-$9的链表以便使用
    {                 //Create a $1-$9 linked list to use
        memset(current->name, 0, 32 * sizeof(char));
        sprintf(current->name, "%d", i);//将数字转换成字符串 Convert numbers to strings
        current->value = NULL;
        current->next = malloc(sizeof(struct valueNode));
        current = current->next;
    }
    current->next = NULL;
    currentEnd = current;
    /*current = mylinkList;
    for(i = 1; i <= 9; ++i)
    {
        printf("%s", current->name);
        current = current->next;
    }*/
    fflush(stdout);//强制刷新标准输出 Force the program to refresh standard output
}

void setpath(char *newpath)//设置新的当前工作路径 Set myshell's new position
{
    chdir(newpath);
}

void valueLinkList(int shiftNum, char *newName, char *newValue)//这个函数将维护一个链表以用于echo的指令展开或者shift操作
{                                                              //This function will maintain a linked list for echo expansion or shift operations.
    struct valueNode *current = mylinkList;
    if(0 == shiftNum)//不需要左移的情况 No need to move left
    {
        while(NULL != current)//循环，遍历是否有结点与欲插入值相同
        {                     //Loop traversal to find if there is a node that has the same value as inserted one
            if(strcmp(current->name, newName) == 0)//当前输入已经存在 The current input already exists
            {
                free(current->value);
                current->value = malloc(sizeof(char) * strlen(newValue));//为新值分配空间 Allocate space for new values
                strcpy(current->value, newValue);//复制新值 Copy new value
                return;
            }
            current = current->next;
        }
        currentEnd->next = malloc(sizeof(struct valueNode));//创建新的结点 Create a new node
        currentEnd = currentEnd->next;//进队（伪）add new node
        strcpy(currentEnd->name, newName);//复制新名字，名字不拿超过31个字节 Copy the new name, the name cannot exceed 31 
        currentEnd->value = malloc(sizeof(char) * strlen(newValue));//为新值分配空间 Allocate space for new values
        strcpy(currentEnd->value, newValue);//复制新值 Copy new value
    }
    else
    {
        free(mylinkList->value);//把$1的值给释放了 Released the value of $1
        int i = 1;
        switch(shiftNum)
        {
            case 1:
                for(; i < 9; ++i)//左移1位 Shift 1 node left 
                {
                    current->value = current->next->value;//左移 Shift left
                    current = current->next;
                }
                current->value = NULL;//最右边1位设为空值 The rightmost 1 node is set to null
                break;
            case 2:
                for(; i < 8; ++i)//左移2位 Shift 2 node left
                {
                    current->value = current->next->next->value;//左移 Shift left
                    current = current->next;
                }
                for(i = 0; i < 2; ++i)//最右边2位设为空值 The rightmost 2 node is set to null
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 3:
                for(; i < 7; ++i)//左移3位 Shift 3 node left
                {
                    current->value = current->next->next->next->value;//左移 Shift left
                    current = current->next;
                }
                for(i = 0; i < 3; ++i)//最右边3位设为空值 The rightmost 3 node is set to null
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 4:
                for(; i < 6; ++i)//左移4位 Shift 4 node left
                {
                    current->value = current->next->next->next->next->value;//左移 Shift left
                    current = current->next;
                }
                for(i = 0; i < 3; ++i)//最右边4位设为空值 The rightmost 4 node is set to null
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 5:
                for(; i < 5; ++i)//左移5位 Shift 5 node left
                {
                    current->value = current->next->next->next->next->next->value;//左移 Shift left
                    current = current->next;
                }
                for(i = 0; i < 5; ++i)//最右边5位设为空值 The rightmost 5 node is set to null
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 6:
                for(; i < 4; ++i)//左移6位 Shift 6 node left
                {
                    current->value = current->next->next->next->next->next->next->value;//左移 Shift left
                    current = current->next;
                }
                for(i = 0; i < 6; ++i)//最右边6位设为空值 The rightmost 6 node is set to null
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 7:
                for(; i < 3; ++i)//左移7位 Shift 7 node left
                {
                    current->value = current->next->next->next->next->next->next->next->value;//左移 Shift left
                    current = current->next;
                }
                for(i = 0; i < 7; ++i)//最右边7位设为空值 The rightmost 7 node is set to null
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 8:
                for(; i < 2; ++i)//左移8位 Shift 8 node left
                {
                    current->value = current->next->next->next->next->next->next->next->next->value;//左移 Shift left
                    current = current->next;
                }
                for(i = 0; i < 8; ++i)//最右边8位设为空值 The rightmost 8 node is set to null
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 9:
                current->value = current->next->next->next->next->next->next->next->next->next->value;//左移 Shift left
																									  //左移2位
				current = current->next;
                for(i = 0; i < 9; ++i)//最右边9位设为空值 The rightmost 9 node is set to null
                {
                    current->value = NULL;
                    current = current->next;
                }
            default:break;
        }
    }
}

void cd_command(char *address)//跳转目录 jump to other position
{
    setpath(address);
}

void clr_command()//清屏 Clear screen
{
    printf("\033[1A\033[2J\033[H");//\033[1A是到上一行 \033[2J是清屏 \033[H是到最顶行
}                                  //\033[1A is to the previous line;
                                   //\033[2J is to clear screen;
                                   //\033[H is to the top line.

void dir_command()//列出当前目录下有什么文件（非隐藏）
{                 //List all files which is not hiddened in the current position
    char address[80];
    getcwd(address, sizeof(address));//获得当前的工作路径 get current position
    ls_command(address);
}

void echo_command(char *content)//打印给定字符串 Print the given string
{
    printf("%s", content);
}

void environ_command()//打印目前的环境变量 Print current environment variables
{
    char **env = environ;
    while(*env)
    {
        printf("%s\n", *env);
        env++;
    }
}

void help_command()//打印用户手册 Print user manual
{
    FILE *fp = NULL;
    char c;
    int i = 0;
    char currentHelpAddress[1006] = { '\0' };
    strcpy(currentHelpAddress, currentShellAddress);
    for(i = cnt; i >=0; --i)//需要修改成readme文件所在的路径 Need to be modified to the path where the readme file is located
        if(currentHelpAddress[i] == '/')//换成readme的路径 Change to the path of readme
        {
            currentHelpAddress[i+1] = 'r';
            currentHelpAddress[i+2] = 'e';
            currentHelpAddress[i+3] = 'a';
            currentHelpAddress[i+4] = 'd';
            currentHelpAddress[i+5] = 'm';
            currentHelpAddress[i+6] = 'e';
            currentHelpAddress[i+7] = '\0';
            break;
        }
    fp = fopen(currentHelpAddress, "r");//打开用户手册 open manual
    if(NULL == fp)//打开失败 fail to open
        return -1;
    while(fscanf(fp, "%c", &c) != EOF)//打印文本内容 print the manual on the screen
        printf("%c", c);
    fclose(fp);//关闭文件 close file
    fp = NULL;
}

void jobs_command()//列出在后台运行的程序 List programs running in the background
{
    for(int i = 0; i < backGroundNum; ++i)
    {
        printf("%d ", i);
        printf("%s\n", bgTable[i].name);

    }
}

void ls_command(char *address)//执行ls指令，列出当前路径下的文件
{                             //Run the ls command to list all files in the current position.
    DIR *dir = NULL;
    struct dirent *direntp = NULL;
    dir = opendir(address);//打开相应的路径 Open the corresponding path
    if(dir != NULL)//当前目录下有文件 There are files in the current directory
    {
        while(true)
        {
            direntp = readdir(dir);
            if(direntp == NULL)//已经没有文件可以供输出了 No file name is available for output
                break;
            else if(direntp->d_name[0] != '.')
                printf("%s\n", direntp->d_name);
        }
        closedir(dir);//关闭打开的目录 Close open directory
    }

}

void pwd_command()//执行pwd，获取当前的工作目录 Execute pwd to get the current working directory
{
    char buf[80];
    getcwd(buf, sizeof(buf));
    printf("%s\n", buf);
}

void set_command(struct parseInfo *info)//打印或者设置环境变量 Print or set environment variables
{
    if(!info->parameter[1] || strcmp(info->parameter[1], "-a") != 0)//查看环境变量 View environment variables
    {
        environ_command();
        return;
    }
    else
    {
        struct valueNode *current = mylinkList;
        while(current != NULL)//遍历寻找是否有自定义变量存在于myshell维护的链表中
        {                     //Traverse to find if there are custom variables in the linked list maintained by myshell
            if(strcmp(info->parameter[2], current->name) == 0)//找到了 get it
            {
                setenv(current->name, current->value, 1);//设置新的环境变量 Set new environment variables
                break;
            }
            current = current->next;
        }
    }
}

void time_command()//打印系统时间 Printing system time
{
    char *dayOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};//枚举星期 Enumerate week
    time_t timePointer;
    struct tm *p;
    time(&timePointer);
    p = localtime(&timePointer);//拿到计算机本地时间 get local time
    printf("%d/%d/%d ", (1900+p->tm_year), (p->tm_mon), p->tm_mday);
    printf("%s %d:%d:%d\n", dayOfWeek[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
}

int test_command(struct parseInfo *info)//测试表达式的真伪 Test the value of the expression
{
    //字符串测试部分
    if(strcmp(info->parameter[1], "-n") == 0)// test -n string 字符串长度非0 String length is not 0
    {
        if(strlen(info->parameter[2]) != 0)
            return 1;//字符串长度不为0，返回1表示表达式为真 
        else         //The length of the string is not 0, and a value of 1 indicates that the expression is true.
            return 0;//否则为假 Otherwise false
    }

    if(strcmp(info->parameter[1], "-z") == 0)// test -z string1
    {
        if(strlen(info->parameter[2]) == 0)
            return 1;//字符串长度为0，返回1表示表达式为真
        else         //The length of the string is 0. A return of 1 indicates that the expression is true.
            return 0;//否则为假 Otherwise false
    }

    if(strcmp(info->parameter[2], "=") == 0)// test string1 = string2
    {
        if(strcmp(info->parameter[1], info->parameter[3]) == 0)
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else         //The two strings are identical, returning 1 means the expression is true
            return 0;//否则为假 Otherwise false
    }

    if(strcmp(info->parameter[2], "!=") == 0)// test string1 != string2
    {
        if(strcmp(info->parameter[1], info->parameter[3]) == 0)
            return 0;//两个字符串完全相同，返回0表示表达式为假
        else         //The two strings are identical, returning 0 means the expression is false
            return 1;//否则为真 Otherwise false
    }
    //整数判断部分
    if(strcmp(info->parameter[2], "-eq") == 0)// test int1 -eq int2 ==
    {
        if(strcmp(info->parameter[1], info->parameter[3]) == 0)
            return 1;//两个字符串完全相同，则其值必定相同，返回1表示表达式为真
        else         //If the two strings are identical, the values must be the same, and a value of 1 means the expression is true.
            return 0;//否则为假 Otherwise false
    }
    if(strcmp(info->parameter[2], "-ne") == 0)// test int1 -ne int2 !=
    {
        if(strcmp(info->parameter[1], info->parameter[3]) != 0)
            return 1;//两个字符串不完全相同，则其值必定不同，返回1表示表达式为真
        else         //If the two strings are not identical, their values must be different. A return of 1 means the expression is true.
            return 0;//否则为假 Otherwise false
    }
    if(strcmp(info->parameter[2], "-ge") == 0)// test int1 -ge int2 >=
    {
        if(atoi(info->parameter[1]) >= atoi(info->parameter[3]))
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else         //The two strings are identical, returning 1 means the expression is true.
            return 0;//否则为假 Otherwise false
    }
    if(strcmp(info->parameter[2], "-gt") == 0)// test int1 -gt int2 >
    {
        if(atoi(info->parameter[1]) > atoi(info->parameter[3]))
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else         //The two strings are identical, returning 1 means the expression is true.
            return 0;//否则为假 Otherwise false
    }
    if(strcmp(info->parameter[2], "-le") == 0)// test int1 -le int2 <=
    {
        if(atoi(info->parameter[1]) <= atoi(info->parameter[3]))
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else         //The two strings are identical, returning 1 means the expression is true.
            return 0;//否则为假 Otherwise false
    }
    if(strcmp(info->parameter[2], "-lt") == 0)// test int1 -lt int2 <
    {
        if(atoi(info->parameter[1]) < atoi(info->parameter[3]))
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else         //The two strings are identical, returning 1 means the expression is true.
            return 0;//否则为假 Otherwise false
    }
}

void umask_command(struct parseInfo *info)//修改系统掩码值 Modify the system mask value
{                                           
    if(info->parameter[1] && (strcmp(info->parameter[1], "\0") != 0))//用户处于设置状态 User is in the setup state
    {
        int mask =0;
        mask = strtol(info->parameter[1], NULL, 8);
        umask((mode_t)mask);
    }
    else//否则返回当前的系统掩码值 Otherwise return the current system mask value
    {
        int mask = 0;
        mask = umask(S_IRUSR | S_IWUSR | S_IXUSR);//修改成任意其他值，该函数返回值为修改前的掩码值
        //Modified to any other value, the function returns the value of the mask before the modification
        printf("%3.o", mask);
        fflush(stdout);//强制刷新 Forced refresh
        umask(mask);
    }
}

int unset_command(struct parseInfo *info)//删除现有的变量，需要从链表与环境变量两个地方删除
{//Delete existing variables, you need to delete from the linked list and environment variables
    struct valueNode *current = mylinkList->next;
    struct valueNode *before = mylinkList;
    //因为本人定义的这个链表开始10个结点为内置的参数，所以不可删除，
    //Because the list defined by me starts with 10 nodes as built-in parameters, it cannot be deleted.
    //因此不检查最开始的那个结点是否满足删除要求
    //Therefore, it is not checked whether the first node meets the deletion requirement.
    while(current != NULL)//开始遍历检查链表 Start traversing the checklist
    {
        if(strcmp(current->name, info->parameter[1]) == 0)//存在符合条件的 Qualified
        {
            if(currentEnd == current)
            {
                currentEnd = before;
            }
            if(current->value != NULL)//把值的空间释放掉 Release the space
                free(current->value);
            before->next = current->next;//走向下一个结点 Move to next node
            struct valueNode *tmp = current;
            current = current->next;
            free(tmp);//释放当前位置的空间 release the space of current position
        }
        before = before->next;
        if(current != NULL)
            current = current->next;
    }
    unsetenv(info->parameter[1]);//删除变量 delete variable
}

int normal_cmd(struct parseInfo *info, char* command,int cmdlen,int infd, int outfd, int fork)
{
    return is_internal_cmd(info, info->command);
}

int is_internal_cmd(struct parseInfo *info, char* command)//这个函数用来执行各项内部指令，不同内部指令进入不同分支
{//This function is used to execute various internal instructions, and different internal instructions enter different branches.
    char *endposition = NULL;
    if((endposition = strstr(info->parameter[0], "=")) != NULL) //当前为赋值操作 abc=1 Current operation is assignment, abc=1
    {
        char *newName = malloc(sizeof(char) * (endposition - info->parameter[0] + 1));
        memset(newName, 0, (endposition - info->parameter[0] + 1) *sizeof(char));
        strncpy(newName, info->parameter[0], endposition - info->parameter[0]);//提取abc=1中的变量名abc Extract the variable name abc from abc=1
        endposition++;//让当前指针指向=后面的字符 Let the current pointer point to the character after =
        char *newValue = malloc(sizeof(char) * strlen(endposition));
        memset(newValue, 0, strlen(endposition) * sizeof(char));
        strcpy(newValue, endposition);//提取变量值 Extract variable values
        valueLinkList(0, newName, newValue);
        free(newName);//释放空间，下同 Free up space, the same below
        free(newValue);
    }
    else if(strcmp(info->parameter[0], "bg") == 0)
    {

    }
    else if(strcmp(info->parameter[0], "cd") == 0)//切换工作目录 Switch working directory
    {
        cd_command(info->parameter[1]);
    }
    else if(strcmp(info->parameter[0], "clr") == 0)//清屏 Clear Screen
    {
        clr_command();
    }
    else if(strcmp(info->parameter[0], "dir") == 0)//列出当前工作目录下文件 List all files under current position
    {
        dir_command();
    }
    else if(strcmp(info->parameter[0], "echo") == 0)//回响 echo
    {
        echo_command(info->parameter[1]);
    }
    else if(strcmp(info->parameter[0], "environ") == 0)//显示环境变量 Display environment variable
    {
        environ_command();
    }
    else if(strcmp(info->parameter[0], "exit") == 0)//退出shell exit
    {
        return INFINITY;
    }
    else if(strcmp(info->parameter[0], "exec") == 0)//将外部程序直接覆盖bash的进程 Directly overwrite bash processes with external programs
    {
        execvp(info->parameter[1], NULL);
        return INFINITY;
    }
    else if(strcmp(info->parameter[0], "help") == 0)//打印用户手册 print manual
    {
        help_command();
    }
    else if(strcmp(info->parameter[0], "jobs") == 0)//打印后台程序 print all program running in background
    {
        jobs_command();
    }
    else if(strcmp(info->parameter[0], "ls") == 0)//列出指定文件夹下的目录 list all the file and folder in the requested position
    {
        ls_command(info->parameter[1]);
    }
    else if(strcmp(info->parameter[0], "pwd") == 0)//显示当前工作目录 print current position
    {
        pwd_command();
    }
    else if(strcmp(info->parameter[0], "quit") == 0)//退出shell exit shell
    {
        return INFINITY;
    }
    else if(strcmp(info->parameter[0], "set") == 0)//设置新的环境变量 Set new environment variables
    {
        set_command(info);
    }
    else if(strcmp(info->parameter[0], "shift") == 0)//左移参数 Left shift parameter
    {
        valueLinkList(atoi(info->parameter[1]), NULL, NULL);
    }
    else if(strcmp(info->parameter[0], "time") == 0)//打印时间 print time
    {
        time_command();
    }
    else if(strcmp(info->parameter[0], "test") == 0)//测试表达式 test expression
    {
        int result = 0;
        result = test_command(info);
        printf("test reslut is: %d", result);
    }
    else if(strcmp(info->parameter[0], "umask") == 0)//设置系统掩码 set system mask
    {
        umask_command(info);
    }
    else if(strcmp(info->parameter[0], "unset") == 0)//移除变量 Remove variable
    {
        unset_command(info);
    }
    else//否则就是普通的外部程序，默认执行的是/bin下的程序，其他外部程序请自己输入详细路径
    {//Otherwise, it is a normal external program. The default is to execute the program under /bin. 
    //For other external programs, please enter the detailed path yourself.
        /*char currentCommandAddress[1032] = { '\0' };
        strcpy(currentCommandAddress, currentShellAddress);
        int i = 0;
        currentCommandAddress[i] = '\0';
        for(i = cnt; i >=0; --i)
            if(currentCommandAddress[i] == '/')
            {
                strcpy(&currentCommandAddress[i + 1], info->parameter[0]);
                break;
            }*/
        int pid = 0;
        if((pid = fork()) < 0)
        {
            perror("程序运行失败");
        }
        else if (pid == 0)//子程序 Subroutine
        {
            char currentCommandAddress[1032] = { '\0' };
            strcpy(currentCommandAddress, currentShellAddress);//复制当前shell的根路径 Copy the root path of the current shell
            int i = 0;
            currentCommandAddress[i] = '\0';
            for(i = cnt; i >=0; --i)
            if(currentCommandAddress[i] == '/')
            {
                strcpy(&currentCommandAddress[i + 1], "myshell");//作出<pathname>/myshell  <pathname>/myshell
                break;
            }
            setenv("myshell", currentCommandAddress, 1);//本步将写入参数 Write parameter
            execvp(info->parameter[0], info->parameter);
            exit(0);
        }
        else
        {
            waitpid(pid, NULL, 0);//等待子进程结束 Waiting for the child process to end
        }
    }
    return 1;
}

int readcommand(struct parseInfo *info) //读取并分析指令 Read and analyze instructions
{

    char *paramBegin = info->prompt;
    char *paramEnd = info->prompt;
    int paramCount = 0;

    fgets(info->prompt, MAX_LINE, stdin);
    strcpy(info->normalPrompt, info->prompt);
    while(true)
    {
        while(*paramEnd == ' ' || *paramEnd == '\t')//吃掉当前要处理的parameter之前的空格或者tab
        //delete the space or tab before the current parameter to be processed
        {
            ++paramBegin;
            ++paramEnd;
        }

        if(*paramEnd == '\n' || *paramEnd == '\0')//走到最后发现是换行符，结束处理
        {//check whole the sentence and find that there is a newline in the end, end processing
            if(paramCount == 0)
                return -1;//-1表示没有任何parameter -1 means there is no parameter
            break;
        }

        while(*paramEnd != ' ' && *paramEnd != '\t' && *paramEnd != '\0' && *paramEnd != '\n')
        {//paramEnd目前遇到的都是这项parameter的内容
        //What is currently encountered is the content of this parameter.
            ++paramEnd;
        }
        if(paramCount == 0)//如果这是处理的第一个parameter If this is the first parameter processed
        {
            info->command = paramBegin;
            info->parameter[0] = paramBegin;
            ++paramCount;
        }
        else if(paramCount <= MAX_NUMBER_OF_ARG)//没有超过最大限度 Not exceeding the maximum
        {
            info->parameter[paramCount] = paramBegin;
            ++paramCount;
        }
        else//后面的均被忽略 The latter are ignored
            break;
        if(*paramEnd == '\0' || *paramEnd == '\n')//再次检查paramEnd指针是否已经走到最后
        //Check again if the paramEnd pointer has reached the end.
        {
            *paramEnd = '\0';//统一置换成\0以方便处理 Uniformly replaced with \0 for easy handling
            break;
        }
        else
        {
            *paramEnd = '\0';
            ++paramEnd;
            paramBegin = paramEnd;
        }
    }
    info->numberOfParameter = paramCount;
    return paramCount;
    //normal_cmd(command, 300, NULL, NULL, NULL);
}

void parseInfoInitialize(struct parseInfo *info)//将语义分析结构体进行重新初始化，为接收下一条指令做准备
{//Reinitialize the semantic analysis structure, prepare for the next instruction
    info->flag = 0;
    info->numberOfParameter = 0;
    info->inFile = NULL;
    info->outFile = NULL;
    info->command = NULL;
    info->command2 = NULL;
    info->parameter2 = NULL;
}

void parcing(struct parseInfo *info)//语义分析函数
{//Semantic analysis function
    int i = 0;
    for(i = 0; i < info->numberOfParameter;)
    {
        char *endPosition = 0;
        if((endPosition = strstr(info->parameter[i], "$")) != NULL)//当前为赋值表达式 assignment expression
        {
            endPosition++;
            struct valueNode *current = mylinkList;
            while(current != NULL)//再次遍历检查链表是否有可以替换的变量
            {//Traverse again to check if there are variables that can be replaced in the linked list
                if(strcmp(endPosition, current->name) == 0)//找到可以替换的 Found replaceable
                {
                    if(current->value == NULL)//当前结点无值 The current node has no value
                    {
                        printf("This variable has no value!");
                        current = current->next;
                        continue;
                    }
                    info->parameter[i] = malloc(sizeof(char) * strlen(current->value));
                    strcpy(info->parameter[i], current->value);//字符展开 Character expansion
                    break;
                }
                current = current->next;
            }
            if(current == NULL)//没找到可替换的 Did not find a replaceable
            {
                info->parameter[i] = malloc(sizeof(char));
                *(info->parameter[i]) = '\0';//输出空 Output empty
            }
        }
        else if(strcmp(info->parameter[i], "<") == 0 || strcmp(info->parameter[i], "<<") == 0)
        //打上重定向输入的记号 Mark the redirection input
        {
            info->flag |= IN_REDIRECT;
            info->inFile = info->parameter[i + 1]; //标识重定向后的信息流来源 Identify the source of the stream after redirecting
            if(strcmp(info->parameter[i - 1], "0"))
                info->stream = STANDARDINPUT;//标记为标准输入 Mark as standard input
            info->parameter[i] = NULL;
            i += 2;
        }
        else if(strcmp(info->parameter[i], ">") == 0)
        //打上重定向覆盖输出的记号 Mark as the redirect override output
        {
            info->flag |= OUT_REDIRECT_OVERWRITE;
            info->outFile = info->parameter[i + 1]; //标识重定向后的信息流来源 Identify the source of the stream after redirecting
            if(strcmp(info->parameter[i - 1], "1")) //如果是标准输出,标记为标准输出 Marked as standard output if it is a standard output
                info->stream = STANDARDINPUT;
            else if(strcmp(info->parameter[i - 1], "2"))
                info->stream = STANDARDOUTPUT;
            info->parameter[i] = NULL;
            i += 2;
        }
        else if(strcmp(info->parameter[i], ">>") == 0)
        //打上重定向覆盖输出的记号 Mark as the redirect override output
        {
            info->flag |= OUT_REDIRECT_ADDITION;
            info->outFile = info->parameter[i + 1]; //标识重定向后的信息流来源Identify the source of the stream after redirecting
            if(strcmp(info->parameter[i - 1], "1")) //如果是标准输出,标记为标准输出 Marked as standard output if it is a standard output
                info->stream = STANDARDINPUT;
            else if(strcmp(info->parameter[i - 1], "2"))
                info->stream = STANDARDOUTPUT;
            info->parameter[i] = NULL;
            i += 2;
        }
        else if(strcmp(info->parameter[i], "&") == 0) //打上后台运行的标记 Mark as running in the background
        {
            info->flag |= BACKGROUND;
            info->parameter[i] = NULL;
            ++i;
            strcpy(bgTable[backGroundNum].name, info->command);
            backGroundNum++;
        }
        else if(strcmp(info->parameter[i], "|") == 0) //管道指令 pipe
        {
            info->flag |= IS_PIPE;
            info->command2 = info->parameter[i + 1];    //获取管道后的指令 Get the instruction after the |
            info->parameter2 = &(info->parameter[i + 1]);  //获取管道后的指令的第一个参数 get first parameter after |
            info->parameter[i] = NULL;
            i += 2;
        }
        else if(strcmp(info->parameter[i], "fg") == 0)//前台运行命令 running in the Front command
        {
            info->flag |= FRONTGROUND;//打上前台运行标记 mark as running in the front
            info->command = info->parameter[i + 1];
            info->parameter = &(info->parameter[i + 1]);
            --(info->numberOfParameter);//fg不算在主体命令的分词中 Fg is not counted in the participle of the subject command
            ++i;
        }
        else if(strcmp(info->parameter[i], "bg") == 0)//后台运行命令的标记 running commands in the background
        {
            info->flag |= BACKGROUND;
            info->command = info->parameter[i + 1];//重新指向命令 Redirect command
            info->parameter = &(info->parameter[i + 1]);
            --(info->numberOfParameter);
            ++i;
            strcpy(bgTable[backGroundNum].name, info->command);//记录下后台命令 Record background commands
            backGroundNum++;
        }
        else
            ++i;//不需要处理 No need to cope with
    }

}

void printPrompt(char *argv[])//打印命令提示符 Print command prompt
{
    if(argv[1] && strcmp(argv[1], "1") == 0)//是否为脚本模式，交互模式才需要输出命令提示符 Whether it is script mode, interactive mode requires output command prompt
        return;
    //printf("\nPlease make sure this input is right, or you can not execute your script:\n");
    //printf("Please enter now where myshell is: %s", currentShellAddress);
    //scanf("%s", currentShellAddress);
    cnt = readlink("/proc/self/exe", currentShellAddress, 1000);//获取当前myshell的脚本地址 Get the current myshell script address
    if(cnt < 0 || cnt >= 1000)//地址过长或者获取失败 The address is too long or the acquisition failed.
    {
        printf("Open myshell fail!");
        exit(-1);//返回 return
    }
    //printf("%s", currentShellAddress);
    char buf[80];
    getcwd(buf, sizeof(buf));    //取得当前的工作目录 get current position
    printf("\nmyshell>%s/$ ", buf);
}

int main(int argvs, char *argv[])
{
    char *args[MAX_LINE/2 + 1]; /* command line arguments */
    int should_run = 0; /* flag to determine when to exit program */
    struct parseInfo *info = (struct parseInfo*)malloc(sizeof(struct parseInfo));//signal to parse
    int pipe_fd[2] = { 0, 0 };
    int status = 0;
    pid_t childPID = 0;
    info->prompt = (char*)malloc(sizeof(char) * MAX_LINE);
    info->normalPrompt = (char*)malloc(sizeof(char) * MAX_LINE);//当输入的命令只有基本内部指令的时候将使用本变量进行操作
    //This variable will be used when the input command has only basic internal instructions.
    info->parameter = (char**)malloc(sizeof(char*)*(MAX_NUMBER_OF_ARG + 2));
    //用来存储输入的命令各个parameter存在的地方
    //The place where each parameter of the command used to store the input exists
    init();//初始化 Initialization
    parseInfoInitialize(info);//初始化结构体 Initialization structure
    printPrompt(argv);
    fflush(stdout);
    while (true)
    {
        int inFd, outFd;

        readcommand(info);//读取指令并进行预处理 Read instructions and perform preprocessing
        if(-1 == info->numberOfParameter)
            continue;
        parcing(info);//对指令进行语义分析 Semantic analysis of instructions
        if(0 == info->flag)//从指令中什么都没有解析出，那就是普通指令
        {//Nothing else is resolved from the instruction, that is the ordinary instruction

            if(strcmp(info->parameter[0], "myshell") == 0)//是脚本执行指令 Execute script
            {
                int scriptPID;
                if((scriptPID = fork()) < 0)//创建新的进程以来执行脚本 Execute script since creating a new process
                {
                    perror("脚本进程创建失败！");
                }
                else if(scriptPID == 0) // 脚本子进程 child process running Script 
                {
                    int infd = 0;
                    infd = open(info->parameter[1], O_RDONLY | O_CREAT, 0777);
                    dup2(infd, fileno(stdin));//将输入重定向为脚本 Redirect input source, get input from script
                    char *myargvs[80] = { NULL };
                    myargvs[0] = info->parameter[0];
                    myargvs[1] = "1";//传入参数，告诉子进程新开的myshell目前处于脚本模式，不需要打印命令提示符
                    //Incoming parameters, telling the child process that the newly opened myshell is currently in script mode, no need to print a command prompt
                    execvp(currentShellAddress, myargvs);//子进程新运行一个myshell运行脚本内容
                    //The child process newly runs a shell to run the script content
                    exit(0);//结束子进程 End child process
                }
                else
                {
                    waitpid(scriptPID, NULL, 0);//等待子进程返回 Waiting for the child process to return
                    printPrompt(argv);//打印新的命令提示符 Print a new command prompt
                    parseInfoInitialize(info);//初始化结构体 Initialization structure
                    continue;
                }
            }
            //否则就是一般的内部指令执行（注意：由于实现方式的原因，并不是所有的内部指令都被包括在了normal_cmd()这个函数中）
            //Otherwise it is a general internal instruction execution (note: not all internal instructions are included in the normal_cmd() function due to implementation reasons)
            if(normal_cmd(info, info->normalPrompt, strlen(info->normalPrompt), NULL, NULL, NULL)== INFINITY)
            {//当返回值为0时代表输入了一个exit或者quit指令
            //When the return value is 0, it means that an exit or quit command is input.
                break;//退出循环结束bash Exit loop, end bash
            }
            parseInfoInitialize(info);//初始化结构体 Initialization structure
            if(getpid() != 0)
            {
                char buf[80];
                getcwd(buf, sizeof(buf));    //取得当前的工作目录 Get the current working directory
                printf("\nmyshell>%s/$ ", buf);
            }
            continue;
        }
        else if(IS_PIPE & info->flag)
        {
            if(pipe(pipe_fd) < 0)//创建管道，为下面新建子进程做准备
            {//Create a pipeline to prepare for the new child process below
                perror("用于指令的管道创建失败！");
            }
        }//else 即为其他重定向、后台运行等操作 
        //That is, other redirects, background operations, etc.
        if((childPID = fork()) < 0)
        {
            perror("子进程创建失败！");
        }
        else if(childPID == 0)//子进程 Child processs
        {
            if(info->flag & IN_REDIRECT)//flag检测到输入重定向 Input redirection detected in flag.
            {
                inFd = open(info->inFile, O_RDONLY, 0777);//以只读方式打开，打开重定向的文件
                //Open as read-only, open the redirected file
                close(fileno(stdin));//关闭标准输入 close standard input
                dup2(inFd, fileno(stdin));//将原先与标准输入对接的接口重定向到inFd上 
                //Redirect the interface that originally docked with the standard input to inFd
                close(inFd);//结束操作，关闭inFd End operation, close inFd
            }

            if(info->flag & IS_PIPE)//存在管道操作 pipe
            {
                if(info->flag & OUT_REDIRECT_OVERWRITE)//重定向为覆盖写时 Redirect to overwrite when writing
                {
                    close(pipe_fd[0]);
                    close(pipe_fd[1]);//关闭管道，被覆盖 Close the pipe and be covered
                    outFd = open(info->outFile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    //打开输出文件，打开方式为只写，不存在时创建，覆盖写，创建新文件的时候权限为777
                    //Open the output file, open the way to write only, create when it does not exist, overwrite the write, when the new file is created, the permissions are 777
                    close(fileno(stdout));
                    dup2(outFd, fileno(stdout));
                    close(outFd);
                }
                else if(info->flag & OUT_REDIRECT_ADDITION)//重定向为添加写时 Redirect to add write
                {
                    close(pipe_fd[0]);
                    close(pipe_fd[1]);//关闭管道，被覆盖 Close the pipe and be covered
                    outFd = open(info->outFile, O_WRONLY | O_CREAT | O_APPEND, 0777);
                    //打开输出文件，打开方式为只写，不存在时创建，添加写，创建新文件的时候权限为777
                    //Open the output file, open the way to write only, create when it does not exist, add write, create a new file when the permissions are 777
                    close(fileno(stdout));
                    dup2(outFd, fileno(stdout));
                    close(outFd);
                }
                else//单纯的管道操作 Pipe operate
                {
                    close(fileno(stdout));//关闭标准输出 Close standard output
                    close(pipe_fd[0]);//关闭管道读 Close pip read
                    dup2(pipe_fd[1], fileno(stdout)); //将标准输出去掉，改为向管道输入东西 Remove the standard output and change the input to the pipe
                    close(pipe_fd[1]);//关闭管道写 Close pipe write
                }

            }
            else // 不存在管道操作 No pipe
            {
                if(info->flag & OUT_REDIRECT_OVERWRITE) //重定向输出为覆盖写时 Redirect output to overwrite when writing
                {
                    outFd = open(info->outFile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    //打开输出文件，打开方式为只写，不存在时创建，覆盖写，创建新文件的时候权限为777
                    //Open the output file, open the way to write only, create when it does not exist, add write, create a new file when the permissions are 777
                    close(fileno(stdout));
                    dup2(outFd, fileno(stdout));//将原先与标准输出对应的接口接到管道的输入端上
                    //Connect the interface corresponding to the standard output to the input of the pipe
                    close(outFd);
                }
                else if(info->flag & OUT_REDIRECT_ADDITION)//重定向为添加写时 Redirect to add write
                {
                    outFd = open(info->outFile, O_WRONLY | O_CREAT | O_APPEND, 0777);
                    //打开输出文件，打开方式为只写，不存在时创建，添加写，创建新文件的时候权限为777
                    //Open the output file, open the way to write only, create when it does not exist, add write, create a new file when the permissions are 777
                    close(fileno(stdout));
                    dup2(outFd, fileno(stdout));//将原先与标准输出对应的接口接到管道的输入端上
                    //Connect the interface corresponding to the standard output to the input of the pipe
                    close(outFd);
                }
            }
            sleep(0.5);
            normal_cmd(info, info->command, NULL, NULL, NULL, 1);//设置好全部的重定向管道信息以后执行相关的指令
            //After setting all the redirected pipeline information, execute the relevant instructions.
            exit(0);
        }
        else//父进程 Parent process
        {

            int pipeChildPID = 0;
            int i = 0;
            if(info->flag & IS_PIPE)//存在管道指令，将开新的子进程接收来自管道的数据
            //The command requests a new pipe, so it will open a new child process to receive data from the pipe
            {
                if((pipeChildPID = fork()) < 0)
                {
                    perror("主进程管道子进程创建失败");
                }
                else if (pipeChildPID == 0)//子进程，接收管道数据 Child process, receiving pipe data
                {
                    close(pipe_fd[1]);//关闭管道写 Close pipe write
                    close(fileno(stdin));
                    //char buf[80] = { '\0' };
                    //read(pipe_fd[0], buf, 80);
                    if(info->flag & BACKGROUND)//存在后台运行 background running
                    {
                        char bufin[80];
                        getcwd(bufin, sizeof(bufin));    //取得当前的工作目录 Get the current working directory
                        printf("myshell>%s", bufin);
                        printf("/$ ");
                        fflush(stdout);
                        //printf("read from pipe: %s\n", buf);
                        //parseInfoInitialize(info);//初始化结构体 Initialization structure
                        //continue;
                    }
                    //printf("read from pipe: %s\n", buf);//debug使用 use in debugging
                    dup2(pipe_fd[0], fileno(stdin));//将原本与标准输入相对应的接口转接到管道的出口上
                                                    //Transfer the interface that originally corresponds to the standard input to the exit of the pipe
                    close(pipe_fd[0]);
                    char *myargvs[100] = { NULL };
                    int i = 0;
                    while (info->parameter2[i])
                    {
                        myargvs[i] = info->parameter2[i];
                        i++;
                    }
                    myargvs[i] = NULL;
                    //myargvs[i] = &buf[0];
                    //myargvs[++i] = NULL;
                    i = 0;

                    execvp(info->command2, info->parameter2);//执行外部程序 Execution of external programs
                    exit(0);
                }
                else
                {
                    waitpid(pipeChildPID, &status, 0);//等待管道信息接收并执行完毕 Wait for the pipeline information to be received and executed
                }
            }
            if ((info->flag & BACKGROUND) && !(info->flag & IS_PIPE))//如果是后台运行的进程，无需等待其结束即可进入下一指令的接收
            {                                                        //If it is a process running in the background, you can enter the next instruction without waiting for it to end.
                printPrompt(argv);
                fflush(stdout);
                parseInfoInitialize(info);//初始化结构体 Initialization structure
                continue;
            }
            else if ((info->flag & BACKGROUND) && (info->flag & IS_PIPE))//理由同上 The reason is the same as above
            {
                parseInfoInitialize(info);//初始化结构体 Initialization structure
                continue;
            }
            else
            {
                waitpid(childPID, NULL, 0);//等待子进程结束才能进行下一步操作
                                           //Wait for the child process to finish before proceeding to the next step
                printPrompt(argv);
                parseInfoInitialize(info);//初始化结构体 Initialization structure
            }
        }
    }
    free(info->parameter);//释放空间，下同 Free up space, the same below
    free(info);
    return 0;
}