#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <inttypes.h>

#include "string-matching.h"

/*
 * Boyer-Moore Algorithem
 */

/* Initialisation */
bm_string_t *BoyerMoore_Init( char *y, int n ) 
{
	bm_string_t *string = (bm_string_t*) malloc( sizeof(bm_string_t) );
	if (!string) {
		perror( "malloc" );
		return NULL;
	}

	string->length = n;

	string->pattern = (char*) malloc( sizeof(char)*(n+20) );
	memcpy( string->pattern, y, n );
	string->pattern[n] = '\0'; /* just for debugging..*/

	string->bad_character_shift = (int*) malloc( sizeof(int)*256 );
	string->good_suffix_shift = (int*) malloc( sizeof(int)*(n+1) );

	if (!string->pattern || !string->bad_character_shift || !string->good_suffix_shift) {
		perror( "malloc" );
		return NULL;
	}

	BoyerMoore_BadCharacterShift( string->pattern, string->length, string->bad_character_shift );
	BoyerMoore_GoodSuffixShift( string->pattern, string->length, string->good_suffix_shift );
	
	return string;

}

/* Case Insensitive */
void BoyerMoore_MakeCaseInsensitive( bm_string_t *string )
{
	char *s;
	if (!string)
		return;

	s = string->pattern;
	while( *s != '\0' ) {
		*s = toupper( *s );
		s++;
	}

	BoyerMoore_BadCharacterShift( string->pattern, string->length, string->bad_character_shift );
	BoyerMoore_GoodSuffixShift( string->pattern, string->length, string->good_suffix_shift );

}

/* Preprocessing of the Bad Character function shift. */
void BoyerMoore_BadCharacterShift(char *x, int m, int *bm_bc) 
{
	int a, j;
	
	for (a = 0; a < 256; a++) 
		bm_bc[ a ] = m;

	for (j=0; j < m-1; j++)
		bm_bc[ (unsigned char)(x[j]) ] = m-j-1;

}


/* Preprocessing of the Good Suffix function shift. */
void BoyerMoore_GoodSuffixShift(char *x, int m, int *bm_gs) 
{
	int i, j, p, *f = (int*) malloc( (m+1)*sizeof(int) );
	
	if (!f) {
		perror ("malloc" );
		return;
	}

	memset( bm_gs, 0, (m+1)*sizeof(int) );

	f[ m ] = j = m+1;

	for (i = m; i > 0; i-- ) {

		while (j <= m && x[i-1] != x[j-1]) {

			if (bm_gs[j] == 0) 
				bm_gs[j] = j-i;

			j=f[j];
		}
		f[i-1] = --j;
	}

	p = f[0];
	for ( j = 0; j <= m; ++j) {

		if (bm_gs[j] == 0) 
			bm_gs[j]=p;
		if (j == p) 
			p= f[p];
	}
	free(f);
}


int BoyerMoore_StringMatching(char *data, int dlen, char *ptrn, int plen,
			      int bm_bc[], int bm_gs[] ) {

	int i, j;
         
	i = 0;

	while(i <= dlen-plen) {
		
		for (j = plen - 1; j >= 0 && data[i + j] == ptrn[j]; --j);

		if (j < 0)
			return i+1;
		else {
			int k = bm_bc[(uint8_t)data[i + j]] - plen + j + 1;
			i += (k > bm_gs[j + 1]) ? k : bm_gs[j + 1];
		}
	}
	
	return 0;
}

int BoyerMoore_CI_StringMatching(char *data, int dlen, char *ptrn, int plen,
                                 int bm_bc[], int bm_gs[] ) {

	int i, j;
         
	i = 0;

	while (i <= dlen-plen) {
		
		for (j = plen - 1; j >= 0 && toupper(data[i + j]) == ptrn[j]; --j);

		if (j < 0)
			return i+1;
		else {
			int k = bm_bc[toupper((uint8_t)data[i + j])] - plen + j + 1;
			i += (k > bm_gs[j + 1]) ? k : bm_gs[j + 1];
		}
	}
	
	return 0;
}
