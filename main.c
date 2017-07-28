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

enum bool {true = 1, false = 0};
void init();
void setpath(char* newpath);   	/*设置搜索路径*/
int readcommand(struct parseInfo *info);			/*读取用户输入*/
int is_internal_cmd(struct parseInfo *info, char* command); /*解析内部命令*/
int is_pipe(char* cmd,int cmdlen); 		/*解析管道命令*/
int is_io_redirect(char* cmd,int cmdlen);   /*解析重定向*/
int normal_cmd(struct parseInfo *info, char* command,int cmdlen,int infd, int outfd, int fork);  /*执行普通命令*/
/*其他函数…… */
void parseInfoInitialize(struct parseInfo *info);

extern char **environ;
struct valueNode
{
    struct valueNode *next;
    char name[32];
    char *value;
};
static struct valueNode *mylinkList = NULL;
static struct valueNode *currentEnd = NULL;
char currentShellAddress[1000] = { '\0' };
int cnt;
void init()
{
    setpath(currentShellAddress);//设置默认初始工作目录
    mylinkList = malloc(sizeof(struct valueNode));
    struct valueNode *current = mylinkList;
    int i = 1;
    for(; i <= 9; ++i)//创建$1-$9的链表以便使用
    {
        memset(current->name, 0, 32 * sizeof(char));
        sprintf(current->name, "%d", i);
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
    fflush(stdout);
}

void setpath(char *newpath)/*"/bin:/usr/bin"*/
{
    chdir(newpath);
}

void valueLinkList(int shiftNum, char *newName, char *newValue)//这个函数将维护一个链表以用于echo的指令展开或者shift操作
{
    struct valueNode *current = mylinkList;
    if(0 == shiftNum)//不需要左移的情况
    {
        while(NULL != current)
        {
            if(strcmp(current->name, newName) == 0)//当前输入已经存在
            {
                free(current->value);
                current->value = malloc(sizeof(char) * strlen(newValue));//为新值分配空间
                strcpy(current->value, newValue);//复制新值
                return;
            }
            current = current->next;
        }
        currentEnd->next = malloc(sizeof(struct valueNode));//创建新的结点
        currentEnd = currentEnd->next;//进队（伪）
        strcpy(currentEnd->name, newName);//复制新名字，名字不拿超过31个字节
        currentEnd->value = malloc(sizeof(char) * strlen(newValue));//为新值分配空间
        strcpy(currentEnd->value, newValue);//复制新值
    }
    else
    {
        free(mylinkList->value);//把$1的值给释放了
        int i = 1;
        switch(shiftNum)
        {
            case 1:
                for(; i < 9; ++i)
                {
                    current->value = current->next->value;//左移
                    current = current->next;
                }
                current->value = NULL;
                break;
            case 2:
                for(; i < 8; ++i)
                {
                    current->value = current->next->next->value;//左移
                    current = current->next;
                }
                for(i = 0; i < 2; ++i)
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 3:
                for(; i < 7; ++i)
                {
                    current->value = current->next->next->next->value;//左移
                    current = current->next;
                }
                for(i = 0; i < 3; ++i)
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 4:
                for(; i < 6; ++i)
                {
                    current->value = current->next->next->next->next->value;//左移
                    current = current->next;
                }
                for(i = 0; i < 3; ++i)
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 5:
                for(; i < 5; ++i)
                {
                    current->value = current->next->next->next->next->next->value;//左移
                    current = current->next;
                }
                for(i = 0; i < 5; ++i)
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 6:
                for(; i < 4; ++i)
                {
                    current->value = current->next->next->next->next->next->next->value;//左移
                    current = current->next;
                }
                for(i = 0; i < 6; ++i)
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 7:
                for(; i < 3; ++i)
                {
                    current->value = current->next->next->next->next->next->next->next->value;//左移
                    current = current->next;
                }
                for(i = 0; i < 7; ++i)
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 8:
                for(; i < 2; ++i)
                {
                    current->value = current->next->next->next->next->next->next->next->next->value;//左移
                    current = current->next;
                }
                for(i = 0; i < 8; ++i)
                {
                    current->value = NULL;
                    current = current->next;
                }
                break;
            case 9:
                current->value = current->next->next->next->next->next->next->next->next->next->value;//左移
                current = current->next;
                for(i = 0; i < 9; ++i)
                {
                    current->value = NULL;
                    current = current->next;
                }
            default:break;
        }
    }
}

void cd_command(char *address)
{
    setpath(address);
}

void clr_command()
{
    printf("\033[1A\033[2J\033[H");//\033[1A是到上一行 \033[2J是清屏 \033[H是到最顶行
}

void dir_command()
{
    char address[80];
    getcwd(address, sizeof(address));
    ls_command(address);
}

void echo_command(char *content)
{
    printf("%s", content);
}

void environ_command()
{
    char **env = environ;
    while(*env)
    {
        printf("%s\n", *env);
        env++;
    }
}

void ls_command(char *address)//执行ls指令，列出当前路径下的文件
{
    DIR *dir = NULL;
    struct dirent *direntp = NULL;
    dir = opendir(address);
    if(dir != NULL)
    {
        while(true)
        {
            direntp = readdir(dir);
            if(direntp == NULL)//已经没有文件可以供输出了
                break;
            else if(direntp->d_name[0] != '.')
                printf("%s\n", direntp->d_name);
        }
        closedir(dir);
    }

}

void pwd_command()//执行pwd，获取当前的工作目录
{
    char buf[80];
    getcwd(buf, sizeof(buf));
    printf("%s\n", buf);
}

void time_command()
{
    char *dayOfWeek[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    time_t timePointer;
    struct tm *p;
    time(&timePointer);
    p = localtime(&timePointer);
    printf("%d/%d/%d ", (1900+p->tm_year), (p->tm_mon), p->tm_mday);
    printf("%s %d:%d:%d\n", dayOfWeek[p->tm_wday], p->tm_hour, p->tm_min, p->tm_sec);
}

int test_command(struct parseInfo *info)
{
    //字符串测试部分
    if(strcmp(info->parameter[1], "-n") == 0)// test -n string 字符串长度非0
    {
        if(strlen(info->parameter[2]) != 0)
            return 1;//字符串长度不为0，返回1表示表达式为真
        else
            return 0;//否则为假
    }

    if(strcmp(info->parameter[1], "-z") == 0)// test -z string1
    {
        if(strlen(info->parameter[2]) == 0)
            return 1;//字符串长度为0，返回1表示表达式为真
        else
            return 0;//否则为假
    }

    if(strcmp(info->parameter[2], "=") == 0)// test string1 = string2
    {
        if(strcmp(info->parameter[1], info->parameter[3]) == 0)
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else
            return 0;//否则为假
    }

    if(strcmp(info->parameter[2], "!=") == 0)// test string1 != string2
    {
        if(strcmp(info->parameter[1], info->parameter[3]) == 0)
            return 0;//两个字符串完全相同，返回0表示表达式为假
        else
            return 1;//否则为真
    }
    //整数判断部分
    if(strcmp(info->parameter[2], "-eq") == 0)// test int1 -eq int2 ==
    {
        if(strcmp(info->parameter[1], info->parameter[3]) == 0)
            return 1;//两个字符串完全相同，则其值必定相同，返回1表示表达式为真
        else
            return 0;//否则为假
    }
    if(strcmp(info->parameter[2], "-ne") == 0)// test int1 -ne int2 !=
    {
        if(strcmp(info->parameter[1], info->parameter[3]) != 0)
            return 1;//两个字符串不完全相同，则其值必定不同，返回1表示表达式为真
        else
            return 0;//否则为假
    }
    if(strcmp(info->parameter[2], "-ge") == 0)// test int1 -ge int2 >=
    {
        if(atoi(info->parameter[1]) >= atoi(info->parameter[3]))
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else
            return 0;//否则为假
    }
    if(strcmp(info->parameter[2], "-gt") == 0)// test int1 -gt int2 >
    {
        if(atoi(info->parameter[1]) > atoi(info->parameter[3]))
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else
            return 0;//否则为假
    }
    if(strcmp(info->parameter[2], "-le") == 0)// test int1 -le int2 <=
    {
        if(atoi(info->parameter[1]) <= atoi(info->parameter[3]))
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else
            return 0;//否则为假
    }
    if(strcmp(info->parameter[2], "-lt") == 0)// test int1 -lt int2 <
    {
        if(atoi(info->parameter[1]) < atoi(info->parameter[3]))
            return 1;//两个字符串完全相同，返回1表示表达式为真
        else
            return 0;//否则为假
    }
}

int unset_command(struct parseInfo *info)
{
    struct valueNode *current = mylinkList->next;
    struct valueNode *before = mylinkList;
    //因为本人定义的这个链表开始10个结点为内置的参数，所以不可删除，
    //因此不检查最开始的那个结点是否满足删除要求
    while(current != NULL)
    {
        if(strcmp(current->name, info->parameter[1]) == 0)
        {
            if(currentEnd == current)
            {
                currentEnd = before;
            }
            if(current->value != NULL)
                free(current->value);
            before->next = current->next;
            struct valueNode *tmp = current;
            current = current->next;
            free(tmp);
        }
        before = before->next;
        if(current != NULL)
            current = current->next;
    }
}

int normal_cmd(struct parseInfo *info, char* command,int cmdlen,int infd, int outfd, int fork)
{
    return is_internal_cmd(info, info->command);
}
int is_internal_cmd(struct parseInfo *info, char* command)
{
    char *endposition = NULL;
    if((endposition = strstr(info->parameter[0], "=")) != NULL) //当前为赋值操作
    {
        char *newName = malloc(sizeof(char) * (endposition - info->parameter[0] + 1));
        memset(newName, 0, (endposition - info->parameter[0] + 1) *sizeof(char));
        strncpy(newName, info->parameter[0], endposition - info->parameter[0]);
        endposition++;
        char *newValue = malloc(sizeof(char) * strlen(endposition));
        memset(newValue, 0, strlen(endposition) * sizeof(char));
        strcpy(newValue, endposition);
        valueLinkList(0, newName, newValue);
        free(newName);
        free(newValue);
    }
    else if(strcmp(info->parameter[0], "bg") == 0)
    {

    }
    else if(strcmp(info->parameter[0], "cd") == 0)//切换工作目录
    {
        cd_command(info->parameter[1]);
    }
    else if(strcmp(info->parameter[0], "clr") == 0)//清屏
    {
        clr_command();
    }
    else if(strcmp(info->parameter[0], "dir") == 0)//列出当前工作目录下文件
    {
        dir_command();
    }
    else if(strcmp(info->parameter[0], "echo") == 0)//回响
    {
        echo_command(info->parameter[1]);
    }
    else if(strcmp(info->parameter[0], "environ") == 0)//显示环境变量
    {
        environ_command();
    }
    else if(strcmp(info->parameter[0], "exit") == 0)//退出shell
    {
        return INFINITY;
    }
    else if(strcmp(info->parameter[0], "exec") == 0)//将外部程序直接覆盖bash的进程
    {
        execvp(info->parameter[1], NULL);
        return INFINITY;
    }
    else if(strcmp(info->parameter[0], "help") == 0)
    {
        FILE *fp = NULL;
        char c;
        int i = 0;
        char currentHelpAddress[1006] = { '\0' };
        strcpy(currentHelpAddress, currentShellAddress);
        for(i = cnt; i >=0; --i)
            if(currentHelpAddress[i] == '/')
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
        printf("%s", currentHelpAddress);
        fp = fopen(currentHelpAddress, "r");
        if(NULL == fp)
            return -1;
        while(fscanf(fp, "%c", &c) != EOF)
            printf("%c", c);
        fclose(fp);
        fp = NULL;
    }
    else if(strcmp(info->parameter[0], "ls") == 0)//列出指定文件夹下的目录
    {
        ls_command(info->parameter[1]);
    }
    else if(strcmp(info->parameter[0], "pwd") == 0)//显示当前工作目录
    {
        pwd_command();
    }
    else if(strcmp(info->parameter[0], "quit") == 0)//退出shell
    {
        return INFINITY;
    }
    else if(strcmp(info->parameter[0], "shift") == 0)//左移参数
    {
        valueLinkList(atoi(info->parameter[1]), NULL, NULL);
    }
    else if(strcmp(info->parameter[0], "time") == 0)//打印时间
    {
        time_command();
    }
    else if(strcmp(info->parameter[0], "test") == 0)//测试表达式
    {
        int result = 0;
        result = test_command(info);
        printf("test reslut is: %d", result);
    }
    else if(strcmp(info->parameter[0], "umask") == 0)
    {
        if(info->parameter[1] && (strcmp(info->parameter[1], "\0") != 0))//用户处于设置状态
        {
            int mask =0;
            mask = strtol(info->parameter[1], NULL, 8);
            umask((mode_t)mask);
        }
        else
        {
            int mask = 0;
            mask = umask(S_IRUSR | S_IWUSR | S_IXUSR);
            printf("%3.o", mask);
            fflush(stdout);
            umask(mask);
        }
    }
    else if(strcmp(info->parameter[0], "unset") == 0)
    {
        unset_command(info);
    }
    return 1;
}

int readcommand(struct parseInfo *info) //读取并分析指令
{

    char *paramBegin = info->prompt;
    char *paramEnd = info->prompt;
    int paramCount = 0;

    fgets(info->prompt, MAX_LINE, stdin);
    strcpy(info->normalPrompt, info->prompt);
    while(true)
    {
        while(*paramEnd == ' ' || *paramEnd == '\t')//吃掉当前要处理的parameter之前的空格或者tab
        {
            ++paramBegin;
            ++paramEnd;
        }

        if(*paramEnd == '\n' || *paramEnd == '\0')//走到最后发现是换行符，结束处理
        {
            if(paramCount == 0)
                return -1;//-1表示没有任何parameter
            break;
        }

        while(*paramEnd != ' ' && *paramEnd != '\t' && *paramEnd != '\0' && *paramEnd != '\n')
        {//paramEnd目前遇到的都是这项parameter的内容
            ++paramEnd;
        }
        if(paramCount == 0)//如果这是处理的第一个parameter
        {
            info->command = paramBegin;
            info->parameter[0] = paramBegin;
            ++paramCount;
        }
        else if(paramCount <= MAX_NUMBER_OF_ARG)//没有超过最大限度
        {
            info->parameter[paramCount] = paramBegin;
            ++paramCount;
        }
        else//后面的均被忽略
            break;
        if(*paramEnd == '\0' || *paramEnd == '\n')//再次检查paramEnd指针是否已经走到最后
        {
            *paramEnd = '\0';//统一置换成\0以方便处理
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

void parseInfoInitialize(struct parseInfo *info)
{
    info->flag = 0;
    info->numberOfParameter = 0;
    info->inFile = NULL;
    info->outFile = NULL;
    info->command = NULL;
    info->command2 = NULL;
    info->parameter2 = NULL;
}

void parcing(struct parseInfo *info)
{
    int i = 0;
    for(i = 0; i < info->numberOfParameter;)
    {
        char *endPosition = 0;
        if((endPosition = strstr(info->parameter[i], "$")) != NULL)//当前为赋值表达式
        {
            endPosition++;
            struct valueNode *current = mylinkList;
            while(current != NULL)
            {
                if(strcmp(endPosition, current->name) == 0)//找到可以替换的
                {
                    if(current->value == NULL)
                    {
                        printf("This variable has no value!");
                        current = current->next;
                        continue;
                    }
                    info->parameter[i] = malloc(sizeof(char) * strlen(current->value));//可能会导致内存泄漏
                    strcpy(info->parameter[i], current->value);//字符展开
                    break;
                }
                current = current->next;
            }
            if(current == NULL)//没找到可替换的
            {
                info->parameter[i] = malloc(sizeof(char));
                *(info->parameter[i]) = '\0';
            }
        }
        else if(strcmp(info->parameter[i], "<") == 0 || strcmp(info->parameter[i], "<<") == 0)
        //打上重定向输入的记号
        {
            info->flag |= IN_REDIRECT;
            info->inFile = info->parameter[i + 1]; //标识重定向后的信息流来源
            if(strcmp(info->parameter[i - 1], "0"))
                info->stream = STANDARDINPUT;
            info->parameter[i] = NULL;
            i += 2;
        }
        else if(strcmp(info->parameter[i], ">") == 0)
        //打上重定向覆盖输出的记号
        {
            info->flag |= OUT_REDIRECT_OVERWRITE;
            info->outFile = info->parameter[i + 1]; //标识重定向后的信息流来源
            if(strcmp(info->parameter[i - 1], "1")) //如果是标准输出,标记为标准输出
                info->stream = STANDARDINPUT;
            else if(strcmp(info->parameter[i - 1], "2"))
                info->stream = STANDARDOUTPUT;
            info->parameter[i] = NULL;
            i += 2;
        }
        else if(strcmp(info->parameter[i], ">>") == 0)
        //打上重定向覆盖输出的记号
        {
            info->flag |= OUT_REDIRECT_ADDITION;
            info->outFile = info->parameter[i + 1]; //标识重定向后的信息流来源
            if(strcmp(info->parameter[i - 1], "1")) //如果是标准输出,标记为标准输出
                info->stream = STANDARDINPUT;
            else if(strcmp(info->parameter[i - 1], "2"))
                info->stream = STANDARDOUTPUT;
            info->parameter[i] = NULL;
            i += 2;
        }
        else if(strcmp(info->parameter[i], "&") == 0) //打上后台运行的标记
        {
            info->flag |= BACKGROUND;
            info->parameter[i] = NULL;
            ++i;
        }
        else if(strcmp(info->parameter[i], "|") == 0) //管道指令
        {
            info->flag |= IS_PIPE;
            info->command2 = info->parameter[i + 1];    //获取管道后的指令
            info->parameter2 = &(info->parameter[i + 1]);  //获取管道后的指令的第一个参数
            info->parameter[i] = NULL;
            i += 2;
        }
        else if(strcmp(info->parameter[i], "fg") == 0)
        {
            info->flag |= FRONTGROUND;
            info->command = info->parameter[i + 1];
            info->parameter = &(info->parameter[i + 1]);
            --(info->numberOfParameter);
            ++i;
        }
        else if(strcmp(info->parameter[i], "bg") == 0)
        {
            info->flag |= BACKGROUND;
            info->command = info->parameter[i + 1];
            info->parameter = &(info->parameter[i + 1]);
            --(info->numberOfParameter);
            ++i;
        }
        else
            ++i;
    }

}

void printPrompt(char *argv[])
{
    if(argv[1] && strcmp(argv[1], "1") == 0)
        return;
    //printf("\nPlease make sure this input is right, or you can not execute your script:\n");
    //printf("Please enter now where myshell is: %s", currentShellAddress);
    //scanf("%s", currentShellAddress);
    cnt = readlink("/proc/self/exe", currentShellAddress, 1000);
    if(cnt < 0 || cnt >= 1000)
    {
        printf("Open myshell fail!");
        exit(-1);
    }
    //printf("%s", currentShellAddress);
    char buf[80];
    getcwd(buf, sizeof(buf));    //取得当前的工作目录
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
    info->parameter = (char**)malloc(sizeof(char*)*(MAX_NUMBER_OF_ARG + 2));
    //用来存储输入的命令各个parameter存在的地方
    init();//初始化
    parseInfoInitialize(info);//初始化结构体
    printPrompt(argv);
    fflush(stdout);
    while (true)
    {
        int inFd, outFd;

        readcommand(info);//读取指令并进行预处理
        if(-1 == info->numberOfParameter)
            continue;
        parcing(info);//对指令进行语义分析
        if(0 == info->flag)//从指令中什么都没有解析出，那就是普通指令
        {

            if(strcmp(info->parameter[0], "myshell") == 0)
            {
                int scriptPID;
                if((scriptPID = fork()) < 0)
                {
                    perror("脚本进程创建失败！");
                }
                else if(scriptPID == 0) // 脚本子进程
                {
                    int infd = 0;
                    infd = open(info->parameter[1], O_RDONLY | O_CREAT, 0777);
                    dup2(infd, fileno(stdin));
                    char *myargvs[80] = { NULL };
                    myargvs[0] = info->parameter[0];
                    myargvs[1] = "1";
                    execvp(currentShellAddress, myargvs);
                    exit(0);
                }
                else
                {
                    waitpid(scriptPID, NULL, 0);
                    printPrompt(argv);
                    parseInfoInitialize(info);//初始化结构体
                    continue;
                }
            }
            if(normal_cmd(info, info->normalPrompt, strlen(info->normalPrompt), NULL, NULL, NULL)== INFINITY)
            {//当返回值为0时代表输入了一个exit或者quit指令
                break;//退出循环结束bash
            }
            parseInfoInitialize(info);
            if(getpid() != 0)
            {
                char buf[80];
                getcwd(buf, sizeof(buf));    //取得当前的工作目录
                printf("\nmyshell>%s/$ ", buf);
            }
            continue;
        }
        else if(IS_PIPE & info->flag)
        {
            if(pipe(pipe_fd) < 0)//创建管道，为下面新建子进程做准备
            {
                perror("用于指令的管道创建失败！");
            }
        }//else 即为其他重定向、后台运行等操作

        if((childPID = fork()) < 0)
        {
            perror("子进程创建失败！");
        }
        else if(childPID == 0)//子进程
        {
            if(info->flag & IN_REDIRECT)//flag检测到输入重定向
            {
                inFd = open(info->inFile, O_RDONLY, 0777);//以只读方式打开，打开重定向的文件
                close(fileno(stdin));//关闭标准输入
                dup2(inFd, fileno(stdin));//将原先与标准输入对接的接口重定向到inFd上
                close(inFd);//结束操作，关闭inFd
            }

            if(info->flag & IS_PIPE)
            {
                if(info->flag & OUT_REDIRECT_OVERWRITE)
                {
                    close(pipe_fd[0]);
                    close(pipe_fd[1]);
                    outFd = open(info->outFile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    //打开输出文件，打开方式为只写，不存在时创建，覆盖写，创建新文件的时候权限为777
                    close(fileno(stdout));
                    dup2(outFd, fileno(stdout));
                    close(outFd);
                }
                else if(info->flag & OUT_REDIRECT_ADDITION)//重定向为添加写时
                {
                    close(pipe_fd[0]);
                    close(pipe_fd[1]);
                    outFd = open(info->outFile, O_WRONLY | O_CREAT | O_APPEND, 0777);
                    //打开输出文件，打开方式为只写，不存在时创建，添加写，创建新文件的时候权限为777
                    close(fileno(stdout));
                    dup2(outFd, fileno(stdout));
                    close(outFd);
                }
                else//单纯的管道操作
                {
                    close(fileno(stdout));
                    close(pipe_fd[0]);
                    dup2(pipe_fd[1], fileno(stdout)); //将标准输出去掉，改为向管道输入东西
                    close(pipe_fd[1]);
                }

            }
            else // 不存在管道操作
            {
                if(info->flag & OUT_REDIRECT_OVERWRITE) // 重定向输出为覆盖写时
                {
                    outFd = open(info->outFile, O_WRONLY | O_CREAT | O_TRUNC, 0777);
                    //打开输出文件，打开方式为只写，不存在时创建，覆盖写，创建新文件的时候权限为777
                    close(fileno(stdout));
                    dup2(outFd, fileno(stdout));
                    close(outFd);
                }
                else if(info->flag & OUT_REDIRECT_ADDITION)//重定向为添加写时
                {
                    outFd = open(info->outFile, O_WRONLY | O_CREAT | O_APPEND, 0777);
                    //打开输出文件，打开方式为只写，不存在时创建，添加写，创建新文件的时候权限为777
                    close(fileno(stdout));
                    dup2(outFd, fileno(stdout));
                    close(outFd);
                }
            }
            normal_cmd(info, info->command, NULL, NULL, NULL, 1);//设置好全部的重定向管道信息以后执行相关的指令
            exit(0);
        }
        else//父进程
        {

            int pipeChildPID = 0;
            int i = 0;
            if(info->flag & IS_PIPE)//
            {
                if((pipeChildPID = fork()) < 0)
                {
                    perror("主进程管道子进程创建失败");
                }
                else if (pipeChildPID == 0)
                {
                    close(pipe_fd[1]);
                    close(fileno(stdin));
                    //char buf[80] = { '\0' };
                    //read(pipe_fd[0], buf, 80);
                    if(info->flag & BACKGROUND)
                    {
                        char bufin[80];
                        getcwd(bufin, sizeof(bufin));    //取得当前的工作目录
                        printf("myshell>%s", bufin);
                        printf("/$ ");
                        fflush(stdout);
                        //printf("read from pipe: %s\n", buf);
                        //parseInfoInitialize(info);//初始化结构体
                        //continue;
                    }
                    //printf("read from pipe: %s\n", buf);//debug使用
                    dup2(pipe_fd[0], fileno(stdin));

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

                    execvp(info->command2, info->parameter2);
                    exit(0);
                }
                else
                {
                    waitpid(pipeChildPID, &status, 0);
                }
            }
            if ((info->flag & BACKGROUND) && !(info->flag & IS_PIPE))
            {
                printPrompt(argv);
                parseInfoInitialize(info);//初始化结构体
                continue;
            }
            else if ((info->flag & BACKGROUND) && (info->flag & IS_PIPE))
            {
                parseInfoInitialize(info);//初始化结构体
                continue;
            }
            else
            {
                waitpid(childPID, NULL, 0);
                printPrompt(argv);
                parseInfoInitialize(info);//初始化结构体
            }
        }
    }
    free(info->parameter);
    free(info);
    return 0;
}
