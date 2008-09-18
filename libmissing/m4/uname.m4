# uname.m4 serial 1
dnl Copyright (C) 2007 Free Software Foundation, Inc.
dnl This file is free software; the Free Software Foundation
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([gl_FUNC_UNAME],
[
  AC_CHECK_FUNCS_ONCE([uname])
  if test $ac_cv_func_uname = no; then
    HAVE_UNAME=0
    AC_LIBOBJ([uname])
    gl_PREREQ_UNAME
  fi
])

# Prerequisites of lib/uname.c.
AC_DEFUN([gl_PREREQ_UNAME], [:])
