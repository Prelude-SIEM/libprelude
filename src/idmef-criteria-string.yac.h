#ifndef BISON_IDMEF_CRITERIA_STRING_YAC_H
# define BISON_IDMEF_CRITERIA_STRING_YAC_H

#ifndef YYSTYPE
typedef union {
	char *str;
	idmef_criterion_t *criterion;
	idmef_criteria_t *criteria;
	idmef_relation_t relation;
	idmef_operator_t operator;
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


#endif /* not BISON_IDMEF_CRITERIA_STRING_YAC_H */
