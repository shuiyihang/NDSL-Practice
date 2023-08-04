#include "bplusTree.h"
int main()
{
    fprintf(stderr, "\n> B+tree set testing...\n");

    struct bplus_tree *tree = bplus_tree_init(5);
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
    bplus_tree_dump(tree);
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
    bplus_tree_delete(tree,23);
    bplus_tree_delete(tree,18);
    bplus_tree_delete(tree,21);
    bplus_tree_delete(tree,10);
    bplus_tree_delete(tree,28);
    bplus_tree_delete(tree,38);

    // 打印树结构
    bplus_tree_dump(tree);
    print_list(tree);


    bplus_tree_delete(tree,30);
    bplus_tree_delete(tree,22);
    bplus_tree_delete(tree,24);
    bplus_tree_delete(tree,19);
    bplus_tree_delete(tree,16);
    bplus_tree_delete(tree,33);
    bplus_tree_delete(tree,35);
    bplus_tree_delete(tree,25);


    fprintf(stderr, "\n> B+tree repeat set testing...\n");

    bplus_tree_get(tree,13);
    bplus_tree_set_force(tree,(kv_t){.id = 13,.name = "make"});
    bplus_tree_get(tree,13);

    // 全部删除
    bplus_tree_delete(tree,13);
    bplus_tree_delete(tree,32);

    bplus_tree_get(tree,31);

    bplus_tree_free(tree);
    return 0;
}