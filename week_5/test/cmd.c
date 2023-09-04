#include <stdio.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>
#include <malloc.h>
#include "bplusTree.h"
#include <sys/time.h>

#define CMD_NUM     5
typedef int (*CMDFUNC)(struct bplus_tree *tree,char* argv[]);

const char *gHost = "mykey>> ";
static char *line_read = NULL;


#define TEST_TIME

char helpCmd[] = 
{
    "Help:\n" \
    "-i keyA valA : insert\n" \
    "-q keyA : query\n" \
    "-d keyA : delete\n" \
    "-p : print tree\n"\
};

char helpEdit[] = 
{
    "Help:\n" \
    "set keyA valA\n" \
    "get keyA\n" \
    "del keyA\n" \
    "ptree\n"\
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

#ifdef TEST_TIME
    struct timeval begin,end;
    gettimeofday(&begin,NULL);
    bplus_tree_set_force(tree,tmp);
    gettimeofday(&end,NULL);
    double time_use = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)/1000000.0;
    fprintf(stderr, "\n> set key spend in %.8f secs\n",time_use);
#elif
    bplus_tree_set_force(tree,tmp);
#endif
    return 0;
}

int get_key(struct bplus_tree *tree,char* argv[])
{
#ifdef TEST_TIME
    struct timeval begin,end;
    gettimeofday(&begin,NULL);
    bplus_tree_get(tree,str_2_int(argv[1]));
    gettimeofday(&end,NULL);
    double time_use = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)/1000000.0;
    fprintf(stderr, "\n> get key spend in %.8f secs\n",time_use);
#elif
    bplus_tree_get(tree,str_2_int(argv[1]));
#endif
    return 0;
}

int del_key(struct bplus_tree *tree,char* argv[])
{
#ifdef TEST_TIME
    struct timeval begin,end;
    gettimeofday(&begin,NULL);
    bplus_tree_delete(tree,str_2_int(argv[1]));
    gettimeofday(&end,NULL);
    double time_use = (end.tv_sec - begin.tv_sec) + (end.tv_usec - begin.tv_usec)/1000000.0;
    fprintf(stderr, "\n> del key spend in %.8f secs\n",time_use);
#elif
    bplus_tree_delete(tree,str_2_int(argv[1]));
#endif
    return 0;
}

int exit_key(struct bplus_tree *tree,char* argv[])
{

    return 2;
}

int print_tree(struct bplus_tree *tree,char* argv[])
{
    bplus_tree_dump(tree,NULL);
    return 0;
}

CMDFUNC  cmdfunc[CMD_NUM] = {
    set_key,
    get_key,
    del_key,
    exit_key,
    print_tree,
};

char *cmdStr[CMD_NUM + 1] = 
{
    "set",
    "get",
    "del",
    "exit",
    "ptree",
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

            case 'p':
            {
                // 便于调试删除节点，查看树结构
                print_tree(tree,NULL);
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