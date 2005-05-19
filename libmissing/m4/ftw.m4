# strsep.m4 serial 3
dnl Copyright (C) 2002, 2003, 2004 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_FTW],
[
  AC_REPLACE_FUNCS(ftw)
  gl_PREREQ_FTW
])

# Prerequisites of lib/ftw.c.
AC_DEFUN([gl_PREREQ_FTW], [:])
