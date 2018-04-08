#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <asm/ptrace-abi.h>   /* For constants ORIG_EAX etc */
#include <sys/user.h>   /* For user_regs_struct etc. */
#include <sys/syscall.h>

#if 0
int main()
{   pid_t child;
    long orig_eax;
    child = fork();
    if(child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl("/bin/ls", "ls", NULL);
    }
    else {
        wait(NULL);
        orig_eax = ptrace(PTRACE_PEEKUSER,
                          child, 4 * ORIG_EAX,
                          NULL);
        printf("The child made a "
               "system call %ld\n", orig_eax);
        ptrace(PTRACE_CONT, child, NULL, NULL);
    }
    return 0;
}
#endif

#if 0
/*追踪某个进程pc指针的位置和指令内容*/
int main(int argc, char *argv[])
{   
	pid_t traced_process;
    struct user_regs_struct regs;
    long ins;

    if(argc != 2) {
        printf("Usage: %s <pid to be traced>\n", argv[0]);
        exit(1);
    }

    traced_process = atoi(argv[1]);
    ptrace(PTRACE_ATTACH, traced_process,  NULL, NULL);
    wait(NULL);
    ptrace(PTRACE_GETREGS, traced_process, NULL, &regs);   /*获取整组寄存器配置*/
    ins = ptrace(PTRACE_PEEKTEXT, traced_process, regs.eip, NULL);  /*获取pc指针里的指令内容*/
    printf("EIP: %lx Instruction executed: %lx\n", regs.eip, ins);
    ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
    return 0;
}
#endif

const int long_size = sizeof(long);
void getdata(pid_t child, long addr, char *str, int len)
{   char *laddr;
    int i, j;
    union u {
            long val;
            char chars[long_size];
    }data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while(i < j) {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * 4, NULL);
        memcpy(laddr, data.chars, long_size);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if(j != 0) {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * 4, NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}
void putdata(pid_t child, long addr,
             char *str, int len)
{   char *laddr;
    int i, j;
    union u {
            long val;
            char chars[long_size];
    }data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while(i < j) {
        memcpy(data.chars, laddr, long_size);
        ptrace(PTRACE_POKEDATA, child, addr + i * 4, data.val);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if(j != 0) {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, child, addr + i * 4, data.val);
    }
}

long freespaceaddr(pid_t pid)
{
    FILE *fp;
    char filename[30];
    char line[85];
    long addr,addr1;
    char str[20];
    sprintf(filename, "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if(fp == NULL)
        exit(1);
    while(fgets(line, 85, fp) != NULL) {
        sscanf(line, "%lx-%*lx %*s %*s %s", &addr, str, str, str, str);
        if(strcmp(str, "00:00") == 0)
            break;
    }
    fclose(fp);
	addr = 0xb7dce000;
	printf("addr = %x\n",addr);
    return addr;
}
#if 0
/*设置断点*/
int main(int argc, char *argv[])
{   
	pid_t traced_process;
    struct user_regs_struct regs, newregs;
    long ins;
    /* int 0x80, int3 */
    char code[] = {0xcd,0x80,0xcc,0};
    char backup[4];

    if(argc != 2) {
        printf("Usage: %s <pid to be traced>\n", argv[0]);
        exit(1);
    }

    traced_process = atoi(argv[1]);
    ptrace(PTRACE_ATTACH, traced_process, NULL, NULL);
    wait(NULL);
    ptrace(PTRACE_GETREGS, traced_process, NULL, &regs);
  
    getdata(traced_process, regs.eip, backup, 3); /*保存原先的三个指令*/

    putdata(traced_process, regs.eip, code, 3); /*插入断点指令*/

    ptrace(PTRACE_CONT, traced_process, NULL, NULL);  /*程序继续运行，触发断点指令*/
    wait(NULL);
    printf("The process stopped, putting back the original instructions\n");
    printf("Press <enter> to continue\n");
    getchar();
 
    putdata(traced_process, regs.eip, backup, 3); /*替换会原来的指令*/
    ptrace(PTRACE_SETREGS, traced_process, NULL, &regs); /*设置pc到原来的位置重新运行*/
    ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
    return 0;
}

/*运行特定代码*/
int main(int argc, char *argv[])
{   
	pid_t traced_process;
    struct user_regs_struct oldregs, regs;
    long ins;
    int len = 41;
    char insertcode[] =
"\xeb\x15\x5e\xb8\x04\x00"
        "\x00\x00\xbb\x02\x00\x00\x00\x89\xf1\xba"
        "\x0c\x00\x00\x00\xcd\x80\xcc\xe8\xe6\xff"
        "\xff\xff\x48\x65\x6c\x6c\x6f\x20\x57\x6f"
        "\x72\x6c\x64\x0a\x00";
    char backup[len];
    long addr;
    if(argc != 2) {
        printf("Usage: %s <pid to be traced>\n", argv[0]);
        exit(1);
    }
    traced_process = atoi(argv[1]);
    ptrace(PTRACE_ATTACH, traced_process, NULL, NULL);
    wait(NULL);
    ptrace(PTRACE_GETREGS, traced_process, NULL, &regs);
    addr = freespaceaddr(traced_process);
    getdata(traced_process, addr, backup, len);
    putdata(traced_process, addr, insertcode, len);
    memcpy(&oldregs, &regs, sizeof(regs));
    regs.eip = addr;
    ptrace(PTRACE_SETREGS, traced_process, NULL, &regs);
    ptrace(PTRACE_CONT, traced_process, NULL, NULL);
    wait(NULL);
    printf("The process stopped, Putting back the original instructions\n");
    putdata(traced_process, addr, backup, len);
    ptrace(PTRACE_SETREGS, traced_process, NULL, &oldregs);
    printf("Letting it continue with original flow\n");
    ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
    return 0;
}
#endif

/*单步执行*/
int main(int argc, char *argv[])
{   
	pid_t traced_process;
	const int long_size = sizeof(long);
    
	if(argc != 2) {
        printf("Usage: %s <pid to be traced>\n", argv[0]);
        exit(1);
    }
    traced_process = atoi(argv[1]);
    ptrace(PTRACE_ATTACH, traced_process, NULL, NULL);
	wait(NULL);
    ptrace(PTRACE_SINGLESTEP, traced_process, NULL, NULL);

    {
        int status;
        union u {
            long val;
            char chars[long_size];
        }data;
        struct user_regs_struct regs;
        int start = 0;
        long ins;
        while(1) {
            wait(&status);
            if(WIFEXITED(status))
                break;
            ptrace(PTRACE_GETREGS, traced_process, NULL, &regs);
//            if(start == 1) 
			{
                ins = ptrace(PTRACE_PEEKTEXT, traced_process, regs.eip, NULL);
                printf("EIP: %lx Instruction executed: %lx\n", regs.eip, ins);
                ptrace(PTRACE_SINGLESTEP, traced_process, NULL, NULL);
            }
/*
            if(regs.orig_eax == SYS_write) {
                start = 1;
                ptrace(PTRACE_SINGLESTEP, traced_process, NULL, NULL);
            }
            else
                ptrace(PTRACE_SYSCALL, traced_process, NULL, NULL);
*/
        }
    }
    ptrace(PTRACE_DETACH, traced_process, NULL, NULL);
    return 0;
}