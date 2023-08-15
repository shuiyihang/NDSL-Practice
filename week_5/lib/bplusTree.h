#ifndef BPLUS_TREE_C
#define BPLUS_TREE_C

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define MAX_NAME_LEN   10

#define BPLUS_MAX_ORDER   32
#define BPLUS_MAX_LEVEL   20


#define STLEN   1024
#define WORD_LEN    31// 1 + max order
// #define _TRACE_DEBUG

#define list_entry(ptr, type, member) \
        ((type *)((char *)(ptr) - (size_t)(&((type *)0)->member)))


typedef struct keyVal
{
    int id;
    char name[MAX_NAME_LEN];
}kv_t;


typedef struct node
{
    struct node *prev,*next;
} link_node,*link_dlist;



static inline void list_init(link_dlist list)
{
    list->prev = list->next = list;
}

static inline void list_delet(link_dlist node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    list_init(node);
}

static inline void __list_add(link_dlist node,link_dlist prev,link_dlist next)
{
    node->next = next;
    node->prev = prev;
    next->prev = node;
    prev->next = node;
}

static inline void list_add_front(link_dlist node,link_dlist obj)
{
    __list_add(node,obj->prev,obj);
}
// 插到obj节点后面
static inline void list_add_back(link_dlist node,link_dlist obj)
{
    __list_add(node,obj,obj->next);
}

// 尾插

static inline void list_insert_tail(link_dlist node,link_dlist obj)
{
    link_dlist tail = obj->prev;
    list_add_back(node,tail);
}

// 链队列
typedef struct{
    struct node *front,*rear;
}linkQueue;

void queue_init(linkQueue *q)
{
    q->front = q->rear = (struct node *)malloc(sizeof(struct node));
    q->front->next = NULL;
}

int queue_isEmpty(linkQueue *q)
{
    return q->front == q->rear;
}

void queue_push(linkQueue *q,struct node *elem)
{
    q->rear->next = elem;
    q->rear = elem;
}

struct node* queue_pop(linkQueue *q)
{
    if(q->rear == q->front)return NULL;
    struct node* tmp = q->front->next;
    q->front->next = tmp->next;
    if(tmp == q->rear)q->rear = q->front;

    return tmp;
}

enum{
    BPLUS_LEAF,
    BPLUS_NON_LEAF,
};

enum{
    SIBLING_LEFT,
    SIBLING_RIGHT,
};


struct bplus_node
{
    // 节点类型
    int type;

    // 节点关键字个数
    int nums;

    // 指向父节点
    struct bplus_non_leaf *parent;

    // 父节点中第几个关键字的索引节点
    int parent_key_idx;

    struct node q_node;// 序列化时候的入队节点
};


struct bplus_non_leaf
{
    // 节点类型
    int type;
    // 节点关键字个数
    int elem_nums;

    // 指向父节点
    struct bplus_non_leaf *parent;

    // 父节点中第几个关键字的索引节点
    int parent_key_idx;

    struct node q_node;// 序列化时候的入队节点

    int key[BPLUS_MAX_ORDER-1];

    struct bplus_node *ptr[BPLUS_MAX_ORDER];
};

struct bplus_leaf
{
    // 节点类型
    int type;
    // 节点键值对个数
    int kv_nums;

    // 指向父节点
    struct bplus_non_leaf *parent;

    // 父节点中第几个关键字的索引节点
    int parent_key_idx;

    struct node q_node;// 序列化时候的入队节点

    struct node link;
    // 节点包含信息
    kv_t *items;
};

struct bplus_tree
{
    // 阶数
    int order;
    size_t item_size;
    struct bplus_node *root;

    // 顺序连接叶子节点
    link_node head; 
};

static inline int children(struct bplus_node *node)
{
    return ((struct bplus_non_leaf *) node)->elem_nums;
}

static inline int is_leaf(struct bplus_node *node)
{
    return node->type == BPLUS_LEAF;
}

struct bplus_tree *bplus_tree_init(int order);
int bplus_tree_set(struct bplus_tree *tree, kv_t item);
int bplus_tree_set_force(struct bplus_tree *tree, kv_t item);// 存在替换
void bplus_tree_dump(struct bplus_tree *tree);
void bplus_tree_get(struct bplus_tree *tree, int key);
void bplus_tree_free(struct bplus_tree *tree);

int bplus_tree_delete(struct bplus_tree *tree, int key);

void print_list(struct bplus_tree *tree);

void serialize(struct bplus_tree *tree,FILE *fp);
struct bplus_tree *deserialize(FILE *fp);
#endif