
#include "bplusTree.h"
/**
 * 保留每个节点中最小值
 * 非叶子节点元素个数为孩子节点数 - 1
*/

#define false   0
#define true    1

static int parent_node_build(struct bplus_tree *tree,struct bplus_node *left,struct bplus_node *right,int key);

static size_t align_size(size_t size)
{
    // 8字节对齐
    if(size & (sizeof(long) - 1)){
        size += sizeof(long) - (size&(sizeof(long) - 1));
    }
    return size;
}
struct bplus_tree *bplus_tree_init(int order)
{
    struct bplus_tree *tree = calloc(1,sizeof(struct bplus_tree));
    if(tree){
        order = order > (WORD_LEN-1)?(WORD_LEN-1):order;
        tree->order = order;
        tree->root = NULL;
        tree->item_size = sizeof(kv_t);
        list_init(&tree->head);
    }
    return tree;
}

static struct bplus_leaf *leaf_new(int order)
{
    size_t items_offset = sizeof(struct bplus_leaf);
    size_t size = sizeof(kv_t) * order + items_offset;
    struct bplus_leaf *node = calloc(1,align_size(size));
    list_init(&node->link);
    node->type = BPLUS_LEAF;
    node->parent_key_idx = -1;
    node->parent = NULL;
    node->items = (kv_t *)((char*)node + items_offset);
    return node;
}

static struct bplus_non_leaf *non_leaf_new(void)
{

    size_t size = sizeof(struct bplus_non_leaf);

    struct bplus_non_leaf *node = calloc(1,align_size(size));
    node->type = BPLUS_NON_LEAF;
    node->parent_key_idx = -1;
    return node;
}

static void *get_item_at(struct bplus_tree *tree,struct bplus_leaf *node,size_t idx)
{
    return node->items + tree->item_size*idx;
}

static void set_item_at(struct bplus_tree *tree,struct bplus_leaf *node,size_t idx,kv_t item)
{
    void *addr = get_item_at(tree,node,0);
    memcpy(addr,&item,tree->item_size);
}

static int binary_search(int *key,int len,int target,int *found)
{
    // 在节点中二分查找元素
    int low = 0;
    int high = len - 1;
    while (low <= high)
    {
        int mid = (low + high)/2;
    
        if(key[mid] == target){
            *found = 1;
            return mid+1;// key中存储的是右节点的最小值
        }else if(key[mid] < target){
            low = mid + 1;
        }else{
            high = mid - 1;
        }
    }

    *found = 0;
    return low;
}

static int kv_bsearch(kv_t *items,int len,int target,int *found)
{
    // 在节点中二分查找元素
    int low = 0;
    int high = len - 1;
    while (low <= high)
    {
        int mid = (low + high)/2;

        if(items[mid].id == target){
            *found = 1;
            return mid;
        }else if(items[mid].id < target){
            low = mid + 1;
        }else{
            high = mid - 1;
        }
    }

    *found = 0;
    return low;
}

static int leaf_simple_insert(struct bplus_tree *tree,struct bplus_leaf *leaf,kv_t item,int pos)
{
    memmove(leaf->items + (pos + 1),
            leaf->items + (pos),
            tree->item_size*(leaf->kv_nums - pos));
    leaf->items[pos] = item;
    leaf->kv_nums++;
    return 0;
}

static void leaf_split_left(struct bplus_tree *tree,struct bplus_leaf *leaf,struct bplus_leaf *left,kv_t item,int pos)
{
    int split = (tree->order + 1)/2;
    list_add_front(&left->link,&leaf->link);
    // 填充左节点元素
    int i = 0,j = 0;
    for(;i < split;i++)
    {
        if(i == pos){
            left->items[i] = item;
        }else{
            left->items[i] = leaf->items[j];
            j++;
        }
    }
    left->kv_nums = i;

    // 原来的节点元素左移
    for(i=0;j<leaf->kv_nums;j++,i++)
    {
        leaf->items[i] = leaf->items[j];
    }
    leaf->kv_nums = i;
}

static void leaf_split_right(struct bplus_tree *tree,struct bplus_leaf *leaf,struct bplus_leaf *right,kv_t item,int pos)
{
    int split = (tree->order + 1)/2;
    list_add_back(&right->link,&leaf->link);
    int i=0,j;
    for(i=split,j=0;i < leaf->kv_nums;j++)
    {
        if(j == pos - split){
            right->items[j] = item;
        }else{
            right->items[j] = leaf->items[i];
            i++;
        }
    }
    if(j > pos - split){// 说明插入位置在新节点内部
        right->kv_nums = j;
    }else{
        right->kv_nums = j+1;
        right->items[j] = item;
    }
    leaf->kv_nums = split;

}

