# DO NOT EDIT! GENERATED AUTOMATICALLY!
# Copyright (C) 2002-2008 Free Software Foundation, Inc.
#
# This file is free software, distributed under the terms of the GNU
# General Public License.  As a special exception to the GNU General
# Public License, this file may be distributed as part of a program
# that contains a configuration script generated by Autoconf, under
# the same distribution terms as the rest of that program.
#
# Generated by gnulib-tool.
#
# This file represents the compiled summary of the specification in
# gnulib-cache.m4. It lists the computed macro invocations that need
# to be invoked from configure.ac.
# In projects using CVS, this file can be treated like other built files.


# This macro should be invoked from ./configure.in, in the section
# "Checks for programs", right after AC_PROG_CC, and certainly before
# any checks for libraries, header files, types and library functions.
AC_DEFUN([gl_EARLY],
[
  m4_pattern_forbid([^gl_[A-Z]])dnl the gnulib macro namespace
  m4_pattern_allow([^gl_ES$])dnl a valid locale name
  m4_pattern_allow([^gl_LIBOBJS$])dnl a variable
  m4_pattern_allow([^gl_LTLIBOBJS$])dnl a variable
  AC_REQUIRE([AC_PROG_RANLIB])
  AC_REQUIRE([AM_PROG_CC_C_O])
  AC_REQUIRE([AC_GNU_SOURCE])
  AC_REQUIRE([gl_USE_SYSTEM_EXTENSIONS])
  AC_REQUIRE([AC_FUNC_FSEEKO])
  gl_THREADLIB_EARLY
])

