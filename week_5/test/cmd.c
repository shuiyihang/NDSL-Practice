#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <malloc.h>
#include "bplusTree.h"

#define CMD_NUM     4
typedef int (*CMDFUNC)(struct bplus_tree *tree,char* argv[]);

const char *gHost = "mykey>> ";
static char *line_read = NULL;


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


int set_key(struct bplus_tree *tree,char* argv[])
{
    kv_t tmp;
    tmp.id = str_2_int(argv[1]);
    memcpy(tmp.name,argv[2],strlen(argv[2])+1);
    bplus_tree_set(tree,tmp);
    return 0;
}

int get_key(struct bplus_tree *tree,char* argv[])
{
    bplus_tree_get(tree,str_2_int(argv[1]));
    return 0;
}

int del_key(struct bplus_tree *tree,char* argv[])
{
    bplus_tree_delete(tree,str_2_int(argv[1]));
    return 0;
}

int exit_key(struct bplus_tree *tree,char* argv[])
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


int execute(struct bplus_tree *tree,char* word[3])
{
    int idx = 0;
    int str_len = strlen(word[0]);
    int ret = -1;

    while (idx < CMD_NUM)
    {
        if(!strncmp(cmdStr[idx],word[0],str_len))
        {
            ret = cmdfunc[idx](tree,word);
        }
        idx++;
    }
    if(ret == -1)
    {
        printf("%s\n",helpEdit);
    }
    return ret;
}

int cmd_interface(struct bplus_tree *tree,int argc,char* argv[])
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
            if(execute(tree,word) == 2)
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
                set_key(tree,packet);
            }
            break;
            
            case 'q':
            {
                char* packet[3];
                packet[0] = "get";
                packet[1] = optarg;
                get_key(tree,packet);
            }
            break;

            case 'd':
            {
                char* packet[3];
                packet[0] = "del";
                packet[1] = optarg;
                del_key(tree,packet);
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