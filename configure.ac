dnl Process this file with autoconf to produce a configure script.

AC_INIT(src)
AM_INIT_AUTOMAKE(libprelude, 0.4.1)
AM_CONFIG_HEADER(config.h)
AM_DISABLE_STATIC

AM_MAINTAINER_MODE

AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL

dnl Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_C_INLINE
AC_C_BIGENDIAN

AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LDFLAGS)


dnl **************************************************
dnl * Check for getopt_long                          *
dnl **************************************************

AC_CHECK_HEADERS(getopt.h, getopt_long_hdr=yes)
AC_CHECK_LIB(c, getopt_long, getopt_long_func=yes)

if test x$getopt_long_hdr = xyes && test x$getopt_long_func = xyes; then
	AC_DEFINE(HAVE_GETOPT_LONG)
fi


dnl **************************************************
dnl * Check for demand loading of shared lib library *
dnl **************************************************
AC_CHECK_LIB(dl, dlopen, LIBS="-ldl")



dnl **************************************************
dnl * Check for crypt library                        *
dnl **************************************************
AC_CHECK_LIB(crypt, crypt, cryptlib=true)

if test x$cryptlib = xtrue; then
	AC_DEFINE(HAVE_CRYPT)
	LIBS="$LIBS -lcrypt";
fi



dnl **************************************************
dnl * Check for Pthread library                      *
dnl **************************************************

AC_CHECK_LIB(pthread, pthread_create, pthrlib="yes")
if test x$pthrlib == xyes; then
	PTHREAD_LIBS="-lpthread"
else
	AC_CHECK_LIB(c_r, pthread_create, PTHREAD_LIBS="-lc_r")
fi

AC_SUBST(PTHREAD_LIBS)



dnl **************************************************
dnl * Check for OpenSSL.                             *
dnl **************************************************

AC_CHECK_HEADER(openssl/ssl.h, openssl_header=yes)

AC_ARG_ENABLE(openssl, [  --enable-openssl Use OpenSSL to communicate with Report Server [default=auto]], \
	      enable_openssl="$enableval", enable_openssl=auto)

if test x$enable_openssl = xauto ; then
	if test x$openssl_header = xyes; then
		LIBS="$LIBS -lssl -lcrypto"
		AC_DEFINE(HAVE_SSL)
    		enable_openssl=yes
  	else
    		enable_openssl=no
  	fi
fi


dnl **************************************************
dnl * Check for GTK-DOC                              *
dnl **************************************************

AC_ARG_WITH(html-dir, [  --with-html-dir=PATH path to installed docs ])

if test "x$with_html_dir" = "x" ; then
	HTML_DIR='${datadir}/gtk-doc/html';
else
	HTML_DIR=$with_html_dir;
fi

AC_SUBST(HTML_DIR)
AC_CHECK_PROG(GTKDOC, gtkdoc-mkdb, true, false)

gtk_doc_min_version=0.6

if $GTKDOC; then 
	gtk_doc_version=`gtkdoc-mkdb --version`
	AC_MSG_CHECKING([gtk-doc version ($gtk_doc_version) >= $gtk_doc_min_version])

    	if perl << EOF ; then
      		exit (("$gtk_doc_version" =~ /^[[0-9]]+\.[[0-9]]+$/) && ("$gtk_doc_version" >= "$gtk_doc_min_version") ? 0 : 1); 
EOF
      		AC_MSG_RESULT(yes);
   	else
		echo "set to false"
      		AC_MSG_RESULT(no);
      		GTKDOC=false;
   	fi
fi


dnl Let people disable the gtk-doc stuff.
AC_ARG_ENABLE(gtk-doc, [  --enable-gtk-doc  Use gtk-doc to build documentation [default=auto]], enable_gtk_doc="$enableval", enable_gtk_doc=auto)

if test x$enable_gtk_doc = xauto; then
  	if test x$GTKDOC = xtrue; then
    		enable_gtk_doc=yes;
  	else
    		enable_gtk_doc=no;
  	fi
fi

AM_CONDITIONAL(ENABLE_GTK_DOC, test x$enable_gtk_doc = xyes)



dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_TYPE_SIGNAL
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(gettimeofday select socket strdup strerror strstr)

configdir=$sysconfdir/prelude
plugindir=$libdir/prelude

AC_SUBST(configdir)
AC_SUBST(LIBS)

CFLAGS="$CFLAGS -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
-Wbad-function-cast -Wcast-qual -Wcast-align -Wnested-externs"


AC_OUTPUT([

Makefile 
libprelude-config

docs/Makefile
docs/api/Makefile

libmissing/Makefile
sensor-adduser/Makefile

src/Makefile 
src/include/Makefile

], [ chmod +x libprelude-config ])


echo
echo "*** Dumping configuration ***"
echo "    - Use OpenSSL   : $enable_openssl"












