
typedef struct prelude_ident prelude_ident_t;

uint64_t prelude_ident_inc(prelude_ident_t *ident);

uint64_t prelude_ident_dec(prelude_ident_t *ident);

void prelude_ident_destroy(prelude_ident_t *ident);

prelude_ident_t *prelude_ident_new(const char *filename);