static int non_leaf_simple_insert(struct bplus_non_leaf *node,struct bplus_node *l_ch,struct bplus_node *r_ch,int key,int insert)
{
    for(int i = node->elem_nums - 1; i>insert;i--)// 非叶子节点中实际个数比记录的少一个
    {
        node->key[i] = node->key[i-1];
        node->ptr[i+1] = node->ptr[i];
        node->ptr[i+1]->parent_key_idx = i;
    }
    node->key[insert] = key;
    node->ptr[insert] = l_ch;
    node->ptr[insert]->parent_key_idx = insert - 1;
    node->ptr[insert+1] = r_ch;
    node->ptr[insert+1]->parent_key_idx = insert;
    
    node->elem_nums++;

    return 0;
}

static int non_leaf_split_left(struct bplus_non_leaf *node,
                        struct bplus_non_leaf *left,
                        struct bplus_node *l_ch,struct bplus_node *r_ch,int key,int insert,int split)
{
    int split_key;
    int i,j;

    // 更改指针
    for(i=0,j=0;j < split +1;i++,j++){// split+1个分支节点
        if(j == insert){
            left->ptr[j] = l_ch;
            left->ptr[j]->parent = left;
            left->ptr[j]->parent_key_idx = j - 1;

            left->ptr[j+1] = r_ch;
            left->ptr[j+1]->parent = left;
            left->ptr[j+1]->parent_key_idx = j;
            j++;
        }else{
            left->ptr[j] = node->ptr[i];
            left->ptr[j]->parent = left;
            left->ptr[j]->parent_key_idx = j -1;
        }
    }
    left->elem_nums = split + 1;

    // 填充值
    for(i = 0,j = 0;i<split;i++){
        if(i == insert){
            left->key[i] = key;
        }else{
            left->key[i] = node->key[j];
            j++;
        }
    }

    node->ptr[0] = node->ptr[split];
    split_key = node->key[split - 1];
    node->ptr[0]->parent = node;
    node->ptr[0]->parent_key_idx = -1;

    for(i = split,j=0;i<node->elem_nums - 1;i++,j++){
        node->key[j] = node->key[i];
        node->ptr[j + 1] = node->ptr[i+1];
        node->ptr[j+1]->parent = node;
        node->ptr[j+1]->parent_key_idx = j;
    }
    node->elem_nums = j+1;
    return split_key;
}

static int non_leaf_split_right(struct bplus_non_leaf *node,
                        struct bplus_non_leaf *right,
                        struct bplus_node *l_ch,struct bplus_node *r_ch,int key,int insert,int split)
{
    // 填充右节点
    int split_key;

    split_key = node->key[split];// idx = split的右指针下标是split+1,这个后继作为右边节点的第一个指针指向的节点。

    right->ptr[0] = node->ptr[split+1];
    right->ptr[0]->parent = right;
    right->ptr[0]->parent_key_idx = -1;

    int i,j;
    // node中剩余的右边元素下标从split+1开始到(nums-1)-1
    for(i=split+1,j=0;i<node->elem_nums - 1;j++){

        // -1 不包括split_key
        if(j != insert - split -1){
            right->key[j] = node->key[i];
            right->ptr[j+1] = node->ptr[i+1];
            right->ptr[j+1]->parent = right;
            right->ptr[j+1]->parent_key_idx = j;
            i++;
        }

    }

    // 上面的循环结束后留有空位
    if(j > insert - split -1){
        right->elem_nums = j + 1;// 非叶子节点记录比实际多一个
    }else{
        // 插入位置在最右边
        right->elem_nums = j + 2;
    }

    j = insert -split -1;

    right->key[j] = key;
    right->ptr[j] = l_ch;
    right->ptr[j]->parent = right;
    right->ptr[j]->parent_key_idx = j -1;
    right->ptr[j+1] = r_ch;
    right->ptr[j+1]->parent = right;
    right->ptr[j+1]->parent_key_idx = j;
    node->elem_nums = split + 1;
    return split_key;
}

static int non_leaf_split_right2(struct bplus_non_leaf *node,
                        struct bplus_non_leaf *right,
                        struct bplus_node *l_ch,struct bplus_node *r_ch,int key,int insert,int split)
{
    // insert = split的情况
    int split_key = key;
    node->ptr[split] = l_ch;
    node->ptr[split]->parent = node;
    node->ptr[split]->parent_key_idx = split - 1;

    // 填充right
    right->ptr[0] = r_ch;
    right->ptr[0]->parent = right;
    right->ptr[0]->parent_key_idx = -1;

    int i,j;
    for(i=split,j=0;i<node->elem_nums - 1;j++,i++){
        right->key[j] = node->key[i];
        right->ptr[j+1] = node->ptr[i+1];
        right->ptr[j+1]->parent = right;
        right->ptr[j+1]->parent_key_idx = j;
    }
    right->elem_nums = j + 1;
    node->elem_nums = split + 1;
    // printf("==%d===\n",right->elem_nums);
    return split_key;
}

