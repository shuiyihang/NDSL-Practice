#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <malloc.h>

#define CMD_NUM     4
typedef int (*CMDFUNC)(char* argv[]);

const char *gHost = "mysky>> ";
static char *line_read = NULL;


#define     FILENAME    "key.txt"
#define     WORD_LEN    3


char helpCmd[] = 
{
    "Help:\n" \
    "-i keyA valA\n" \
    "-q keyA\n" \
    "-d keyA\n" \
};

char helpEdit[] = 
{
    "Help:\n" \
    "set keyA valA\n" \
    "get keyA\n" \
    "del keyA\n" \
};

char* rl_gets(void)
{
    if(line_read)
    {
        free(line_read);
        line_read = NULL;
    }
    line_read = readline(gHost);

    if(line_read && *line_read)
    {
        add_history(line_read);
    }
    return line_read;
}


int set_key(char* argv[])
{
    char buff[1024];
    char *word[WORD_LEN];
    int idx = 0;
    char *tmp;
    char flag = 0;

    if(argv[1] == NULL || argv[2] == NULL)
    {
        return -1;
    }

    FILE* fp = fopen(FILENAME,"a+");
    if(fp == NULL)
    {
        return -2;
    }
    fseek(fp,0L,SEEK_SET);

    char* remain;
    while (fgets(buff,1024,fp) != NULL)
    {
        tmp = buff;
        while ((idx < WORD_LEN) && (word[idx] = strtok_r(tmp," ",&remain)) != NULL)
        {
            tmp = NULL;
            idx++;
        }
        if(!strncmp(argv[1],word[0],strlen(argv[1])))
        {
            flag = 1;
            break;
        }
    }

    if(flag == 1)
    {
        printf("%s has exist,please delete first!\n",argv[1]);
    }else
    {
        fprintf(fp,"%s %s\n",argv[1],argv[2]);
    }
    fclose(fp);
    return 0;
}

int get_key(char* argv[])
{
    char buff[1024];
    char *word[WORD_LEN];
    int idx = 0;
    char *tmp;
    char flag = 0;

    if(argv[1] == NULL)
    {
        return -1;
    }

    FILE* fp = fopen(FILENAME,"r");
    if(fp == NULL)
    {
        return -2;
    }

    char* remain;
    while (fgets(buff,1024,fp) != NULL)
    {
        tmp = buff;
        while ((idx < WORD_LEN) && (word[idx] = strtok_r(tmp," ",&remain)) != NULL)
        {
            tmp = NULL;
            idx++;
        }
        if(!strncmp(argv[1],word[0],strlen(argv[1])))
        {
            printf("%s:%s",word[0],word[1]);
            
            flag = 1;
        }
    }
    
    if(flag == 0)
    {
        printf("%s not exist!\n",argv[1]);
    }
    fclose(fp);
    return 0;
}

int del_key(char* argv[])
{
    char buff[1024] = {0};
    char save[1024] = {0};
    char *word[WORD_LEN];
    int idx = 0;
    char *tmp;

     if(argv[1] == NULL)
    {
        return -1;
    }

    FILE* fp = fopen(FILENAME,"r");
    FILE* fp_2 = fopen("tmp.txt","w");
    if(fp == NULL || fp_2 == NULL)
    {
        return -2;
    }

    char* remain;
    char flag = 0;

    while (fgets(buff,1024,fp) != NULL)
    {
        memcpy(save,buff,strlen(buff));
        tmp = buff;
        while ((idx < WORD_LEN) && (word[idx] = strtok_r(tmp," ",&remain)) != NULL)
        {
            tmp = NULL;
            idx++;
        }
        if(!strncmp(argv[1],word[0],strlen(argv[1])))
        {
            flag = 1;
            continue;
        }else
        {
            
            fprintf(fp_2,"%s",save);
        }
    }

    if(flag == 0)
    {
        printf("%s not exist!\n",argv[1]);
    }
    
    fclose(fp);
    fclose(fp_2);
    remove(FILENAME);
    rename("tmp.txt",FILENAME);
    return 0;
}

int exit_key(char* argv[])
{

    return 2;
}

CMDFUNC  cmdfunc[CMD_NUM] = {
    set_key,
    get_key,
    del_key,
    exit_key,
};

char *cmdStr[CMD_NUM + 1] = 
{
    "set",
    "get",
    "del",
    "exit"
};


int execute(char* word[3])
{
    int idx = 0;
    int str_len = strlen(word[0]);
    int ret = -1;

    while (idx < CMD_NUM)
    {
        if(!strncmp(cmdStr[idx],word[0],str_len))
        {
            ret = cmdfunc[idx](word);
        }
        idx++;
    }
    if(ret == -1)
    {
        printf("%s\n",helpEdit);
    }
    return ret;
}

int main(int argc,char* argv[])
{

    if(argc < 2)
    {
        // 进入交互界面
        while (1)
        {
            char *line = rl_gets();
            char *remain;
            char *word[WORD_LEN];
            int idx = 0;

            while ((idx < WORD_LEN) && (word[idx] = strtok_r(line," ",&remain)) != NULL)
            {
                line = NULL;
                idx++;
            }
            #if 0
            for(int i=0;i<idx;i++)
            {
                printf("%s\n",word[i]);
            }
            #endif
            if(execute(word) == 2)
            {
                break;
            }
        }
        

    }else
    {
        extern char *optarg;
        extern int optind; // argv 的当前索引值
        // 命令行
        int opt;
        char flag = 0;
        while ((opt = getopt(argc,argv,"q:d:i:")) != -1)
        {
            flag = 1;
            switch (opt)
            {
            case 'i':
            {
                char* packet[3];
                packet[0] = "set";
                packet[1] = optarg;
                packet[2] = argv[optind];
                set_key(packet);
            }
            break;
            
            case 'q':
            {
                char* packet[3];
                packet[0] = "get";
                packet[1] = optarg;
                get_key(packet);
            }
            break;

            case 'd':
            {
                char* packet[3];
                packet[0] = "del";
                packet[1] = optarg;
                del_key(packet);
            }
            break;

            default:
                flag = 0;
                break;
            }
        }

        if(flag == 0)
        {
            printf("%s",helpCmd);
        }
        
    }

    return 0;
}