# This macro should be invoked from ./configure.in, in the section
# "Check for header files, types and library functions".
AC_DEFUN([gl_INIT],
[
  AM_CONDITIONAL([GL_COND_LIBTOOL], [true])
  gl_cond_libtool=true
  m4_pushdef([AC_LIBOBJ], m4_defn([gl_LIBOBJ]))
  m4_pushdef([AC_REPLACE_FUNCS], m4_defn([gl_REPLACE_FUNCS]))
  m4_pushdef([AC_LIBSOURCES], m4_defn([gl_LIBSOURCES]))
  m4_pushdef([gl_LIBSOURCES_LIST], [])
  m4_pushdef([gl_LIBSOURCES_DIR], [])
  gl_COMMON
  gl_source_base='libmissing'
changequote(,)dnl
LTALLOCA=`echo "$ALLOCA" | sed 's/\.[^.]* /.lo /g;s/\.[^.]*$/.lo/'`
changequote([, ])dnl
AC_SUBST([LTALLOCA])
  gl_FUNC_ALLOCA
  gl_HEADER_ARPA_INET
  AC_PROG_MKDIR_P
  gl_COND
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  if test "$ac_cv_header_winsock2_h" = yes; then
    AC_LIBOBJ([connect])
  fi
  gl_SYS_SOCKET_MODULE_INDICATOR([connect])
  gl_FUNC_DUP2
  gl_UNISTD_MODULE_INDICATOR([dup2])
  gl_HEADER_ERRNO_H
  gl_FLOAT_H
  gl_FUNC_FOPEN
  gl_STDIO_MODULE_INDICATOR([fopen])
  gl_FUNC_FSEEKO
  gl_STDIO_MODULE_INDICATOR([fseeko])
  gl_FUNC_FTW
  gl_GETADDRINFO
  gl_NETDB_MODULE_INDICATOR([getaddrinfo])
  gl_FUNC_GETDELIM
  gl_STDIO_MODULE_INDICATOR([getdelim])
  gl_FUNC_GETLINE
  gl_STDIO_MODULE_INDICATOR([getline])
  gl_FUNC_GETPASS
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  if test "$ac_cv_header_winsock2_h" = yes; then
    AC_LIBOBJ([getsockname])
  fi
  gl_SYS_SOCKET_MODULE_INDICATOR([getsockname])
  AC_SUBST([LIBINTL])
  AC_SUBST([LTLIBINTL])
  gl_FUNC_GETTIMEOFDAY
  gl_INET_NTOP
  gl_ARPA_INET_MODULE_INDICATOR([inet_ntop])
  gl_INET_PTON
  gl_ARPA_INET_MODULE_INDICATOR([inet_pton])
  gl_LOCALCHARSET
  LOCALCHARSET_TESTS_ENVIRONMENT="CHARSETALIASDIR=\"\$(top_builddir)/$gl_source_base\""
  AC_SUBST([LOCALCHARSET_TESTS_ENVIRONMENT])
  gl_LOCK
  gl_FUNC_LSEEK
  gl_UNISTD_MODULE_INDICATOR([lseek])
  AC_FUNC_MALLOC
  AC_DEFINE([GNULIB_MALLOC_GNU], 1, [Define to indicate the 'malloc' module.])
  gl_FUNC_MALLOC_POSIX
  gl_STDLIB_MODULE_INDICATOR([malloc-posix])
  gl_FUNC_MEMMEM
  gl_FUNC_MEMMEM_SIMPLE
  gl_STRING_MODULE_INDICATOR([memmem])
  gl_MINMAX
  gl_FUNC_MKTIME
  gl_HEADER_NETDB
  gl_HEADER_NETINET_IN
  AC_PROG_MKDIR_P
  gl_PATHMAX
  gl_FUNC_PERROR
  gl_STRING_MODULE_INDICATOR([perror])
  gl_FUNC_POLL
  gl_FUNC_REALLOC_POSIX
  gl_STDLIB_MODULE_INDICATOR([realloc-posix])
  gl_REGEX
  gl_RELOCATABLE_LIBRARY
  AC_REQUIRE([gl_HEADER_SYS_SELECT])
  if test "$ac_cv_header_winsock2_h" = yes; then
    AC_LIBOBJ([winsock-select])
  fi
  gl_SYS_SELECT_MODULE_INDICATOR([select])
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  if test "$ac_cv_header_winsock2_h" = yes; then
    AC_LIBOBJ([setsockopt])
  fi
  gl_SYS_SOCKET_MODULE_INDICATOR([setsockopt])
  gl_SIGNAL_H
  gl_SIGNALBLOCKING
  gl_SIGNAL_MODULE_INDICATOR([sigprocmask])
  gl_SIZE_MAX
  gl_FUNC_SLEEP
  gl_UNISTD_MODULE_INDICATOR([sleep])
  gl_FUNC_SNPRINTF
  gl_STDIO_MODULE_INDICATOR([snprintf])
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  if test "$ac_cv_header_winsock2_h" = yes; then
    AC_LIBOBJ([socket])
  fi
  gl_SYS_SOCKET_MODULE_INDICATOR([socket])
  gl_TYPE_SOCKLEN_T
  gt_TYPE_SSIZE_T
  AM_STDBOOL_H
  gl_STDINT_H
  gl_STDIO_H
  gl_STDLIB_H
  gl_STRCASE
  gl_FUNC_STRCASESTR
  gl_FUNC_STRCASESTR_SIMPLE
  gl_STRING_MODULE_INDICATOR([strcasestr])
  gl_FUNC_STRDUP
  gl_STRING_MODULE_INDICATOR([strdup])
  gl_FUNC_STRDUP_POSIX
  gl_STRING_MODULE_INDICATOR([strdup])
  gl_FUNC_STRERROR
  gl_STRING_MODULE_INDICATOR([strerror])
  gl_HEADER_STRING_H
  gl_HEADER_STRINGS_H
  gl_FUNC_STRNDUP
  gl_STRING_MODULE_INDICATOR([strndup])
  gl_FUNC_STRNLEN
  gl_STRING_MODULE_INDICATOR([strnlen])
  gl_FUNC_STRPTIME
  gl_FUNC_STRSEP
  gl_STRING_MODULE_INDICATOR([strsep])
  gl_HEADER_SYS_SELECT
  AC_PROG_MKDIR_P
  gl_HEADER_SYS_SOCKET
  gl_MODULE_INDICATOR([sys_socket])
  AC_PROG_MKDIR_P
  gl_HEADER_SYS_STAT_H
  AC_PROG_MKDIR_P
  gl_HEADER_SYS_TIME_H
  AC_PROG_MKDIR_P
  gl_THREAD
  gl_THREADLIB
  gl_HEADER_TIME_H
  gl_TIME_R
  gl_FUNC_TIMEGM
  gl_TLS
  gl_FUNC_UNAME
  gl_UNISTD_H
  gl_FUNC_VASNPRINTF
  gl_FUNC_VSNPRINTF
  gl_STDIO_MODULE_INDICATOR([vsnprintf])
  gl_WCHAR_H
  gl_WCTYPE_H
  gl_FUNC_WRITE
  gl_UNISTD_MODULE_INDICATOR([write])
  gl_XSIZE
  m4_ifval(gl_LIBSOURCES_LIST, [
    m4_syscmd([test ! -d ]m4_defn([gl_LIBSOURCES_DIR])[ ||
      for gl_file in ]gl_LIBSOURCES_LIST[ ; do
        if test ! -r ]m4_defn([gl_LIBSOURCES_DIR])[/$gl_file ; then
          echo "missing file ]m4_defn([gl_LIBSOURCES_DIR])[/$gl_file" >&2
          exit 1
        fi
      done])dnl
      m4_if(m4_sysval, [0], [],
        [AC_FATAL([expected source file, required through AC_LIBSOURCES, not found])])
  ])
  m4_popdef([gl_LIBSOURCES_DIR])
  m4_popdef([gl_LIBSOURCES_LIST])
  m4_popdef([AC_LIBSOURCES])
  m4_popdef([AC_REPLACE_FUNCS])
  m4_popdef([AC_LIBOBJ])
  AC_CONFIG_COMMANDS_PRE([
    gl_libobjs=
    gl_ltlibobjs=
    if test -n "$gl_LIBOBJS"; then
      # Remove the extension.
      sed_drop_objext='s/\.o$//;s/\.obj$//'
      for i in `for i in $gl_LIBOBJS; do echo "$i"; done | sed "$sed_drop_objext" | sort | uniq`; do
        gl_libobjs="$gl_libobjs $i.$ac_objext"
        gl_ltlibobjs="$gl_ltlibobjs $i.lo"
      done
    fi
    AC_SUBST([gl_LIBOBJS], [$gl_libobjs])
    AC_SUBST([gl_LTLIBOBJS], [$gl_ltlibobjs])
  ])
  gltests_libdeps=
  gltests_ltlibdeps=
  m4_pushdef([AC_LIBOBJ], m4_defn([gltests_LIBOBJ]))
  m4_pushdef([AC_REPLACE_FUNCS], m4_defn([gltests_REPLACE_FUNCS]))
  m4_pushdef([AC_LIBSOURCES], m4_defn([gltests_LIBSOURCES]))
  m4_pushdef([gltests_LIBSOURCES_LIST], [])
  m4_pushdef([gltests_LIBSOURCES_DIR], [])
  gl_COMMON
  gl_source_base='libmissing/tests'
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  if test "$ac_cv_header_winsock2_h" = yes; then
    AC_LIBOBJ([accept])
  fi
  gl_SYS_SOCKET_MODULE_INDICATOR([accept])
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  if test "$ac_cv_header_winsock2_h" = yes; then
    AC_LIBOBJ([bind])
  fi
  gl_SYS_SOCKET_MODULE_INDICATOR([bind])
  gl_FUNC_CLOSE
  gl_UNISTD_MODULE_INDICATOR([close])
  gl_FUNC_FCLOSE
  gl_STDIO_MODULE_INDICATOR([fclose])
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  if test "$ac_cv_header_winsock2_h" = yes; then
    AC_LIBOBJ([ioctl])
    gl_REPLACE_SYS_IOCTL_H
  fi
  gl_SYS_IOCTL_MODULE_INDICATOR([ioctl])
  gl_MODULE_INDICATOR([ioctl])
  AC_REQUIRE([gl_HEADER_SYS_SOCKET])
  if test "$ac_cv_header_winsock2_h" = yes; then
    AC_LIBOBJ([listen])
  fi
  gl_SYS_SOCKET_MODULE_INDICATOR([listen])
  AC_CHECK_DECLS_ONCE([alarm])
  AC_CHECK_HEADERS_ONCE([unistd.h sys/wait.h])
  gl_SOCKETS
  gt_TYPE_WCHAR_T
  gt_TYPE_WINT_T
  AC_CHECK_DECLS_ONCE([alarm])
  gl_SYS_IOCTL_H
  AC_PROG_MKDIR_P
  AC_CHECK_FUNCS([shutdown])
  gl_YIELD
  m4_ifval(gltests_LIBSOURCES_LIST, [
    m4_syscmd([test ! -d ]m4_defn([gltests_LIBSOURCES_DIR])[ ||
      for gl_file in ]gltests_LIBSOURCES_LIST[ ; do
        if test ! -r ]m4_defn([gltests_LIBSOURCES_DIR])[/$gl_file ; then
          echo "missing file ]m4_defn([gltests_LIBSOURCES_DIR])[/$gl_file" >&2
          exit 1
        fi
      done])dnl
      m4_if(m4_sysval, [0], [],
        [AC_FATAL([expected source file, required through AC_LIBSOURCES, not found])])
  ])
  m4_popdef([gltests_LIBSOURCES_DIR])
  m4_popdef([gltests_LIBSOURCES_LIST])
  m4_popdef([AC_LIBSOURCES])
  m4_popdef([AC_REPLACE_FUNCS])
  m4_popdef([AC_LIBOBJ])
  AC_CONFIG_COMMANDS_PRE([
    gltests_libobjs=
    gltests_ltlibobjs=
    if test -n "$gltests_LIBOBJS"; then
      # Remove the extension.
      sed_drop_objext='s/\.o$//;s/\.obj$//'
      for i in `for i in $gltests_LIBOBJS; do echo "$i"; done | sed "$sed_drop_objext" | sort | uniq`; do
        gltests_libobjs="$gltests_libobjs $i.$ac_objext"
        gltests_ltlibobjs="$gltests_ltlibobjs $i.lo"
      done
    fi
    AC_SUBST([gltests_LIBOBJS], [$gltests_libobjs])
    AC_SUBST([gltests_LTLIBOBJS], [$gltests_ltlibobjs])
  ])
  LIBTESTS_LIBDEPS="$gltests_libdeps"
  AC_SUBST([LIBTESTS_LIBDEPS])
])

