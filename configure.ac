m4_define([libprelude_major_version], [3])
m4_define([libprelude_minor_version], [0])
m4_define([libprelude_micro_version], [0])
m4_define([libprelude_level_version], [LIBPRELUDE_RELEASE_LEVEL_FINAL])
m4_define([libprelude_serial_version], [0])

m4_define([serial_string], ifelse(libprelude_level_version, LIBPRELUDE_RELEASE_LEVEL_ALPHA, alpha,
                                  ifelse(libprelude_level_version, LIBPRELUDE_RELEASE_LEVEL_BETA, beta,
                                  ifelse(libprelude_level_version, LIBPRELUDE_RELEASE_LEVEL_RC, rc))))

m4_define([version_error], ifelse(serial_string, [], ,
                                  ifelse(libprelude_serial_version, 0, error)))
ifelse(version_error, [], , [m4_fatal(Release level 'serial_string' and serial 'libprelude_serial_version' are incompatible)])

m4_append([serial_string], ifelse(serial_string, [],
                                  ifelse(libprelude_serial_version, 0, , .libprelude_serial_version),
                                  libprelude_serial_version))

AC_INIT(libprelude, m4_join([.], [libprelude_major_version],
                                 [libprelude_minor_version],
                                 m4_join([], libprelude_micro_version, serial_string)))

LIBPRELUDE_MAJOR_VERSION=libprelude_major_version
LIBPRELUDE_MINOR_VERSION=libprelude_minor_version
LIBPRELUDE_MICRO_VERSION=libprelude_micro_version
LIBPRELUDE_LEVEL_VERSION=libprelude_level_version
LIBPRELUDE_SERIAL_VERSION=libprelude_serial_version


dnl **********************************************************
dnl * Library soname (https://www.sourceware.org/autobook/autobook/autobook_61.html#Library-Versioning)
dnl **********************************************************

libprelude_current=25
libprelude_revision=0
libprelude_age=2
LIBPRELUDE_SONAME=$libprelude_current:$libprelude_revision:$libprelude_age

libpreludecpp_current=8
libpreludecpp_revision=0
libpreludecpp_age=0
LIBPRELUDECPP_SONAME=$libpreludecpp_current:$libpreludecpp_revision:$libpreludecpp_age


AC_PREREQ(2.59)
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AC_CONFIG_SRCDIR([src])
AM_INIT_AUTOMAKE([-Wall tar-pax 1.9])

AC_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

dnl **********************************************************
dnl * Check required program.
dnl **********************************************************

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AM_PROG_LEX
AC_PROG_YACC
AC_PATH_PROG(GREP, grep, no) # for prelude-config.h generation
BINDINGS_CC="$CC"


dnl **********************************************************
dnl * Setup GnuLib, libtool and libltdl.
dnl **********************************************************

dnl Provide replacement for system missing this GnuLib required macro.
m4_ifndef([AS_VAR_COPY], [m4_define([AS_VAR_COPY], 
       [AS_LITERAL_IF([$1[]$2], [$1=$$2], [eval $1=\$$2])])]) 

gl_EARLY
LT_CONFIG_LTDL_DIR([libltdl])
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
LT_INIT([dlopen win32-dll disable-static])

dnl There is a bug in LTDL_INIT, which modify $LIBS when it shouldn't
old_LIBS=$LIBS
LTDL_INIT([recursive])
LIBS=$old_LIBS

if test x$LIBLTDL != x"\${top_build_prefix}libltdl/libltdlc.la"; then
	LIBLTDL_DECL=$LIBLTDL
fi

AC_SUBST(LIBLTDL_DECL)

dnl **********************************************************
dnl * When cross compiling, we need to compile and run a program on the build machine.
dnl **********************************************************

AC_MSG_CHECKING(for cc for build)
if test "$cross_compiling" = "yes"; then
  CC_FOR_BUILD="${CC_FOR_BUILD-cc}"
else
  CC_FOR_BUILD="${CC_FOR_BUILD-$CC}"
fi
AC_MSG_RESULT($CC_FOR_BUILD)
AC_ARG_VAR(CC_FOR_BUILD,[build system C compiler])



dnl **********************************************************
dnl * Checks to see if ANSI C99 CPP variable __func__ works. *
dnl **********************************************************

dnl From http://lists.gnu.org/archive/html/bug-autoconf/2002-07/msg00028.html

AC_DEFUN([AC_CPP_FUNC], [AC_REQUIRE([AC_PROG_CC_STDC])dnl
AC_CACHE_CHECK([for an ANSI C99-conforming __func__], ac_cv_cpp_func,
[AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
[[char *foo = __func__;]])],
  [ac_cv_cpp_func=yes],
  [AC_COMPILE_IFELSE([AC_LANG_PROGRAM([],
[[char *foo = __FUNCTION__;]])],
  [ac_cv_cpp_func=__FUNCTION__],
  [ac_cv_cpp_func=no])])])