static int non_leaf_insert(struct bplus_tree *tree,struct bplus_non_leaf *node,struct bplus_node *l_ch,struct bplus_node *r_ch,int key)
{
    int found;
    int insert = binary_search(node->key,node->elem_nums - 1,key,&found);
    if(node->elem_nums == tree->order){
        int split = node->elem_nums/2;// elem_num比实际记录个数大一
        int split_key;// 分裂产生的key
        struct bplus_non_leaf *sibling = non_leaf_new();
        if(insert < split){
            split_key = non_leaf_split_left(node,sibling,l_ch,r_ch,key,insert,split);
        }else if(insert > split){
            // printf("==%d,%d===%d\n",split,insert,__LINE__);
            split_key = non_leaf_split_right(node,sibling,l_ch,r_ch,key,insert,split);
            // for (size_t i = 0; i < sibling->elem_nums - 1; i++)
            // {
            //     printf("==%d===\n",sibling->key[i]);
            // }
        }else{
            // printf("==%d,%d===\n",node->elem_nums,insert);
            split_key = non_leaf_split_right2(node,sibling,l_ch,r_ch,key,insert,split);
            // for (size_t i = 0; i < sibling->elem_nums - 1; i++)
            // {
            //     printf("==%d===\n",sibling->key[i]);
            // }
            
        }

        if(insert < split){
            return parent_node_build(tree,(struct bplus_node *)sibling,(struct bplus_node *)node,split_key);
        }else{
            return parent_node_build(tree,(struct bplus_node *)node,(struct bplus_node *)sibling,split_key);
        }
    }else{
        return non_leaf_simple_insert(node,l_ch,r_ch,key,insert);
    }
}

static int parent_node_build(struct bplus_tree *tree,struct bplus_node *left,struct bplus_node *right,int key)
{
    if(!left->parent && !right->parent){
        // 
        struct bplus_non_leaf *parent = non_leaf_new();
        parent->key[0] = key;
        parent->ptr[0] = left;
        parent->ptr[1] = right;
        left->parent = right->parent = parent;
        left->parent_key_idx = -1;
        right->parent_key_idx = 0;
        parent->elem_nums = 2;// 虽然实际只记录了一个，但是还是按照两个来计算溢出的
        tree->root = (struct bplus_node *)parent;
        return 0;
    }else if(!left->parent){
        // 左节点为新产生的
        left->parent = right->parent;
        return non_leaf_insert(tree,left->parent,left,right,key);
    }else{
        right->parent = left->parent;
        return non_leaf_insert(tree,right->parent,left,right,key);
    }
}

static int leaf_insert(struct bplus_tree *tree,struct bplus_leaf *leaf,kv_t item,int force)
{
    int found;
    int pos = kv_bsearch(leaf->items,leaf->kv_nums,item.id,&found);
    if(found){
        if(force){
            leaf->items[pos] = item;
            return 0;
        }else{
#ifdef _TRACE_DEBUG
            printf("kv has exist!\n");
#endif
            return -1;// 已经存在
        }
    }
    if(leaf->kv_nums == tree->order){
        // 分裂
        int split = (tree->order + 1)/2;
        struct bplus_leaf *sibling = leaf_new(tree->order);

        if(pos < split){
            // 产生的新节点放在左边
            leaf_split_left(tree,leaf,sibling,item,pos);
        }else{
            // 产生的新节点放在右边
            leaf_split_right(tree,leaf,sibling,item,pos);
        }
        // 产生父节点

        if(pos < split){
            // 保存leaf里的最小的
            return parent_node_build(tree,(struct bplus_node *)sibling,(struct bplus_node *)leaf,leaf->items[0].id);
        }else{
            return parent_node_build(tree,(struct bplus_node *)leaf,(struct bplus_node *)sibling,sibling->items[0].id);
        }
    }else{
        // printf("====sinsert:%d,pos:%d\n",item.id,pos);
        return leaf_simple_insert(tree,leaf,item,pos);
    }
}

static int __item_set(struct bplus_tree *tree, kv_t item,int force)
{
    struct bplus_node *node = tree->root;

    if(!node){
        struct bplus_leaf *root = leaf_new(tree->order);
        set_item_at(tree,root,0,item);
        root->kv_nums = 1;
        tree->root = (struct bplus_node *)root;
        list_add_back(&root->link,&tree->head);
        return 0;
    }
    // 根节点不空寻找可插入的叶节点
    while(node != NULL){
        if(is_leaf(node)){
            return leaf_insert(tree,(struct bplus_leaf *)node,item,force);
        }else{
            struct bplus_non_leaf *bnl = (struct bplus_non_leaf *)node;

            int found;
            int pos = binary_search(bnl->key,bnl->elem_nums - 1,item.id,&found);
            // printf("===pos:%d,item:%d\n",pos,item.id);
            node = bnl->ptr[pos];
        }
    }
    return -1;

}

int bplus_tree_set(struct bplus_tree *tree, kv_t item)
{
    return __item_set(tree,item,false);
}

int bplus_tree_set_force(struct bplus_tree *tree, kv_t item)
{
    return __item_set(tree,item,true);
}