# Like AC_LIBOBJ, except that the module name goes
# into gl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gl_LIBOBJ], [
  AS_LITERAL_IF([$1], [gl_LIBSOURCES([$1.c])])dnl
  gl_LIBOBJS="$gl_LIBOBJS $1.$ac_objext"
])

# Like AC_REPLACE_FUNCS, except that the module name goes
# into gl_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gl_REPLACE_FUNCS], [
  m4_foreach_w([gl_NAME], [$1], [AC_LIBSOURCES(gl_NAME[.c])])dnl
  AC_CHECK_FUNCS([$1], , [gl_LIBOBJ($ac_func)])
])

# Like AC_LIBSOURCES, except the directory where the source file is
# expected is derived from the gnulib-tool parameterization,
# and alloca is special cased (for the alloca-opt module).
# We could also entirely rely on EXTRA_lib..._SOURCES.
AC_DEFUN([gl_LIBSOURCES], [
  m4_foreach([_gl_NAME], [$1], [
    m4_if(_gl_NAME, [alloca.c], [], [
      m4_define([gl_LIBSOURCES_DIR], [libmissing])
      m4_append([gl_LIBSOURCES_LIST], _gl_NAME, [ ])
    ])
  ])
])

# Like AC_LIBOBJ, except that the module name goes
# into gltests_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gltests_LIBOBJ], [
  AS_LITERAL_IF([$1], [gltests_LIBSOURCES([$1.c])])dnl
  gltests_LIBOBJS="$gltests_LIBOBJS $1.$ac_objext"
])

