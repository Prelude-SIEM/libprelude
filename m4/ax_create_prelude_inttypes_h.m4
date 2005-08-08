AC_DEFUN([AX_CREATE_PRELUDE_INTTYPES_H],[
ac_prelude_inttypes_h="$1"
ac_prelude_inttypes_h_temp="$1_"

AC_SUBST(__PRELUDE_HAVE_STDINT_H)
AC_SUBST(__PRELUDE_HAVE_INTTYPES_H)
AC_SUBST(__PRELUDE_HAVE_64BIT_LONG)
AC_SUBST(__PRELUDE_STDINT_HAVE_UINT8)
AC_SUBST(__PRELUDE_STDINT_HAVE_UINT16)
AC_SUBST(__PRELUDE_STDINT_HAVE_UINT32)
AC_SUBST(__PRELUDE_STDINT_HAVE_UINT64)
AC_SUBST(__PRELUDE_64BIT_FORMAT_PREFIX)


ac_cv_have_stdint_h=
AC_CHECK_HEADER(stdint.h, ac_cv_have_stdint_h=yes, ac_cv_have_stdint_h=no)

if test "$ac_cv_have_stdint_h" = "yes" ; then
  __PRELUDE_HAVE_STDINT_H="#define __PRELUDE_HAVE_STDINT_H"
else
  __PRELUDE_HAVE_STDINT_H="/* #define __PRELUDE_HAVE_STDINT_H */"
fi

ac_cv_have_inttypes_h=
AC_CHECK_HEADER(inttypes.h, ac_cv_have_inttypes_h=yes, ac_cv_have_inttypes_h=no)

if test "$ac_cv_have_inttypes_h" = "yes" ; then
  __PRELUDE_HAVE_INTTYPES_H="#define __PRELUDE_HAVE_INTTYPES_H"
else
  __PRELUDE_HAVE_INTTYPES_H="/* #define __PRELUDE_HAVE_INTTYPES_H */"
fi

AC_COMPILE_CHECK_SIZEOF(long)

if test "$ac_cv_sizeof_long" = "8" ; then
  __PRELUDE_HAVE_64BIT_LONG="#define __PRELUDE_HAVE_64BIT_LONG"
else
  __PRELUDE_HAVE_64BIT_LONG="/* #define __PRELUDE_HAVE_64BIT_LONG */"
fi

ac_cv_have_uint8_t=
AC_CHECK_TYPE([uint8_t], ac_cv_have_uint8_t=yes, ac_cv_have_uint8_t=no)

if test "$ac_cv_have_uint8_t" = "yes" ; then
  __PRELUDE_STDINT_HAVE_UINT8="#define __PRELUDE_STDINT_HAVE_UINT8"
else
  __PRELUDE_STDINT_HAVE_UINT8="/* #define __PRELUDE_STDINT_HAVE_UINT8 */"
fi

ac_cv_have_uint16_t=
AC_CHECK_TYPE([uint16_t], ac_cv_have_uint16_t=yes, ac_cv_have_uint16_t=no)

if test "$ac_cv_have_uint16_t" = "yes" ; then
  __PRELUDE_STDINT_HAVE_UINT16="#define __PRELUDE_STDINT_HAVE_UINT16"
else
  __PRELUDE_STDINT_HAVE_UINT16="/* #define __PRELUDE_STDINT_HAVE_UINT16 */"
fi

ac_cv_have_uint32_t=
AC_CHECK_TYPE([uint32_t], ac_cv_have_uint32_t=yes, ac_cv_have_uint32_t=no)

if test "$ac_cv_have_uint32_t" = "yes" ; then
  __PRELUDE_STDINT_HAVE_UINT32="#define __PRELUDE_STDINT_HAVE_UINT32"
else
  __PRELUDE_STDINT_HAVE_UINT32="/* #define __PRELUDE_STDINT_HAVE_UINT32 */"
fi

ac_cv_have_uint64_t=
AC_CHECK_TYPE([uint64_t], ac_cv_have_uint64_t=yes, ac_cv_have_uint64_t=no)

if test "$ac_cv_have_uint64_t" = "yes" ; then
  __PRELUDE_STDINT_HAVE_UINT64="#define __PRELUDE_STDINT_HAVE_UINT64"
else
  __PRELUDE_STDINT_HAVE_UINT64="/* #define __PRELUDE_STDINT_HAVE_UINT64 */"
fi



dnl
dnl This check is adapted from glib.

AC_MSG_CHECKING(for printf and scanf 64 bits conversion specifier)

for format in l ll q I64; do
AC_TRY_RUN([

#include <stdio.h>
#include <stdlib.h>

$__PRELUDE_HAVE_STDINT_H
$__PRELUDE_HAVE_INTTYPES_H
$__PRELUDE_STDINT_HAVE_UINT64

#ifdef __PRELUDE_HAVE_STDINT_H
# include <stdint.h>
#endif

#ifdef __PRELUDE_HAVE_INTTYPES_H
# include <inttypes.h>
#endif

#ifndef __PRELUDE_STDINT_HAVE_UINT64
# ifdef __PRELUDE_HAVE_64BIT_LONG

typedef long int64_t;
typedef unsigned long uint64_t;

# else

typedef long long int64_t;
typedef unsigned long long uint64_t;

# endif
#endif

int main(void)
{
	char buf[1024];
	int64_t parsed;
	int64_t orig = -0x3afafafafafafafaLL;

	snprintf(buf, sizeof(buf), "%${format}u", orig);
	sscanf(buf, "%${format}u", &parsed);

	exit(parsed != orig);
}

], [format64_prefix=${format} 
    break], [], [:])

done


if test -n "$format64_prefix"; then
	AC_MSG_RESULT($format64_prefix)
	__PRELUDE_64BIT_FORMAT_PREFIX="#define __PRELUDE_64BIT_FORMAT_PREFIX \"$format64_prefix\""
else
	AC_MSG_RESULT(none found)
	AC_MSG_ERROR(could not find required prefix for 64bit format)
fi

])