static int __kv_get(struct bplus_tree *tree, kv_t *item)
{
    struct bplus_node *node = tree->root;
    if(!node){
        return false;
    }
    while (node != NULL)
    {
        if(is_leaf(node)){
            struct bplus_leaf *leaf = (struct bplus_leaf *)node;
            int found;
            int pos = kv_bsearch(leaf->items,leaf->kv_nums,item->id,&found);
            if(found){
                // item->name
                memcpy(item,&leaf->items[pos],tree->item_size);
            #ifdef _TRACE_DEBUG
                printf("===parent_idx:%d===\n",node->parent_key_idx);
            #endif
                return true;
            }else{
                return false;
            }
        }else{
            int found;
            struct bplus_non_leaf *bnl = (struct bplus_non_leaf *)node;

            int pos = binary_search(bnl->key,bnl->elem_nums - 1,item->id,&found);
            node = bnl->ptr[pos];
        }
    }
    return false;
}

void bplus_tree_get(struct bplus_tree *tree, int key)
{
    kv_t tmp;
    tmp.id = key;
    if(__kv_get(tree,&tmp))
    {
        fprintf(stderr, "(%d,%s)\n",tmp.id,tmp.name);
    }else{
        fprintf(stderr, "key not exist\n");
    }
}

static void leaf_simple_remove(struct bplus_leaf *leaf,int remove)
{
    memmove(leaf->items+remove,leaf->items+remove+1,(leaf->kv_nums - remove -1)*sizeof(kv_t));
    leaf->kv_nums--;
}

static int leaf_sibling_select(struct bplus_non_leaf *parent,struct bplus_leaf * l_sbl,struct bplus_leaf *r_sbl,int idx)
{
    if(idx == -1)return SIBLING_RIGHT;
    if(idx == parent->elem_nums - 2)return SIBLING_LEFT;
    return l_sbl->kv_nums >= r_sbl->kv_nums ? SIBLING_LEFT:SIBLING_RIGHT;
}



static void leaf_borrow_from_left(struct bplus_leaf *leaf,struct bplus_leaf *l_sbl,int parent_idx,int remove)
{
    memmove(leaf->items+1,leaf->items,remove*sizeof(kv_t));
    leaf->items[0] = l_sbl->items[l_sbl->kv_nums - 1];
    l_sbl->kv_nums--;
    leaf->parent->key[parent_idx] = leaf->items[0].id;
}

static void leaf_borrow_from_right(struct bplus_leaf *leaf,struct bplus_leaf *r_sbl,int parent_idx)
{
    leaf->items[leaf->kv_nums] = r_sbl->items[0];
    leaf->kv_nums++;
    memmove(r_sbl->items,r_sbl->items+1,sizeof(kv_t)*(r_sbl->kv_nums - 1));
    r_sbl->kv_nums--;
    r_sbl->parent->key[parent_idx] = r_sbl->items[0].id;
}

static void leaf_merge_into_left(struct bplus_leaf *leaf,struct bplus_leaf *left,int remove)
{
    int i,j;
    for(i = left->kv_nums,j=0;j < leaf->kv_nums;j++)
    {
        if(j != remove){
            left->items[i] = leaf->items[j];
            i++;
        }
    }
    left->kv_nums = i;

    list_delet(&leaf->link);
    free(leaf);
}

static void leaf_merge_from_right(struct bplus_leaf *leaf,struct bplus_leaf *right)
{
    memmove(leaf->items+leaf->kv_nums,right->items,right->kv_nums*sizeof(kv_t));
    leaf->kv_nums += right->kv_nums;

    list_delet(&right->link);// 确保叶子节点依旧连接
    free(right);
}

static void non_leaf_simple_remove(struct bplus_non_leaf *non_leaf,int remove)
{
    int i;
    for(i=remove;i<non_leaf->elem_nums - 2;i++){
        non_leaf->key[i] = non_leaf->key[i+1];
        non_leaf->ptr[i+1] = non_leaf->ptr[i+2];
        non_leaf->ptr[i+1]->parent_key_idx = i;
    }
    non_leaf->elem_nums--;
}

static int non_leaf_sibling_select(struct bplus_non_leaf *parent,int idx)
{
    struct bplus_non_leaf *l_sbl,*r_sbl;
    if(idx == -1)return SIBLING_RIGHT;
    if(idx == parent->elem_nums - 2)return SIBLING_LEFT;
#ifdef _TRACE_DEBUG
    printf("========%d=======%d\n",__LINE__,idx);
#endif
    l_sbl = (struct bplus_non_leaf *)parent->ptr[idx];
    r_sbl = (struct bplus_non_leaf *)parent->ptr[idx+2];
    return l_sbl->elem_nums >= r_sbl->elem_nums ? SIBLING_LEFT:SIBLING_RIGHT;
}

