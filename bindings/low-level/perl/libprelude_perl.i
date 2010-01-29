/*****
*
* Copyright (C) 2003, 2004, 2005 PreludeIDS Technologies. All Rights Reserved.
* Author: Nicolas Delon <nicolas.delon@prelude-ids.com>
*
* This file is part of the Prelude library.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by 
* the Free Software Foundation; either version 2, or (at your option)
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; see the file COPYING.  If not, write to
* the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*
*****/

%module Prelude

%{
void swig_perl_raise_error(int error)
{
	char buf[1024];

	snprintf(buf, sizeof(buf), "Prelude error - %s: %s", prelude_strsource(error), prelude_strerror(error));
	croak(buf);
}


SV *swig_perl_string(prelude_string_t *string)
{
	if ( string ) {
		return newSVpv(prelude_string_get_string(string), prelude_string_get_len(string));
	} else {
		SvREFCNT_inc (& PL_sv_undef);
		return &PL_sv_undef;
	}
}


SV *swig_perl_data(idmef_data_t *data)
{
	switch ( idmef_data_get_type(data) ) {
	case IDMEF_DATA_TYPE_CHAR: 
	case IDMEF_DATA_TYPE_BYTE:
		return newSVpv((const char *)idmef_data_get_data(data), 1);

	case IDMEF_DATA_TYPE_CHAR_STRING: 
		return newSVpv((const char *)idmef_data_get_data(data), idmef_data_get_len(data) - 1);

	case IDMEF_DATA_TYPE_BYTE_STRING:
		return newSVpv((const char *)idmef_data_get_data(data), idmef_data_get_len(data));

	case IDMEF_DATA_TYPE_UINT32:
		return newSVpvf("%d", idmef_data_get_uint32(data));

	case IDMEF_DATA_TYPE_UINT64:
		return newSVpvf("%" PRELUDE_PRIu64, idmef_data_get_uint64(data));

	case IDMEF_DATA_TYPE_FLOAT:
		return newSVpvf("%hf", idmef_data_get_float(data));

	default:
		SvREFCNT_inc (& PL_sv_undef);
		return &PL_sv_undef;
	}
}

%}


/* This typemap is used to allow NULL pointers in _get_next_* functions
 */
%typemap(in) SWIGTYPE *LISTEDPARAM {
	if ( ! SvOK($input) ) {
                $1 = NULL;
	} else {
		if ( SWIG_ConvertPtr($input, (void **)&$1, $1_descriptor, 0) ) {
			croak("Expected type $1_type for argument $argnum.");
			return;
		}
	}
}


%typemap(in) char **argv {
	AV *tempav;
	I32 len;
	int i;
	SV  **tv;

	if ( ! SvROK($input) )
	    croak("Argument $argnum is not a reference.");

        if ( SvTYPE(SvRV($input)) != SVt_PVAV )
	    croak("Argument $argnum is not an array.");

        tempav = (AV*) SvRV($input);
	len = av_len(tempav);
	$1 = (char **) malloc((len+2)*sizeof(char *));
	if ( ! $1 )
		croak("out of memory\n");
	for (i = 0; i <= len; i++) {
	    tv = av_fetch(tempav, i, 0);	
	    SvREFCNT_inc(*tv);
            $1[i] = (char *) SvPV_nolen(*tv);
	}
	$1[i] = NULL;
};



%typemap(freearg) char **argv {
	free($1);
};




/**
 * Prelude specific typemaps
 */
%typemap(in) (char *data, size_t len) {
	$1 = SvPV_nolen($input);
	$2 = SvCUR($input);
};

%typemap(in) (const char *data, size_t len) {
	$1 = SvPV_nolen($input);
	$2 = SvCUR($input);
};

%typemap(in) (unsigned char *data, size_t len) {
	$1 = (unsigned char *) SvPV_nolen($input);
	$2 = SvCUR($input);
};

%typemap(in) (const unsigned char *data, size_t len) {
	$1 = (unsigned char *) SvPV_nolen($input);
	$2 = SvCUR($input);
};

%typemap(in) (const void *data, size_t len) {
	$1 = SvPV_nolen($input);
	$2 = SvCUR($input);
};


%typemap(in) (uint64_t *target_id, size_t size) {
	int i;
	AV *av;
	SV **sv;

        if ( ! (SvROK($input) && SvTYPE(SvRV($input)) == SVt_PVAV) )
		croak("Argument $argnum is not an array.");
	
	av = (AV *) SvRV($input);

	$2 = av_len(av) + 1; /* av_len returns the highest index of the array NOT the len of the array  */
	$1 = malloc($2 * sizeof (uint64_t));
	for ( i = 0; i < $2; i++ ) {
		sv = av_fetch(av, i, 0);
		$1[i] = strtoull(SvPV_nolen(*sv), NULL, 0);
	}
};

%typemap(freearg) (uint64_t *target_id, size_t size) {
	free($1);
};

%typemap(out) int {
	$result = newSViv($1);
    	argvi++;
};

