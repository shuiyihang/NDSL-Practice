#include "bplusTree.h"
#include "bplusTest.h"

void big_data_test()
{
    #define DATA_RANGE  3000
    srand((unsigned int)time(0));
    fprintf(stderr, "\n> big data set testing...\n");

    clock_t begin = clock();

    struct bplus_tree *tree = bplus_tree_init(5);

#ifdef LOG_DEBUG
    FILE* log_txt = fopen("log.txt","w+");
#endif
    for(int i = 0;i < DATA_RANGE; i++){

        kv_t key_val;
        key_val.id = rand()%(DATA_RANGE * 2) + 3;
        key_val.name[0] = rand()%26 + 'a';
        key_val.name[1] = rand()%26 + 'a';
        key_val.name[2] = rand()%26 + 'a';
        key_val.name[3] = '\0';
#ifdef LOG_DEBUG
        fprintf(log_txt,"insert:%d,%s\n",key_val.id,key_val.name);
        bplus_tree_set(tree,key_val);
        bplus_tree_dump(tree,log_txt);
        fprintf(log_txt,"\n\n");
#else

        bplus_tree_set(tree,key_val);
#endif
    }

#ifdef LOG_DEBUG
    fclose(log_txt);
#endif

    clock_t end = clock(); 

    fprintf(stderr, "\n> build b+tree spend in %.5f secs\n",(double)(end - begin) / CLOCKS_PER_SEC);
    // 打印树结构
    // bplus_tree_dump(tree,NULL);
    // print_list(tree);


    // 序列化
    fprintf(stderr, "\n> serialize testing...\n");
    FILE* fp = fopen(FILENAME,"w+");
    serialize(tree,fp);
    fclose(fp);
    bplus_tree_free(tree);
    

    // 反序列化还原一棵树
    fprintf(stderr, "\n> deserialize testing...\n");
    begin = clock();
    fp = fopen(FILENAME,"r");
    tree = deserialize(fp);
    fclose(fp);
    end = clock();
    fprintf(stderr, "\n> deserialize build b+tree spend in %.5f secs\n",(double)(end - begin) / CLOCKS_PER_SEC);

    bplus_tree_dump(tree,NULL);
    // print_list(tree);

    bplus_tree_free(tree);
}

void base_operation_test()
{
    fprintf(stderr, "\n> B+tree set testing...\n");

    struct bplus_tree *tree = bplus_tree_init(4);
    bplus_tree_set(tree,(kv_t){.id = 22,.name = "man"});
    bplus_tree_set(tree,(kv_t){.id = 30,.name = "ls"});

    bplus_tree_set(tree,(kv_t){.id = 23,.name = "cat"});
    bplus_tree_set(tree,(kv_t){.id = 24,.name = "list"});
    bplus_tree_set(tree,(kv_t){.id = 10,.name = "san"});
    bplus_tree_set(tree,(kv_t){.id = 16,.name = "le"});
    bplus_tree_set(tree,(kv_t){.id = 21,.name = "due"});
    bplus_tree_set(tree,(kv_t){.id = 32,.name = "find"});
    bplus_tree_set(tree,(kv_t){.id = 18,.name = "ls"});


    // 验证插入溢出
    bplus_tree_set(tree,(kv_t){.id = 19,.name = "man"});
    bplus_tree_set(tree,(kv_t){.id = 25,.name = "ls"}); // mark

    bplus_tree_set(tree,(kv_t){.id = 26,.name = "tree"});
    bplus_tree_set(tree,(kv_t){.id = 27,.name = "map"});

    bplus_tree_set(tree,(kv_t){.id = 28,.name = "mkdir"});

    // 更多数据
    bplus_tree_set(tree,(kv_t){.id = 13,.name = "man"});
    bplus_tree_set(tree,(kv_t){.id = 20,.name = "ls"});

    bplus_tree_set(tree,(kv_t){.id = 33,.name = "where"});
    bplus_tree_set(tree,(kv_t){.id = 35,.name = "ls"});

    bplus_tree_set(tree,(kv_t){.id = 38,.name = "which"});// 19个


    // 打印树结构
    bplus_tree_dump(tree,NULL);
    print_list(tree);


    
    // 验证查询键值对
    fprintf(stderr, "\n> B+tree get testing...\n");
    bplus_tree_get(tree, 28);
    bplus_tree_get(tree, 19);

    // 删除测试
    fprintf(stderr, "\n> B+tree delete testing...\n");
    bplus_tree_delete(tree,26);
    bplus_tree_delete(tree,27);
    bplus_tree_delete(tree,20);
    // bplus_tree_delete(tree,23);
    // bplus_tree_delete(tree,18);
    // bplus_tree_delete(tree,21);
    // bplus_tree_delete(tree,10);
    // bplus_tree_delete(tree,28);
    // bplus_tree_delete(tree,38);

    // 打印树结构
    bplus_tree_dump(tree,NULL);
    print_list(tree);


    // bplus_tree_delete(tree,30);
    // bplus_tree_delete(tree,22);
    // bplus_tree_delete(tree,24);
    // bplus_tree_delete(tree,19);
    // bplus_tree_delete(tree,16);
    // bplus_tree_delete(tree,33);
    // bplus_tree_delete(tree,35);
    // bplus_tree_delete(tree,25);


    fprintf(stderr, "\n> B+tree repeat set testing...\n");

    bplus_tree_get(tree,13);
    bplus_tree_set_force(tree,(kv_t){.id = 13,.name = "make"});
    bplus_tree_get(tree,13);

    bplus_tree_free(tree);
}

void cmd_interface_test(int argc,char* argv[])
{
    struct bplus_tree *tree = bplus_tree_init(5);
    FILE* fp = fopen(FILENAME,"r");
    if(fp) tree = deserialize(fp);
    fclose(fp);

    extern int cmd_interface(struct bplus_tree *tree,int argc,char* argv[]);

    cmd_interface(tree,argc,argv);

    fp = fopen(FILENAME,"w+");
    serialize(tree,fp);
    fclose(fp);
    bplus_tree_free(tree);

}


int main(int argc,char* argv[])
{
    // do_func(big_data_test);
    // do_func(base_operation_test);
    cmd_interface_test(argc,argv);
    return 0;
}