static void non_leaf_borrow_from_left(struct bplus_non_leaf *l_sbl,struct bplus_non_leaf *node,int remove,int parent_idx)
{
    memmove(node->key+1,node->key,remove*sizeof(node->key[0]));
    for(int i = remove + 1;i>0;i--){
        node->ptr[i] = node->ptr[i-1];
        node->ptr[i]->parent_key_idx = i-1;
    }
    // 父节点key下移,兄弟节点key上移
    node->key[0] = node->parent->key[parent_idx];
    node->parent->key[parent_idx] = l_sbl->key[l_sbl->elem_nums - 2];

    node->ptr[0] = l_sbl->ptr[l_sbl->elem_nums - 1];
    node->ptr[0]->parent = node;
    node->ptr[0]->parent_key_idx = -1;

    l_sbl->elem_nums--;
}

static void non_leaf_borrow_from_right(struct bplus_non_leaf *r_sbl,struct bplus_non_leaf *node,int parent_idx)
{
    node->key[node->elem_nums - 1] = node->parent->key[parent_idx];// parent_idx为右节点的idx
    node->parent->key[parent_idx] = r_sbl->key[0];// 兄弟节点key上移

    node->ptr[node->elem_nums] = r_sbl->ptr[0];
    node->ptr[node->elem_nums]->parent = node;
    node->ptr[node->elem_nums]->parent_key_idx = node->elem_nums - 1;
    node->elem_nums++;

    memmove(r_sbl->key,r_sbl->key+1,sizeof(r_sbl->key[0])*(r_sbl->elem_nums - 2));

    for(int i = 0;i<r_sbl->elem_nums-1;i++){// 对应key上移，指针全部左移
        r_sbl->ptr[i] = r_sbl->ptr[i+1];
        r_sbl->ptr[i]->parent_key_idx = i-1;
    }
    r_sbl->elem_nums--;
}

static void non_leaf_merge_into_left(struct bplus_non_leaf *l_sbl,struct bplus_non_leaf *node,int remove,int parent_idx)
{
    int i,j;
    // key值为左节点 + 父节点 + 当前节点
    l_sbl->key[l_sbl->elem_nums - 1] = node->parent->key[parent_idx];

    for(i = l_sbl->elem_nums,j=0;j<node->elem_nums-1;j++){
        if(remove != j){
            l_sbl->key[i] = node->key[j];
            i++;
        }
    }

    for(i = l_sbl->elem_nums,j=0;j<node->elem_nums;j++){
        if(j != remove + 1){
            l_sbl->ptr[i] = node->ptr[j];
            l_sbl->ptr[i]->parent = l_sbl;
            l_sbl->ptr[i]->parent_key_idx = i - 1;
            i++;
        }
    }
    l_sbl->elem_nums = i;
    free(node);
}

static void non_leaf_merge_from_right(struct bplus_non_leaf *r_sbl,struct bplus_non_leaf *node,int parent_idx)
{
    int i,j;
    node->key[node->elem_nums - 1] = node->parent->key[parent_idx];

    memmove(node->key+node->elem_nums,r_sbl->key,(r_sbl->elem_nums - 1)*sizeof(node->key[0]));// 全部节点合并

    for(i=node->elem_nums,j=0;j<r_sbl->elem_nums;i++,j++){
        node->ptr[i] = r_sbl->ptr[j];
        node->ptr[i]->parent = node;
        node->ptr[i]->parent_key_idx = i-1;
    }
    node->elem_nums = i;
    free(r_sbl);
}

static void non_leaf_remove(struct bplus_tree *tree,struct bplus_non_leaf *non_leaf,int remove)
{
    if(non_leaf->elem_nums <= (tree->order + 1)/2){
        struct bplus_non_leaf *parent = non_leaf->parent;
        if(parent != NULL){
            int i = non_leaf->parent_key_idx;
            if(non_leaf_sibling_select(parent,i) == SIBLING_LEFT){
                struct bplus_non_leaf *l_sbl = (struct bplus_non_leaf *)parent->ptr[i];
                // assert(l_sbl);
                if(l_sbl->elem_nums > (tree->order + 1)/2){
                    // 借用
                    non_leaf_borrow_from_left(l_sbl,non_leaf,remove,i);
                }else{
                    // 合并
                #ifdef _TRACE_DEBUG
                    printf("========%d=======\n",__LINE__);
                #endif
                    non_leaf_merge_into_left(l_sbl,non_leaf,remove,i);
                    non_leaf_remove(tree,parent,i);
                }
            }else{
                struct bplus_non_leaf *r_sbl = (struct bplus_non_leaf *)parent->ptr[i+2];
                non_leaf_simple_remove(non_leaf,remove);
                if(r_sbl->elem_nums > (tree->order + 1)/2){
                    // 借用
                #ifdef _TRACE_DEBUG
                    printf("========%d,%d=======\n",__LINE__,non_leaf->elem_nums);
                #endif
                    non_leaf_borrow_from_right(r_sbl,non_leaf,i+1);
                }else{
                    // 合并
                #ifdef _TRACE_DEBUG
                    printf("========%d=======\n",__LINE__);
                #endif
                    non_leaf_merge_from_right(r_sbl,non_leaf,i+1);
                    non_leaf_remove(tree,parent,i+1);
                }
            }

        }else{
        #ifdef _TRACE_DEBUG
            printf("========%d=======\n",__LINE__);
        #endif
            if(non_leaf->elem_nums == 2){// 实际只有一个记录时
                non_leaf->ptr[0]->parent = NULL;// 记录的就只有第二个节点起的索引，此时右节点已经释放
                tree->root = non_leaf->ptr[0];
                free(non_leaf);
            }else{
                non_leaf_simple_remove(non_leaf,remove);
            }
        }
    }else{
    #ifdef _TRACE_DEBUG
        printf("========%d=====%d==\n",__LINE__,remove);
    #endif
        non_leaf_simple_remove(non_leaf,remove);
    }
}