%typemap(out) INTPOINTER * {
	if ( $1 != NULL ) {
		$result = newSViv(*$1);
	} else {
		SvREFCNT_inc (& PL_sv_undef);
		$result = &PL_sv_undef;
	}
}

%typemap(out) UINTPOINTER * {
	if ( $1 != NULL ) {
		$result = newSVuv(*$1);
	} else {
		SvREFCNT_inc (& PL_sv_undef);
		$result = &PL_sv_undef;
	}
}

%typemap(out) INT64POINTER * {
	if ( $1 != NULL ) {
		$result = newSViv(*$1);
	} else {
		SvREFCNT_inc (& PL_sv_undef);
		$result = &PL_sv_undef;
	}
}

%typemap(out) UINT64POINTER * {
	if ( $1 != NULL ) {
		$result = newSVuv(*$1);
	} else {
		SvREFCNT_inc (& PL_sv_undef);
		$result = &PL_sv_undef;
	}
}



%typemap(argout) (uint64_t *source_id, uint32_t *request_id, void **value) {
	int ret = SvIV($result);

	XPUSHs(sv_2mortal(newSVpvf("%" PRELUDE_PRIu64, *$1)));
	XPUSHs(sv_2mortal(newSVuv(*$2)));
	XPUSHs(sv_2mortal(newSViv(ret)));

	if ( *$3 ) {
		switch ( ret ) {
		case PRELUDE_OPTION_REPLY_TYPE_LIST:
			XPUSHs(sv_2mortal(SWIG_NewPointerObj((void *) * $3, SWIG_TypeQuery("prelude_option_t *"), 0)));
			break;
		default:
			XPUSHs(sv_2mortal(newSVpv(*$3, strlen(*$3))));
			break;
		}
	} else {
		SvREFCNT_inc (& PL_sv_undef);
		XPUSHs(&PL_sv_undef);
	}
};


%typemap(in) int *argc (int tmp) {
	tmp = SvIV($input);
	$1 = &tmp;
};


%typemap(in) prelude_string_t * {
	int ret;
	char *str;
	STRLEN len;

	str = SvPV($input, len);
	
	ret = prelude_string_new_dup_fast(&($1), str, len);
	if ( ret < 0 ) {
		swig_perl_raise_error(ret);
		return;
	}
};


%typemap(out) prelude_string_t * {
	$result = swig_perl_string($1);
	argvi++;
};


%typemap(out) idmef_data_t * {
	$result = swig_perl_data($1);
	argvi++;
};


%typemap(out) void * idmef_value_get_object {
	void *swig_type;

	swig_type = swig_idmef_value_get_descriptor(arg1);
	if ( ! swig_type ) {
		SvREFCNT_inc (& PL_sv_undef);
		$result = &PL_sv_undef;
	} else {
		$result = SWIG_NewPointerObj($1, swig_type, 0);
	}

	argvi++;
};


%clear idmef_value_t **ret;
%typemap(argout) idmef_value_t **ret {

	if ( ! SvROK($input) ) {
		croak("Argument $argnum is not a reference.");
		return;
	}

	if ( result == 0 ) {
		SvREFCNT_inc (& PL_sv_undef);
		$result = &PL_sv_undef;

	} else if ( result > 0 ) {
		SV *sv;

		sv = SvRV($input);
		sv_setsv(sv, SWIG_NewPointerObj((void *) * $1, $*1_descriptor, 0));
	}
};


%typemap(in, numinputs=0) prelude_string_t *out {
	int ret;
	
	ret = prelude_string_new(&($1));
	if ( ret < 0 ) {
		swig_perl_raise_error(ret);
		return;
	}
};


%typemap(argout) prelude_string_t *out {
	if ( result >= 0 ) {
		$result = newSVpv(prelude_string_get_string($1), prelude_string_get_len($1));
		argvi++;
	}

	prelude_string_destroy($1);
};


%typemap(in) prelude_msg_t **outmsg ($*1_type tmp) {
	tmp = NULL;
	$1 = ($1_ltype) &tmp;
};


%typemap(in) prelude_connection_t **outconn ($*1_type tmp) {
	tmp = NULL;
	$1 = ($1_ltype) &tmp;
};


%typemap(in) SWIGTYPE **OUTPARAM ($*1_type tmp) {
	$1 = ($1_ltype) &tmp;
};



%typemap(argout) SWIGTYPE **OUTPARAM {
	SV *sv;
	
	if ( result >= 0 ) {
		if ( ! SvROK($input) ) {
			croak("Argument $argnum is not a reference.");
			return;
		}

		sv = SvRV($input);
		sv_setsv(sv, SWIG_NewPointerObj((void *) * $1, $*1_descriptor, 0));
	}
};


%typemap(in) SWIGTYPE *INPARAM {
	if ( ! SvROK($input) ) {
		croak("Argument $argnum is null.");
		return;
	}

	if ( SWIG_ConvertPtr($input, (void **)&arg$argnum, $1_descriptor, 0) ) {
		croak("Expected type $1_type for argument $argnum.");
		return;
	}
}
