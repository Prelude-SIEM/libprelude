#ifndef _LIBPRELUDE_STRING_MATCHING_H
#define _LIBPRELUDE_STRING_MATCHING_H

/* Boyer-Moore structure */
typedef struct {

	char *pattern;
	int length;

	int *bad_character_shift;
	int *good_suffix_shift;

} bm_string_t;

bm_string_t *BoyerMoore_Init( char *y, int n );
void BoyerMoore_MakeCaseInsensitive( bm_string_t *string );


int BoyerMoore_StringMatching(char *y, int n, char *x, int m,
			      int bm_bc[], int bm_gs[] );

int BoyerMoore_CI_StringMatching(char *y, int n, char *x, int m,
			      int bm_bc[], int bm_gs[] );

void BoyerMoore_GoodSuffixShift(char *x, int m, int bm_gs[]);
void BoyerMoore_BadCharacterShift(char *x, int m, int bm_bc[]);

#endif LIBPRELUDE_STRING_MATCHING_H
