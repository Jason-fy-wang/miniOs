#include "list.h"
#include "interrupt.h"

// 链表初始化
void list_init(struct list* plist){
    plist->head.prev = NULL;
    plist->head.next = &plist->tail;
    plist->tail.prev = &plist->head;
    plist->tail.next = NULL;
}

// 把链表元素elem插入到 元素before之前
void list_insert_before(struct list_elem* before, struct list_elem *elem){
    enum intr_status old_status = intr_disable();

    before->prev->next = elem;
    elem->prev = before->prev;
    elem->next = before;
    before->prev = elem;

    intr_set_status(old_status);
}

// 添加元素到列表头
void list_push(struct list* plist, struct list_elem *elem){
    list_insert_before(plist->head.next, elem);
}

// 添加元素到队尾
void list_append(struct list* plist, struct list_elem *elem){
    list_insert_before(&plist->tail, elem);
}

// 删除元素 pelem
void list_remove(struct list_elem* pelem){
    enum intr_status old_status = intr_disable();

    pelem->prev->next = pelem->next;
    pelem->next->prev = pelem->prev;

    intr_set_status(old_status);
}

//将链表第一个元素弹出
struct list_elem* list_pop(struct list* plist){
    struct list_elem *elem = plist->head.next;
    list_remove(elem);
    return elem;
}

bool elem_find(struct list* plist, struct list_elem* obj_elem){
    struct list_elem* elem = plist->head.next;
    while(elem != &plist->tail){
        if(elem == obj_elem){
            return true;
        }
        elem = elem->next;
    }
    return false;
}

/*********
 * 把列表plist中的每个元素elem 和arg 传给回调函数func
 * arg给func用来判断elem是否符合条件
 * 本函数功能是遍历列表内所有元素,逐个判断是否有符合条件的元素
 * 找到符合条件的元素返回元素指针, 否则返回NULL
 */
struct list_elem* list_traversal(struct list* plist, function func, int arg){
    struct list_elem* elem = plist->head.next;
    if(list_empty(plist)){
        return NULL;
    }

    while(elem != &plist->tail){
        if(func(elem,arg)){
            return elem;
        }
        elem = elem->next;
    }
    return NULL;
}



void list_iterate(struct list* plist){

}


bool list_empty(struct list* plist){
    return (plist->head.next == &plist->tail ? true:false);
}

uint32_t list_len(struct list* plist){
    struct list_elem *elem = plist->head.next;
    uint32_t length = 0;
    while(elem != &plist->tail){
        length++;
        elem = elem->next;
    }
    return length;
}


