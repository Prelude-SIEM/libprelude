typedef struct prelude_optlist prelude_optlist_t;

typedef enum {
        required_argument,
        optionnal_argument,
        no_argument,
} prelude_option_argument_t;


prelude_optlist_t *prelude_option_new(void);

int prelude_option_add(prelude_optlist_t *optlist,
                       char shortopt, const char *longopt, const char *desc,
                       prelude_option_argument_t has_arg, void (*set)(const char *optarg));


int prelude_option_parse_arguments(prelude_optlist_t *optlist,
                                   const char *filename, int argc, const char **argv);


void prelude_option_print(prelude_optlist_t *optlist);


void prelude_option_destroy(prelude_optlist_t *optlist);


/*
 * TODO
 */

#if 0

typedef enum {
        local_option,
        wide_option
} prelude_option_type_t;

#endif
