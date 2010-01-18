/* strerror.c - Describing an error code.
   Copyright (C) 2003 g10 Code GmbH

   This file is part of libgpg-error.

   libgpg-error is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.
 
   libgpg-error is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
 
   You should have received a copy of the GNU Lesser General Public
   License along with libgpg-error; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.  */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "prelude-error.h"
#include "gettext.h"
#include "err-codes.h"

/* Return a pointer to a string containing a description of the error
   code in the error value ERR.  This function is not thread-safe.  */
const char *prelude_strerror(prelude_error_t err)
{
	int no;
  	prelude_error_code_t code = prelude_error_get_code(err);

        if ( prelude_error_is_verbose(err) )
                return _prelude_thread_get_error();
        
  	if ( code & PRELUDE_ERROR_SYSTEM_ERROR ) {
      		no = prelude_error_code_to_errno(code);
      		if ( no )
			return strerror(no);
      		else
			code = PRELUDE_ERROR_UNKNOWN_ERRNO;
    	}
        
  	return dgettext(PACKAGE, msgstr + msgidx[msgidxof((int)code)]);
}