if test $ac_cv_cpp_func = yes; then
   AC_DEFINE(__PRELUDE_FUNC__, __func__, [Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C.])
elif test $ac_cv_cpp_func = __FUNCTION__; then
   AC_DEFINE(__PRELUDE_FUNC__, __FUNCTION__, [Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C.])
elif test $ac_cv_cpp_func = no; then
   AC_DEFINE(__PRELUDE_FUNC__, "", [Define to __FUNCTION__ or "" if `__func__' does not conform to ANSI C.])
fi
])

AC_CPP_FUNC



dnl **************************************************
dnl * Check for Pthread library                      *
dnl **************************************************

PTHREAD_LIBS=@LTLIBMULTITHREAD@
PTHREAD_LDFLAGS=""
PTHREAD_CFLAGS=$THREADCPPFLAGS

AC_SUBST(PTHREAD_LDFLAGS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_LIBS)

dnl ***
dnl *** Can we use -Werror -Wmissing-braces for some checks ? ***
AX_CHECK_COMPILE_FLAG(-Werror -Wmissing-braces, buggy_pthread_cppflags="-Werror -Wmissing-braces")


old_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $buggy_pthread_cppflags"

AC_MSG_CHECKING(for buggy pthread mutex initializers)
AC_TRY_COMPILE(
[
#include <pthread.h>
],
[
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
],
compile_ok="yes", compile_ok="no")

CFLAGS="$old_CFLAGS"

if test x$compile_ok = xyes; then
   AC_MSG_RESULT(no)
else
   AC_MSG_RESULT(yes)
   buggy_pthread_initializers="yes"
fi


dnl **************************************************
dnl * Check endianess                                *
dnl **************************************************
AC_C_BIGENDIAN(AC_DEFINE_UNQUOTED(PRELUDE_WORDS_BIGENDIAN, , Byte ordering))



dnl **************************************************
dnl * Check for Gcrypt.                              *
dnl **************************************************

AM_PATH_LIBGCRYPT(1:1.1.94,,
        AC_MSG_ERROR([[
***
*** libgcrypt was not found. You may want to get it from
*** ftp://ftp.gnupg.org/gcrypt/libgcrypt/
***
]]))



dnl **************************************************
dnl * Check for GnuTLS.                              *
dnl **************************************************
GNUTLS_MIN_VERSION=1.0.17

PKG_CHECK_MODULES([LIBGNUTLS], [gnutls >= $GNUTLS_MIN_VERSION], [],
                  [AM_PATH_LIBGNUTLS($GNUTLS_MIN_VERSION, [], [AC_MSG_ERROR($LIBGNUTLS_PKG_ERRORS)])])

old_LIBS=$LIBS
old_CPPFLAGS=$CPPFLAGS
LIBS="$LIBGNUTLS_LIBS"
CPPFLAGS="$LIBGNUTLS_CFLAGS"

AC_CHECK_HEADER(gnutls/gnutls.h, ,
                AC_MSG_ERROR("GnuTLS development headers are required to build libprelude"))

AC_CHECK_LIB(gnutls, gnutls_privkey_import_x509,
             AC_DEFINE_UNQUOTED(HAVE_GNUTLS_PRIVKEY_ABSTRACT, , Define whether GnuTLS provides an abstract privkey implementation,))

dnl
dnl Recent GnuTLS version moved SRP function from libgnutls-extra to libgnutls
dnl
AC_CHECK_LIB(gnutls, gnutls_srp_verifier, srp_available=yes, srp_available=no)
AC_CHECK_LIB(gnutls, gnutls_x509_crt_list_import,
             AC_DEFINE_UNQUOTED(HAVE_GNUTLS_X509_CRT_LIST_IMPORT, , Define whether GnuTLS provides gnutls_x509_crt_list_import,))

AC_CHECK_LIB(gnutls, gnutls_priority_init, gnutls_string_priority=yes, gnutls_string_priority=no)
if test x$gnutls_string_priority = xyes; then
        AC_CHECK_LIB(gnutls, gnutls_priority_set, AC_DEFINE_UNQUOTED(HAVE_GNUTLS_STRING_PRIORITY, , Define whether GnuTLS provide priority parsing),)
fi


LIBS=$old_LIBS
CPPFLAGS=$old_CPPFLAGS

AC_SUBST(PRELUDE_ADMIN_LIBS)
AC_SUBST(PRELUDE_ADMIN_CFLAGS)
PRELUDE_ADMIN_CFLAGS="$LIBGNUTLS_CFLAGS"
PRELUDE_ADMIN_LIBS="$LIBGNUTLS_LIBS $LIBGCRYPT_LIBS"

if test x$srp_available = xno; then
   dnl **************************************************
   dnl * Check for SRP function in GnuTLS extra.        *
   dnl **************************************************
   PKG_CHECK_MODULES([LIBGNUTLS_EXTRA], [gnutls-extra >= $GNUTLS_MIN_VERSION], [gnutls_extra=yes],
                     [AM_PATH_LIBGNUTLS_EXTRA($GNUTLS_MIN_VERSION, gnutls_extra=yes, gnutls_extra=no)])

   if test x$gnutls_extra = xyes; then
        old_LIBS=$LIBS
        LIBS="$LIBGNUTLS_EXTRA_LIBS"
        old_CPPFLAGS=$CPPFLAGS
        CPPFLAGS="$LIBGNUTLS_EXTRA_CFLAGS"

        AC_CHECK_HEADER(gnutls/extra.h, [AC_CHECK_LIB(gnutls-extra, gnutls_srp_verifier, srp_available=yes, srp_available=no)], srp_available=no)
        if test x$srp_available = xyes; then
                PRELUDE_ADMIN_CFLAGS="$PRELUDE_ADMIN_CFLAGS $LIBGNUTLS_EXTRA_CFLAGS"
                PRELUDE_ADMIN_LIBS="$LIBGNUTLS_LIBS $LIBGNUTLS_EXTRA_LIBS $LIBGCRYPT_LIBS"
                AC_DEFINE_UNQUOTED(NEED_GNUTLS_EXTRA, , Define whether SRP support is available through gnutls-extra)
        fi

       LIBS=$old_LIBS
       CPPFLAGS=$old_CPPFLAGS
   fi
fi

if test x$srp_available = xyes; then
	AC_DEFINE_UNQUOTED(GNUTLS_SRP_ENABLED, , Define whether SRP support is enabled)
fi

LIBPRELUDE_LIBS="$LIBPRELUDE_LIBS $LIBGNUTLS_LIBS $LIBGCRYPT_LIBS"


dnl **************************************************
dnl * Check for Ipv6.                                *
dnl **************************************************

AC_CHECK_TYPES([struct sockaddr_in6], have_ipv6_type=yes,, [
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
])

AC_CHECK_DECL(AF_INET6, have_ipv6_def=yes,, [
 #include <sys/types.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
])

if test x$have_ipv6_type = xyes && test x$have_ipv6_def = xyes; then
        AC_DEFINE_UNQUOTED(HAVE_IPV6, , Define whether Ipv6 is available on this system)
fi



dnl **************************************************
dnl * Check for variadic macros                      *
dnl **************************************************
AC_MSG_CHECKING(whether variadic macros are supported)

AC_TRY_COMPILE(
[
#include <stdio.h>
#include <stdarg.h>

#define print_me(...) printf(__VA_ARGS__)
],
[
print_me("test __VA_ARGS__: %s, %d, %d", "a", 0, 1);
return 0;
], have_variadic_macros=yes, have_variadic_macros=no)

if test x$have_variadic_macros = xyes; then
   AC_MSG_RESULT(yes)
   AC_DEFINE_UNQUOTED(HAVE_VARIADIC_MACROS, 1, [Define whether variadic macros are supported])
else
   AC_MSG_RESULT(no)
fi



dnl **************************************************
dnl * Do we need aligned access ? (from tcpdump)     *
dnl **************************************************

AX_CHECK_ALIGNED_ACCESS_REQUIRED
if test "$ax_cv_have_aligned_access_required" = yes; then
        AC_DEFINE_UNQUOTED(PRELUDE_ALIGNED_ACCESS, , Define wether we need to take care with alignment)
fi



dnl ************************************
dnl *   va_copy checks (Thanks Glib!)  *
dnl ************************************
dnl
dnl we currently check for all three va_copy possibilities, so we get
dnl all results in config.log for bug reports.

AC_CACHE_CHECK([for an implementation of va_copy()],prelude_cv_va_copy,[
        AC_LINK_IFELSE([AC_LANG_SOURCE([#include <stdarg.h>
        void f (int i, ...) {
        va_list args1, args2;
        va_start (args1, i);
        va_copy (args2, args1);
        if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
          exit (1);
        va_end (args1); va_end (args2);
        }
        int main() {
          f (0, 42);
          return 0;
        }])],
        [prelude_cv_va_copy=yes],
        [prelude_cv_va_copy=no])
])
AC_CACHE_CHECK([for an implementation of __va_copy()],prelude_cv___va_copy,[
        AC_LINK_IFELSE([AC_LANG_SOURCE([#include <stdarg.h>
        void f (int i, ...) {
        va_list args1, args2;
        va_start (args1, i);
        __va_copy (args2, args1);
        if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
          exit (1);
        va_end (args1); va_end (args2);
        }
        int main() {
          f (0, 42);
          return 0;
        }])],
        [prelude_cv___va_copy=yes],
        [prelude_cv___va_copy=no])
])

if test "x$prelude_cv_va_copy" = "xyes"; then
  g_va_copy_func=va_copy
else if test "x$prelude_cv___va_copy" = "xyes"; then
  g_va_copy_func=__va_copy
fi
fi

if test -n "$g_va_copy_func"; then
  AC_DEFINE_UNQUOTED(PRELUDE_VA_COPY,$g_va_copy_func,[A 'va_copy' style function])
fi

AC_CACHE_CHECK([whether va_lists can be copied by value],prelude_cv_va_val_copy,[
        AC_TRY_RUN([#include <stdarg.h>
        void f (int i, ...) {
        va_list args1, args2;
        va_start (args1, i);
        args2 = args1;
        if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
          exit (1);
        va_end (args1); va_end (args2);
        }
        int main() {
          f (0, 42);
          return 0;
        }],
        [prelude_cv_va_val_copy=yes],
        [prelude_cv_va_val_copy=no],
        [prelude_cv_va_val_copy=yes])
])

if test "x$prelude_cv_va_val_copy" = "xno"; then
  AC_DEFINE(PRELUDE_VA_COPY_AS_ARRAY,1, ['va_lists' cannot be copies as values])
fi


dnl **************************************************
dnl * Swig support                                   *
dnl **************************************************

AC_ARG_WITH(swig, AC_HELP_STRING(--with-swig@<:@=PATH@:>@, Regenerate bindings sources using local SWIG installation @<:@default=auto@:>@),
            [swig_required=true; if test x$withval = xyes; then with_swig="swig"; fi], with_swig="no")

SWIG="no"
if test x$with_swig != xno; then
   AC_PATH_PROG(SWIG, `basename $with_swig`, no, `dirname $with_swig`:$PATH)

   if test x$SWIG = xno; then
      if test x$swig_required = xtrue; then
         AC_MSG_ERROR([Could not find $with_swig binary])
      fi
   fi
fi

AM_CONDITIONAL(HAVE_SWIG, test x$SWIG != xno)


dnl **************************************************
dnl * Perl support                                   *
dnl **************************************************

AC_ARG_WITH(perl, AC_HELP_STRING(--with-perl@<:@=PATH@:>@, Enable support for perl binding @<:@default=auto@:>@),
            [perl_required=true; if test x$withval = xyes; then with_perl="perl"; fi], with_perl="perl")

if test x$with_perl != xno; then

   AC_PATH_PROG(PERL, `basename $with_perl`, no, `dirname $with_perl`:$PATH)

   if test x$PERL = xno; then
      if test x$perl_required = xtrue; then
         AC_MSG_ERROR([Could not find $with_perl binary])
      fi

      with_perl=no
   else
      PERL_CFLAGS="-I`$PERL -e 'use Config; print $Config{archlib}'`/CORE"

      old_CPPFLAGS="$CPPFLAGS"
      CPPFLAGS="$CPPFLAGS $PERL_CFLAGS"

      AC_CHECK_HEADER(EXTERN.h, [AC_CHECK_HEADER(perl.h, with_perl=yes, with_perl=no, [#include <EXTERN.h>])], with_perl=no)
      CPPFLAGS="$old_CPPFLAGS"
   fi
fi

AM_CONDITIONAL(HAVE_PERL, test x$with_perl = xyes)


PERL_INSTALLDIRS="site"

AC_ARG_WITH(perl-installdirs, AC_HELP_STRING(--with-perl-installdirs=@<:@site|vendor@:>@, Specify where to install the Perl module @<:@default=site@:>@),
            PERL_INSTALLDIRS="$withval")

if test x$PERL_INSTALLDIRS != xsite && test x$PERL_INSTALLDIRS != xvendor; then
        AC_MSG_ERROR(Invalid value for --with-perl-installdirs: only "site" or "vendor" supported.)
fi

PERL_EXTRA_PARAMS="INSTALLDIRS=$PERL_INSTALLDIRS"
AC_SUBST(PERL_EXTRA_PARAMS)


dnl **************************************************
dnl * Python support                                 *
dnl **************************************************

AC_ARG_WITH(python2, AC_HELP_STRING(--with-python2@<:@=PATH@:>@, Enable support for python2 binding @<:@default=auto@:>@),
            [python2_required=true; if test x$withval = xyes; then with_python2="python2"; fi], with_python2="python2")

if test x$with_python2 != xno; then
   AC_PATH_PROGS(PYTHON2, $with_python2 "python2.7" "python2.6" "python2.5" "python2.4" "python2.3" "python2.2" "python2.1" "python2.0", no, `dirname $with_python2`:$PATH)
   if test x$PYTHON2 = xno; then
      if test x$python2_required = xtrue; then
         AC_MSG_ERROR([Could not find $with_python2 binary])
      fi

      with_python2=no
   else
      old_CPPFLAGS="$CPPFLAGS"
      CPPFLAGS="-I`$PYTHON2 -c 'from distutils.sysconfig import get_python_inc; print(get_python_inc())'`"

      AC_CHECK_HEADER(Python.h, with_python2=yes, with_python2=no)
      CPPFLAGS="$old_CPPFLAGS"
      AX_RESET_HEADERS_CACHE([Python.h])
   fi
fi

AM_CONDITIONAL(HAVE_PYTHON2, test x$with_python2 = xyes)


AC_ARG_WITH(python3, AC_HELP_STRING(--with-python3@<:@=PATH@:>@, Enable support for python3 binding @<:@default=auto@:>@),
            [python3_required=true; if test x$withval = xyes; then with_python3="python3"; fi], with_python3="python3")

if test x$with_python3 != xno; then
   AC_PATH_PROGS(PYTHON3, $with_python3 "python3.5" "python3.4" "python3.3" "python3.2" "python3.1" "python3.0", no, `dirname $with_python3`:$PATH)
   if test x$PYTHON3 = xno; then
      if test x$python3_required = xtrue; then
         AC_MSG_ERROR([Could not find $with_python3 binary])
      fi

      with_python3=no
   else
      old_CPPFLAGS="$CPPFLAGS"
      CPPFLAGS="-I`$PYTHON3 -c 'from distutils.sysconfig import get_python_inc; print(get_python_inc())'`"

      AC_CHECK_HEADER(Python.h, with_python3=yes, with_python3=no)
      CPPFLAGS="$old_CPPFLAGS"
   fi
fi

AM_CONDITIONAL(HAVE_PYTHON3, test x$with_python3 = xyes)
AM_CONDITIONAL(HAVE_PYTHON, test x$with_python2 = xyes -o x$with_python3 = xyes)

dnl **************************************************
dnl * Ruby support                                   *
dnl **************************************************


AM_PATH_RUBY(,with_ruby=yes, with_ruby=no)
if test x$with_ruby != xno; then
    RUBY_ARCHDIR=`$RUBY -e 'require "rbconfig.rb"; puts RbConfig::expand("\$(archdir)")'`
    RUBY_LIBRUBYARG=`$RUBY -e 'require "rbconfig.rb"; puts RbConfig::expand("\$(LIBRUBYARG)")'`

    RUBY_LIBS="-L$RUBY_ARCHDIR $RUBY_LIBRUBYARG"
    AC_SUBST(RUBY_LIBS)

    RUBY_CCFLAGS=`$RUBY -rrbconfig -e "print RbConfig::CONFIG[['CFLAGS']]"`
    AC_SUBST(RUBY_CCFLAGS)

    old_CPPFLAGS="$CPPFLAGS"
    CPPFLAGS="$RUBY_INCLUDES"
    AC_CHECK_HEADER(ruby.h, with_ruby=yes, with_ruby=no)
    CPPFLAGS="$old_CPPFLAGS"
fi
AM_CONDITIONAL([HAVE_RUBY], [test x$with_ruby = xyes])


dnl **************************************************
dnl * LUA support                                    *
dnl **************************************************


AC_ARG_WITH(lua, AC_HELP_STRING(--with-lua-config@<:@=PATH@:>@, Enable support for lua binding @<:@default=auto@:>@),
            [lua_required=true; if test x$withval = xyes; then with_lua="lua-config"; fi], with_lua="lua-config")

if test x$with_lua != xno; then

   lua_pkg_found=0
   m4_foreach([lua_ver],[[lua],[lua5.1],[lua5.2]],
       [PKG_CHECK_MODULES(LUA, [lua_ver >= 5.1], [
          AC_DEFINE([HAVE_LUA], [1], [liblua])
          AC_DEFINE([HAVE_LUA_H], [1], [lua.h])
          lua_pkg_found=1
       ],[:])]
   )

   if test x$lua_pkg_found = x0; then
       AC_MSG_WARN([*** lua >= 5.1 not found])
   fi

   if test x$lua_pkg_found = x1; then
      AC_SUBST(LUA_CFLAGS)
      AC_SUBST(LUA_LIBS)

      old_CPPFLAGS="$CPPFLAGS"
      CPPFLAGS="$LUA_CFLAGS"
      AC_CHECK_HEADERS([lua.h lauxlib.h], with_lua=yes, [with_lua=no; break])
      CPPFLAGS="$old_CPPFLAGS"

      LUA_VERSION=$(lua -e "print(string.sub(_VERSION, 5))")
      AC_SUBST(LUA_VERSION)

   else
      dnl try lua 5.0
      AC_PATH_PROG(LUA_CONFIG, `basename $with_lua`, no, `dirname $with_lua`:$PATH)
      if test x$LUA_CONFIG = xno; then
         if test x$lua_required = xtrue; then
            AC_MSG_ERROR([Could not find $with_lua binary])
         fi

         with_lua=no
      else
         LUA_CFLAGS=`$LUA_CONFIG --include --vmonly`
         LUA_LIBS=`$LUA_CONFIG --libs --vmonly`
         AC_SUBST(LUA_CFLAGS)
         AC_SUBST(LUA_LIBS)

         old_CPPFLAGS="$CPPFLAGS"
         CPPFLAGS="$LUA_CFLAGS"
         AC_CHECK_HEADERS([lua.h lauxlib.h], with_lua=yes, [with_lua=no; break])
         CPPFLAGS="$old_CPPFLAGS"

         LUA_VERSION=$(lua -e "print(string.sub(_VERSION, 5))")
         AC_SUBST(LUA_VERSION)

      fi
   fi
fi

AM_CONDITIONAL(HAVE_LUA, test x$with_lua = xyes)



dnl **************************************************
dnl * Easy bindings support                          *
dnl **************************************************


AC_ARG_ENABLE(easy-bindings, AC_HELP_STRING(--enable-easy-bindings, Enable support for high level binding @<:@default=yes@:>@),
                enable_easy_bindings=$enableval, enable_easy_bindings="yes")

AM_CONDITIONAL(HAVE_EASY_BINDINGS, test x$enable_easy_bindings = xyes)

if test x$enable_easy_bindings = xyes; then
        AC_CHECK_PROG(HAVE_CXX, $CXX, yes, no)
        if test x$HAVE_CXX != xyes; then
                AC_MSG_ERROR([Easybindings require a C++ compiler, but none were found.])
        fi
fi


dnl **************************************************
dnl * Valgrind support                               *
dnl **************************************************

AC_ARG_WITH(valgrind, AC_HELP_STRING(--with-python@<:@=PATH@:>@, Valgrind support for unit testing @<:@default=auto@:>@),
            [valgrind_required=true; if test x$withval = xyes; then with_valgrind="valgrind"; fi], with_valgrind="valgrind")

if test x$with_valgrind != xno; then

   AC_PATH_PROG(VALGRIND, `basename $with_valgrind`, no, `dirname $with_valgrind`:$PATH)
   if test x$VALGRIND = xno; then
      if test x$valgrind_required = xtrue; then
         AC_MSG_ERROR([Could not find $with_valgrind binary])
      fi

      with_valgrind=no
   else
      with_valgrind=yes
   fi
fi

AM_CONDITIONAL(HAVE_VALGRIND, test x$with_valgrind = xyes)


dnl **************************************************
dnl * Typedefs, structures, compiler characteristics.*
dnl **************************************************

dnl this need to be done before calling GnuLib, since
dnl GnuLib might provide replacement for the different
dnl types

AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_CHECK_TYPES([uid_t, gid_t])
AC_HEADER_TIME
AC_TYPE_LONG_LONG_INT
AC_C_INLINE
AC_SYS_LARGEFILE



dnl **************************************************
dnl * Check for missing function replacement         *
dnl **************************************************
gl_SOURCE_BASE(libmissing)
gl_M4_BASE(libmissing/m4)
gl_INIT


dnl **************************************************
dnl * Check for GTK-DOC                              *
dnl **************************************************

GTK_DOC_CHECK(1.0)


dnl **************************************************
dnl * Required headers / functions.                  *
dnl **************************************************

dnl Needed for FIONREAD under solaris

AC_CHECK_HEADERS_ONCE(sys/filio.h sys/un.h netinet/tcp.h)
AC_CHECK_FUNCS(ftruncate chsize)
AX_CREATE_PRELUDE_INTTYPES_H(src/include/prelude-inttypes.h)



dnl **************************************************
dnl * Setup custom compiler flags                    *
dnl **************************************************

AC_MSG_CHECKING(for cpu-specific compiler flags)
if test "$host_cpu" == "x86_64" && test $enable_static = "yes"; then
        AC_MSG_RESULT(x86_64/static: adding -fPIC)
        AX_CHECK_COMPILE_FLAG(-fPIC, CPPFLAGS="$CPPFLAGS -fPIC")
else
        AC_MSG_RESULT(none needed)
fi

AC_MSG_CHECKING(for platform-specific compiler flags)
case "$host_os" in
 darwin*)
        # It may be called "cc", but it's really a GCC derivative
        # with a problematic special precompiler and precompiled
        # headers; turn off the special precompiler, as some
        # apparently-legal code won't compile with its precompiled
        # headers.
        AC_MSG_RESULT(Darwin: adding -no-cpp-precomp)
        AX_CHECK_COMPILE_FLAG(-no-cpp-precomp, PCFLAGS="$PCFLAGS -no-cpp-precomp")
        ;;
 *)
        AC_MSG_RESULT(none needed)
        ;;
esac


AX_CHECK_LINK_FLAG(-z relro, LDFLAGS="$LDFLAGS -z relro")
AX_CHECK_LINK_FLAG(-z now, LDFLAGS="$LDFLAGS -z now")


for i in -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
         -Wbad-function-cast -Wcast-qual -Wcast-align -Wnested-externs -Wunused \
         -Wformat -Wformat-security; do
        AX_CHECK_COMPILE_FLAG($i, PCFLAGS="$PCFLAGS $i")
done

if test x$buggy_pthread_initializers = xyes; then
   AX_CHECK_COMPILE_FLAG(-Wno-missing-braces, PCFLAGS="$PCFLAGS -Wno-missing-braces")
fi



dnl **************************************************
dnl * Setup build variable                           *
dnl **************************************************

AC_SUBST(CFLAGS)
AC_SUBST(PCFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)
AC_SUBST(BINDINGS_CC)
AC_SUBST(LIBPRELUDE_SONAME)
AC_SUBST(LIBPRELUDECPP_SONAME)
AC_SUBST(LIBPRELUDE_LIBS)
AC_SUBST(LIBPRELUDE_CFLAGS)
AC_SUBST(LIBPRELUDE_VERSION)
AC_SUBST(LIBPRELUDE_MAJOR_VERSION)
AC_SUBST(LIBPRELUDE_MINOR_VERSION)
AC_SUBST(LIBPRELUDE_MICRO_VERSION)
AC_SUBST(LIBPRELUDE_LEVEL_VERSION)
AC_SUBST(LIBPRELUDE_SERIAL_VERSION)
AC_SUBST(PRELUDE_ADMIN_LIBS)
AC_SUBST(PRELUDE_SPOOL_DIR)
AC_SUBST(PRELUDE_CONFIG_DIR)
AC_SUBST(SWIG_DATA_DIR)

AS_AC_EXPAND(SYSCONFDIR, $sysconfdir)
AS_AC_EXPAND(LOCALSTATEDIR, $localstatedir)
AS_AC_EXPAND(LIBDIR, $libdir)
AS_AC_EXPAND(BINDIR, $bindir)
AS_AC_EXPAND(DATADIR, $datadir)

SWIG_DATA_DIR=$DATADIR/libprelude/swig
PRELUDE_SPOOL_DIR=$LOCALSTATEDIR/spool/prelude
PRELUDE_CONFIG_DIR=$SYSCONFDIR/prelude

AC_DEFINE_UNQUOTED(PRELUDE_SPOOL_DIR, "$PRELUDE_SPOOL_DIR", Prelude spool directory)
AC_DEFINE_UNQUOTED(PRELUDE_CONFIG_DIR, "$PRELUDE_CONFIG_DIR", Prelude config directory)

case "${host}" in
        i[[3456789]]86-*-mingw32*) WIN32="yes" ;;
        *cygwin*) WIN32="yes" ;;
        *) WIN32="no" ;;
esac

if test x$WIN32 = xyes; then
        AC_DEFINE_UNQUOTED(INSTALLDIR, "$BINDIR", Prelude library install directory)
else
        AC_DEFINE_UNQUOTED(INSTALLDIR, "$LIBDIR", Prelude library install directory)
fi


AC_CONFIG_FILES([

Makefile
client.conf
global.conf
idmef-client.conf

libprelude-config

libltdl/Makefile

libmissing/Makefile
libmissing/tests/Makefile
m4/Makefile

src/Makefile
src/libprelude.pc
src/libprelude-error/Makefile
src/jsmn/Makefile
src/include/Makefile

src/include/prelude.h
src/include/prelude-inttypes.h

prelude-admin/prelude-adduser
prelude-admin/Makefile

docs/Makefile
docs/api/Makefile
docs/manpages/Makefile

bindings/Makefile
bindings/c++/Makefile
bindings/c++/include/Makefile
bindings/lua/Makefile
bindings/perl/Makefile.PL
bindings/python/Makefile
bindings/python/setup.py
bindings/ruby/Makefile
tests/Makefile

])
AC_CONFIG_COMMANDS([default],[[ chmod +x libprelude-config ]],[[]])
AC_OUTPUT


echo
echo "*** Dumping configuration ***"
echo "    - Generate documentation   : $enable_gtk_doc"
echo "    - LUA binding              : $with_lua"
echo "    - Perl binding             : $with_perl"
echo "    - Python2.x binding        : $with_python2"
echo "    - Python3.x binding        : $with_python3"
echo "    - Ruby binding             : $with_ruby"
echo "    - Easy bindings            : $enable_easy_bindings"
