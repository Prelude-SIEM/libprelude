# strcasestr.m4 serial 1
dnl Copyright (C) 2002, 2003, 2005 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_STRCASESTR],
[
  dnl Persuade glibc <string.h> to declare strcasestr().
  AC_REQUIRE([AC_GNU_SOURCE])

  AC_REPLACE_FUNCS(strcasestr)
  gl_PREREQ_STRCASESTR
])

# Prerequisites of lib/strcasestr.c.
AC_DEFUN([gl_PREREQ_STRCASESTR], [:])