static void non_leaf_update_index(struct bplus_leaf *leaf)
{
    struct bplus_non_leaf *parent = leaf->parent;
    if(parent == NULL){
        return;
    }

    if(leaf->parent_key_idx != -1){
        parent->key[leaf->parent_key_idx] = leaf->items[0].id;
    }else{
        while(parent->parent != NULL && parent->parent_key_idx == -1){
            parent = parent->parent;
        }
        if(parent->parent) parent->parent->key[parent->parent_key_idx] = leaf->items[0].id;
    }
}

static int leaf_remove(struct bplus_tree *tree,struct bplus_leaf *leaf,int key)
{
    int found;
    int remove = kv_bsearch(leaf->items,leaf->kv_nums,key,&found);

    // printf("===del:%d,found:%d==\n",remove,found);
    if(found == false){
        return -1;
    }
    // printf("===del:%d==\n",remove);
    // m/2 向上取整
    if(leaf->kv_nums <= (tree->order + 1)/2){
        // 从左右兄弟借或者合并左右兄弟
        struct bplus_non_leaf *parent = leaf->parent;
        struct bplus_leaf *l_sbl = list_entry(leaf->link.prev,struct bplus_leaf,link);
        struct bplus_leaf *r_sbl = list_entry(leaf->link.next,struct bplus_leaf,link);
        if(parent != NULL){
            int i = leaf->parent_key_idx;
            if(leaf_sibling_select(parent,l_sbl,r_sbl,i) == SIBLING_LEFT){
                if(l_sbl->kv_nums > (tree->order + 1)/2){
                    // 借用: 
                #ifdef _TRACE_DEBUG
                    printf("========%d=======\n",__LINE__);
                #endif
                    leaf_borrow_from_left(leaf,l_sbl,i,remove);
                }else{
                    // 合并
                #ifdef _TRACE_DEBUG
                    printf("========%d=======\n",__LINE__);
                #endif
                    leaf_merge_into_left(leaf,l_sbl,remove);// 只删除了节点，并没有改变父节点的指针指向
                    non_leaf_remove(tree,parent,i);// 删除父节点中下标i的key
                }
            }else{
                leaf_simple_remove(leaf,remove);
                if(r_sbl->kv_nums > (tree->order + 1)/2){
                    // 借用
                    leaf_borrow_from_right(leaf,r_sbl,i+1);
                }else{
                    // 把右节点中的元素向左边合并
                    leaf_merge_from_right(leaf,r_sbl);
                #ifdef _TRACE_DEBUG
                    printf("========%d,%d=======\n",__LINE__,i);
                #endif
                    non_leaf_remove(tree,parent,i+1);
                }
            }
            
        }else{
        #ifdef _TRACE_DEBUG
            printf("========%d=======\n",__LINE__);
        #endif
            if(leaf->kv_nums == 1){
                tree->root = NULL;
                free(leaf);
            }else{
                leaf_simple_remove(leaf,remove);
            }
        }
    }else{
    #ifdef _TRACE_DEBUG
        printf("========%d=======\n",__LINE__);
    #endif
        leaf_simple_remove(leaf,remove);
    }
    // 需要向上查找更新索引值
    if(remove == 0){
        non_leaf_update_index(leaf);
    }
    return 0;
}
int bplus_tree_delete(struct bplus_tree *tree, int key)
{
    struct bplus_node *node = tree->root;
    while(node != NULL){
        if(is_leaf(node)){
            
            struct bplus_leaf *leaf = (struct bplus_leaf *)node;
            return leaf_remove(tree,leaf,key);
        }else
        {
            struct bplus_non_leaf *non_leaf = (struct bplus_non_leaf *)node;
            int found;
            int remove = binary_search(non_leaf->key,non_leaf->elem_nums - 1,key,&found);
            node = non_leaf->ptr[remove];
            // printf("===rm:%d,item:%d\n",remove,key);
        }
        
    }
    return -1;
}



