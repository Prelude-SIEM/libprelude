#ifndef BISON_IDMEF_CRITERIA_STRING_YAC_H
# define BISON_IDMEF_CRITERIA_STRING_YAC_H

#ifndef YYSTYPE
typedef union {
	char *str;
        int operator;
	idmef_criterion_t *criterion;
	idmef_criteria_t *criteria;
	idmef_value_relation_t relation;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	TOK_STRING	257
# define	TOK_RELATION_SUBSTRING	258
# define	TOK_RELATION_REGEXP	259
# define	TOK_RELATION_GREATER	260
# define	TOK_RELATION_GREATER_OR_EQUAL	261
# define	TOK_RELATION_LESS	262
# define	TOK_RELATION_LESS_OR_EQUAL	263
# define	TOK_RELATION_EQUAL	264
# define	TOK_RELATION_NOT_EQUAL	265
# define	TOK_RELATION_IS_NULL	266
# define	TOK_OPERATOR_AND	267
# define	TOK_OPERATOR_OR	268


extern YYSTYPE yylval;

#endif /* not BISON_IDMEF_CRITERIA_STRING_YAC_H */
