#ifndef _LINKEDLISTH_
#define _LINKEDLISTH_

typedef struct linkedlistnode_t linkedlistnode_t;
struct linkedlistnode_t {
  linkedlistnode_t *next, *prev;
  size_t hash, memory;
  void *value;
};

typedef struct linkedlist_t linkedlist_t;
struct linkedlist_t {
  size_t size, memory;
  void (*free_member)(void*);
  linkedlistnode_t *head;
  linkedlistnode_t *tail;
};

linkedlist_t * linkedlist_init(void (*free_member)(void*));
void * linkedlist_add_head(linkedlist_t * list, size_t hash, size_t memory, void * value);
void * linkedlist_get(linkedlist_t * list, size_t hash);
void * linkedlist_get_tail(linkedlist_t * list);
void linkedlist_remove(linkedlist_t * list, linkedlistnode_t * node);
int linkedlist_size(linkedlist_t * list);
int linkedlist_memory(linkedlist_t * list);
void linkedlist_free(linkedlist_t * list);
 
void linkedlist_move_head(linkedlist_t * list, linkedlistnode_t * node);
void linkedlist_remove_tail(linkedlist_t * list);
void linkedlist_visit(linkedlist_t * list, void (*visitor)(void*));

#endif