# Like AC_REPLACE_FUNCS, except that the module name goes
# into gltests_LIBOBJS instead of into LIBOBJS.
AC_DEFUN([gltests_REPLACE_FUNCS], [
  m4_foreach_w([gl_NAME], [$1], [AC_LIBSOURCES(gl_NAME[.c])])dnl
  AC_CHECK_FUNCS([$1], , [gltests_LIBOBJ($ac_func)])
])

# Like AC_LIBSOURCES, except the directory where the source file is
# expected is derived from the gnulib-tool parameterization,
# and alloca is special cased (for the alloca-opt module).
# We could also entirely rely on EXTRA_lib..._SOURCES.
AC_DEFUN([gltests_LIBSOURCES], [
  m4_foreach([_gl_NAME], [$1], [
    m4_if(_gl_NAME, [alloca.c], [], [
      m4_define([gltests_LIBSOURCES_DIR], [libmissing/tests])
      m4_append([gltests_LIBSOURCES_LIST], _gl_NAME, [ ])
    ])
  ])
])

# This macro records the list of files which have been installed by
# gnulib-tool and may be removed by future gnulib-tool invocations.
AC_DEFUN([gl_FILE_LIST], [
  build-aux/config.rpath
  build-aux/link-warning.h
  doc/relocatable.texi
  lib/alloca.c
  lib/alloca.in.h
  lib/arpa_inet.in.h
  lib/asnprintf.c
  lib/c-ctype.c
  lib/c-ctype.h
  lib/config.charset
  lib/connect.c
  lib/dup2.c
  lib/errno.in.h
  lib/float+.h
  lib/float.in.h
  lib/fopen.c
  lib/fseeko.c
  lib/ftw.c
  lib/ftw_.h
  lib/gai_strerror.c
  lib/getaddrinfo.c
  lib/getdelim.c
  lib/getline.c
  lib/getpass.c
  lib/getpass.h
  lib/getsockname.c
  lib/gettext.h
  lib/gettimeofday.c
  lib/glthread/cond.c
  lib/glthread/cond.h
  lib/glthread/lock.c
  lib/glthread/lock.h
  lib/glthread/thread.c
  lib/glthread/thread.h
  lib/glthread/threadlib.c
  lib/glthread/tls.c
  lib/glthread/tls.h
  lib/inet_ntop.c
  lib/inet_pton.c
  lib/intprops.h
  lib/localcharset.c
  lib/localcharset.h
  lib/lseek.c
  lib/malloc.c
  lib/memmem.c
  lib/minmax.h
  lib/mktime.c
  lib/netdb.in.h
  lib/netinet_in.in.h
  lib/pathmax.h
  lib/perror.c
  lib/poll.c
  lib/poll.in.h
  lib/printf-args.c
  lib/printf-args.h
  lib/printf-parse.c
  lib/printf-parse.h
  lib/realloc.c
  lib/ref-add.sin
  lib/ref-del.sin
  lib/regcomp.c
  lib/regex.c
  lib/regex.h
  lib/regex_internal.c
  lib/regex_internal.h
  lib/regexec.c
  lib/relocatable.c
  lib/relocatable.h
  lib/setsockopt.c
  lib/signal.in.h
  lib/sigprocmask.c
  lib/size_max.h
  lib/sleep.c
  lib/snprintf.c
  lib/socket.c
  lib/stdbool.in.h
  lib/stdint.in.h
  lib/stdio-impl.h
  lib/stdio-write.c
  lib/stdio.in.h
  lib/stdlib.in.h
  lib/str-two-way.h
  lib/strcasecmp.c
  lib/strcasestr.c
  lib/strdup.c
  lib/strerror.c
  lib/string.in.h
  lib/strings.in.h
  lib/strncasecmp.c
  lib/strndup.c
  lib/strnlen.c
  lib/strptime.c
  lib/strsep.c
  lib/sys_select.in.h
  lib/sys_socket.in.h
  lib/sys_stat.in.h
  lib/sys_time.in.h
  lib/time.in.h
  lib/time_r.c
  lib/timegm.c
  lib/uname.c
  lib/uname.h
  lib/unistd.in.h
  lib/vasnprintf.c
  lib/vasnprintf.h
  lib/vsnprintf.c
  lib/w32sock.h
  lib/wchar.in.h
  lib/wctype.in.h
  lib/winsock-select.c
  lib/write.c
  lib/xsize.h
  m4/alloca.m4
  m4/arpa_inet_h.m4
  m4/close.m4
  m4/codeset.m4
  m4/cond.m4
  m4/dup2.m4
  m4/errno_h.m4
  m4/extensions.m4
  m4/fclose.m4
  m4/float_h.m4
  m4/fopen.m4
  m4/fseeko.m4
  m4/ftw.m4
  m4/getaddrinfo.m4
  m4/getdelim.m4
  m4/getline.m4
  m4/getpass.m4
  m4/gettimeofday.m4
  m4/glibc21.m4
  m4/gnulib-common.m4
  m4/include_next.m4
  m4/inet_ntop.m4
  m4/inet_pton.m4
  m4/intmax_t.m4
  m4/inttypes_h.m4
  m4/lib-ld.m4
  m4/lib-link.m4
  m4/lib-prefix.m4
  m4/localcharset.m4
  m4/lock.m4
  m4/longlong.m4
  m4/lseek.m4
  m4/malloc.m4
  m4/mbstate_t.m4
  m4/memmem.m4
  m4/minmax.m4
  m4/mktime.m4
  m4/netdb_h.m4
  m4/netinet_in_h.m4
  m4/onceonly.m4
  m4/pathmax.m4
  m4/perror.m4
  m4/poll.m4
  m4/printf.m4
  m4/realloc.m4
  m4/regex.m4
  m4/relocatable-lib.m4
  m4/signal_h.m4
  m4/signalblocking.m4
  m4/size_max.m4
  m4/sleep.m4
  m4/snprintf.m4
  m4/sockets.m4
  m4/socklen.m4
  m4/sockpfaf.m4
  m4/ssize_t.m4
  m4/stdbool.m4
  m4/stdint.m4
  m4/stdint_h.m4
  m4/stdio_h.m4
  m4/stdlib_h.m4
  m4/strcase.m4
  m4/strcasestr.m4
  m4/strdup.m4
  m4/strerror.m4
  m4/string_h.m4
  m4/strings_h.m4
  m4/strndup.m4
  m4/strnlen.m4
  m4/strptime.m4
  m4/strsep.m4
  m4/sys_ioctl_h.m4
  m4/sys_select_h.m4
  m4/sys_socket_h.m4
  m4/sys_stat_h.m4
  m4/sys_time_h.m4
  m4/thread.m4
  m4/threadlib.m4
  m4/time_h.m4
  m4/time_r.m4
  m4/timegm.m4
  m4/tls.m4
  m4/tm_gmtoff.m4
  m4/uname.m4
  m4/unistd_h.m4
  m4/vasnprintf.m4
  m4/vsnprintf.m4
  m4/wchar.m4
  m4/wchar_t.m4
  m4/wctype.m4
  m4/wint_t.m4
  m4/write.m4
  m4/xsize.m4
  m4/yield.m4
  tests/test-alloca-opt.c
  tests/test-arpa_inet.c
  tests/test-c-ctype.c
  tests/test-cond.c
  tests/test-errno.c
  tests/test-fopen.c
  tests/test-fseeko.c
  tests/test-fseeko.sh
  tests/test-getaddrinfo.c
  tests/test-getdelim.c
  tests/test-getline.c
  tests/test-gettimeofday.c
  tests/test-lock.c
  tests/test-lseek.c
  tests/test-lseek.sh
  tests/test-memmem.c
  tests/test-netdb.c
  tests/test-netinet_in.c
  tests/test-perror.c
  tests/test-perror.sh
  tests/test-poll.c
  tests/test-select.c
  tests/test-sleep.c
  tests/test-snprintf.c
  tests/test-sockets.c
  tests/test-stdbool.c
  tests/test-stdint.c
  tests/test-stdio.c
  tests/test-stdlib.c
  tests/test-strcasestr.c
  tests/test-strerror.c
  tests/test-string.c
  tests/test-strings.c
  tests/test-sys_select.c
  tests/test-sys_socket.c
  tests/test-sys_stat.c
  tests/test-sys_time.c
  tests/test-time.c
  tests/test-tls.c
  tests/test-unistd.c
  tests/test-vasnprintf.c
  tests/test-vsnprintf.c
  tests/test-wchar.c
  tests/test-wctype.c
  tests=lib/accept.c
  tests=lib/bind.c
  tests=lib/close.c
  tests=lib/fclose.c
  tests=lib/glthread/yield.h
  tests=lib/ioctl.c
  tests=lib/listen.c
  tests=lib/sockets.c
  tests=lib/sockets.h
  tests=lib/sys_ioctl.in.h
  tests=lib/verify.h
  tests=lib/w32sock.h
])
