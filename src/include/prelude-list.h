#include "list.h"

#define PRELUDE_LINKED_OBJECT \
        struct list_head list


typedef struct {
        PRELUDE_LINKED_OBJECT;
} prelude_linked_object_t;



static inline void prelude_list_del(prelude_linked_object_t *obj) 
{
        list_del(&obj->list);
}



static inline void prelude_list_add(prelude_linked_object_t *obj, struct list_head *head) 
{
        list_add(&obj->list, head);
}



static inline void prelude_list_add_tail(prelude_linked_object_t *obj, struct list_head *head) 
{
        list_add_tail(&obj->list, head);
}


#define prelude_list_get_object(listentry, type)  \
        (type *) list_entry(listentry, prelude_linked_object_t, list)