static void key_print(struct bplus_node *node,FILE *fp)
{
    int i;
    if (is_leaf(node)) {
            struct bplus_leaf *leaf = (struct bplus_leaf *)node;
            fprintf(fp,"leaf:");
            for (i = 0; i < leaf->kv_nums; i++) {
                fprintf(fp," %d",leaf->items[i].id);
            }
    } else {
            struct bplus_non_leaf *non_leaf = (struct bplus_non_leaf *)node;
            fprintf(fp,"node:");
            for (i = 0; i < non_leaf->elem_nums - 1; i++) {
                fprintf(fp," %d",non_leaf->key[i]);
            }
    }
    fprintf(fp,"\n");
}
void bplus_tree_dump(struct bplus_tree *tree,FILE *fp)
{
    if(!fp)fp = stderr;
    struct node_backlog {
        struct bplus_node *node;
        int next_sub_idx;
    };
    int level = 0;
    struct bplus_node *node = tree->root;
    struct node_backlog *p_nbl = NULL;
    struct node_backlog nbl_stack[BPLUS_MAX_LEVEL];
    struct node_backlog *top = nbl_stack;

    for (; ;) {
            if (node != NULL) {
                    int sub_idx = p_nbl != NULL ? p_nbl->next_sub_idx : 0;
                    p_nbl = NULL;

                    if (is_leaf(node) || sub_idx + 1 >= children(node)) {
                            top->node = NULL;
                            top->next_sub_idx = 0;
                    } else {
                            top->node = node;
                            top->next_sub_idx = sub_idx + 1;
                    }
                    top++;
                    level++;

                    if (sub_idx == 0) {
                            int i;
                            for (i = 1; i < level; i++) {
                                    if (i == level - 1) {
                                            fprintf(fp,"%-8s","+-------");
                                    } else {
                                            if (nbl_stack[i - 1].node != NULL) {
                                                    fprintf(fp,"%-8s","|");
                                            } else {
                                                    fprintf(fp,"%-8s"," ");
                                            }
                                    }
                            }
                            key_print(node,fp);
                    }

                    node = is_leaf(node) ? NULL : ((struct bplus_non_leaf *) node)->ptr[sub_idx];
            } else {
                    p_nbl = top == nbl_stack ? NULL : --top;
                    if (p_nbl == NULL) {
                            break;
                    }
                    node = p_nbl->node;
                    level--;
            }
    }
}

static void node_free(struct bplus_node *node)
{
    if(!is_leaf(node)){
        struct bplus_non_leaf *non_leaf = (struct bplus_non_leaf *)node;
        for(int i=0;i<non_leaf->elem_nums;i++){
            node_free(non_leaf->ptr[i]);
        }
        free(non_leaf);
    }else{
        struct bplus_leaf *leaf = (struct bplus_leaf *)node;
        free(leaf);
    }
    
}
void bplus_tree_free(struct bplus_tree *tree)
{
    if(tree)
    {
        if(tree->root){
            node_free(tree->root);
        }
        free(tree);
    }
}


void print_list(struct bplus_tree *tree)
{
    struct bplus_leaf *leaf;
    link_dlist node = tree->head.next;
    
    fprintf(stderr, "\n> B+tree sequence print...\n");
    
    int num = 0;
    while (node != &tree->head)
    {
        leaf = list_entry(node,struct bplus_leaf,link);
        for(int i=0; i<leaf->kv_nums;i++)
        {
            printf("%d ",leaf->items[i].id);
            num++;
        }
        node = node->next;
    }
    
    printf("total:%d\n\n",num);
}


int str_2_int(char *str)
{
    int len = strlen(str);
    int sum = 0;
    int i = 0;
    int elem;
    char flag = 0;
    while (i < len)
    {
        if(*(str+i) < '0' || *(str+i) > '9'){
            if(*(str+i) == '-'){
                flag = 1;
                i++;
                continue;
            }else{
                sum = 0;
                break;
            }
        }else{
            elem = *(str+i) - '0';
            sum *= 10;
            sum += elem;
        }
        i++;
    }
    return flag?-sum:sum;
}

void serialize(struct bplus_tree *tree,FILE *fp)
{
    // 层次遍历序列化
    if(!tree)return;
    if(!fp) fp = stderr;
    struct bplus_node *node = tree->root;
    linkQueue queue;

    fprintf(fp,"%d\n",tree->order);// 第一行记录阶数

    queue_init(&queue);// 带头结点的队列
    queue_push(&queue,&node->q_node);
    while (!queue_isEmpty(&queue))
    {
        node = list_entry(queue_pop(&queue),struct bplus_node,q_node);
        if(node->type == BPLUS_LEAF){
            struct bplus_leaf *leaf = (struct bplus_leaf *)node;
            fprintf(fp,"0,");// BPLUS_LEAF
            int i;
            for(i = 0;i < leaf->kv_nums - 1;i++){
                fprintf(fp,"%d:%s,",leaf->items[i].id,leaf->items[i].name);
            }
            fprintf(fp,"%d:%s\n",leaf->items[i].id,leaf->items[i].name);
        }else{
            struct bplus_non_leaf *non_leaf = (struct bplus_non_leaf *)node;
            fprintf(fp,"1,");
            int i;
            for(i = 0;i < non_leaf->elem_nums - 2;i++){
                fprintf(fp,"%d,",non_leaf->key[i]);
            }
            fprintf(fp,"%d\n",non_leaf->key[i]);
            for(int i = 0;i < non_leaf->elem_nums;i++){
                queue_push(&queue,&non_leaf->ptr[i]->q_node);
            }
        }
    }
    free(queue.front);
    
}


