typedef void (*prelude_async_func_t)(void *object, void *data);



#define PRELUDE_ASYNC_OBJECT                   \
        PRELUDE_LINKED_OBJECT;                 \
        void *data;                            \
        prelude_async_func_t func


typedef struct {
        PRELUDE_ASYNC_OBJECT;
} prelude_async_object_t;



static inline void prelude_async_set_data(prelude_async_object_t *obj, void *data) 
{
        obj->data = data;
}


static inline void prelude_async_set_callback(prelude_async_object_t *obj, prelude_async_func_t func) 
{
        obj->func = func;
}

int prelude_async_init(void);

void prelude_async_add(prelude_async_object_t *obj);

void prelude_async_del(prelude_async_object_t *obj);