int analy_next_line(FILE *fp,char *word[WORD_LEN], char buff[STLEN])
{
    char *tmp;
    
    char* remain;

    int idx = 0;
    if(fgets(buff,STLEN,fp) != NULL)// 未到文件尾部
    {
        // fix:键值有空格有bug
        buff[strlen(buff) - 1] = '\0';// 把换行符替换结束符
        tmp = buff;
        while ((idx < WORD_LEN) && (word[idx] = strtok_r(tmp,",",&remain)) != NULL)
        {
            tmp = NULL;
            idx++;
        }
    }
    // printf("==%s==%s==%s==\n",word[1],word[2],word[3]);
    return idx;
}

void str_2_items(char *str,kv_t *item)
{
    char num[10] = {0};
    char name[MAX_NAME_LEN] = {0};
    char *ch = str;

    int other = 0;
    int i = 0,j = 0;

    while(*ch != '\0'){
        if(*ch == ':'){
            other = 1;
            ch++;
            continue;
        }

        if(other == 0){
            num[i++] = *ch;
        }else{
            name[j++] = *ch;
        }
        ch++;
    }
    num[i] = '\0';
    name[j] = '\0';
    item->id = str_2_int(num);
    strcpy(item->name,name);

    // printf("%d,%s\n",item->id,item->name);
}

struct bplus_tree *deserialize(FILE *fp)
{
    // 反序列化
    struct bplus_tree *tree;
    char *word[WORD_LEN];
    char line_buff[STLEN];// 用来缓存读到的每一行

    int result = analy_next_line(fp,word,line_buff);// 第一行为阶数
    int order = str_2_int(word[0]);
    tree = bplus_tree_init(order);

    linkQueue queue;
    queue_init(&queue);

    result = analy_next_line(fp,word,line_buff);// 读头节点
    // printf("%s==%s==%s==%s==\n",word[0],word[1],word[2],word[3]);

    // 处理头节点
    if(!strncmp(word[0],"0",1)){
        // 叶子
        // printf("======%d=====\n",__LINE__);
        struct bplus_leaf *leaf = leaf_new(tree->order);
        leaf->type = BPLUS_LEAF;
        for(int i = 0;i < result - 1;i++){
            str_2_items(word[i+1],&leaf->items[i]);
        }
        leaf->kv_nums = result - 1;
        tree->root = (struct bplus_node *)leaf;
    }else{
        struct bplus_non_leaf *non_leaf = non_leaf_new();
        non_leaf->type = BPLUS_NON_LEAF;
        non_leaf->elem_nums = result; // 真实元素个数 + 1
        non_leaf->parent = NULL;
        non_leaf->parent_key_idx = -1;
        for(int i = 0;i < non_leaf->elem_nums - 1;i++){
            non_leaf->key[i] = str_2_int(word[i+1]);
        }
        tree->root = (struct bplus_node *)non_leaf;
        queue_push(&queue,&non_leaf->q_node);
    }

    // 剩余节点
    while(!queue_isEmpty(&queue)){
        struct bplus_non_leaf *parent = list_entry(queue_pop(&queue),struct bplus_non_leaf,q_node);
        for(int i = 0;i < parent->elem_nums;i++){
            result = analy_next_line(fp,word,line_buff);
            if(!strncmp(word[0],"0",1)){
                // 叶子节点
                struct bplus_leaf *leaf = leaf_new(tree->order);
                leaf->type = BPLUS_LEAF;
                for(int i = 0;i < result - 1;i++){
                    str_2_items(word[i+1],&leaf->items[i]);// 填充键值对
                }
                leaf->kv_nums = result - 1;
                leaf->parent = parent;
                leaf->parent_key_idx = i - 1;
                parent->ptr[i] = (struct bplus_node *)leaf;
                // 插入叶子节点的链表里面
                list_insert_tail(&leaf->link,&tree->head);
            }else{
                // 非叶子节点
                struct bplus_non_leaf *non_leaf = non_leaf_new();
                non_leaf->type = BPLUS_NON_LEAF;
                non_leaf->elem_nums = result; // 真实元素个数 + 1
                non_leaf->parent = parent;
                non_leaf->parent_key_idx = i - 1;
                for(int i = 0;i < non_leaf->elem_nums - 1;i++){
                    non_leaf->key[i] = str_2_int(word[i+1]);
                }
                parent->ptr[i] = (struct bplus_node *)non_leaf;
                queue_push(&queue,&non_leaf->q_node);
            }
        }
    }
    free(queue.front);
    return tree;
}