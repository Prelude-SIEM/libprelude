AC_DEFUN(AC_REAL_PATH_GENERIC,
[
dnl
dnl we're going to need uppercase, lowercase and user-friendly versions of the
dnl string `LIBRARY'
pushdef([UP], translit([$1], [a-z], [A-Z]))dnl
pushdef([DOWN], translit([$1], [A-Z], [a-z]))dnl

dnl
dnl Get the cflags and libraries from the LIBRARY-config script
dnl
AC_ARG_WITH(DOWN-prefix,[  --with-]DOWN[-prefix=PFX       Prefix where $1 is installed (optional)],
        DOWN[]_config_prefix="$withval", DOWN[]_config_prefix="")

AC_ARG_WITH(DOWN-exec-prefix,[  --with-]DOWN[-exec-prefix=PFX Exec prefix where $1 is installed (optional)],
        DOWN[]_config_exec_prefix="$withval", DOWN[]_config_exec_prefix="")

  custom_path=""


  if test x$DOWN[]_config_exec_prefix != x ; then

     if test x$3 != x; then
     	DOWN[]_config_args="$DOWN[]_config_args --exec-prefix=$DOWN[]_config_exec_prefix"
     fi

     if test x${UP[]_CONFIG+set} != xset ; then
       custom_path=$DOWN[]_config_prefix/bin:
       dnl UP[]_CONFIG=$DOWN[]_config_exec_prefix/bin/DOWN-config
     fi
  fi

  if test x$DOWN[]_config_prefix != x ; then

     if test x$3 != x; then
     	DOWN[]_config_args="$DOWN[]_config_args --prefix=$DOWN[]_config_prefix"
     fi

     if test x${UP[]_CONFIG+set} != xset ; then
	custom_path=$DOWN[]_config_prefix/bin:
        dnl UP[]_CONFIG=$DOWN[]_config_prefix/bin/DOWN-config
     fi
  fi

  AC_PATH_PROG(UP[]_CONFIG, $2, no, $custom_path$PATH)

  ifelse([$6], ,
     AC_MSG_CHECKING(for $1),
     AC_MSG_CHECKING(for $1 - version >= $6)
  )
  no_[]DOWN=""
  if test "$UP[]_CONFIG" = "no" ; then
     no_[]DOWN=yes
  else
     UP[]_CFLAGS="`$UP[]_CONFIG $DOWN[]_config_args --$4`"
     UP[]_LIBS="`$UP[]_CONFIG $DOWN[]_config_args --$5`"

     ifelse([$6], , ,[
        DOWN[]_config_major_version=`$UP[]_CONFIG $DOWN[]_config_args \
         --version | sed 's/[[^0-9]]*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
        DOWN[]_config_minor_version=`$UP[]_CONFIG $DOWN[]_config_args \
         --version | sed 's/[[^0-9]]*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
        DOWN[]_config_micro_version=`$UP[]_CONFIG $DOWN[]_config_args \
         --version | sed 's/[[^0-9]]*\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
        DOWN[]_wanted_major_version="regexp($6, [\<\([0-9]*\)], [\1])"
        DOWN[]_wanted_minor_version="regexp($6, [\<\([0-9]*\)\.\([0-9]*\)], [\2])"
        DOWN[]_wanted_micro_version="regexp($6, [\<\([0-9]*\).\([0-9]*\).\([0-9]*\)], [\3])"

        # Compare wanted version to what config script returned.
        # If I knew what library was being run, i'd probably also compile
        # a test program at this point (which also extracted and tested
        # the version in some library-specific way)
        if test "$DOWN[]_config_major_version" -lt \
                        "$DOWN[]_wanted_major_version" \
          -o \( "$DOWN[]_config_major_version" -eq \
                        "$DOWN[]_wanted_major_version" \
            -a "$DOWN[]_config_minor_version" -lt \
                        "$DOWN[]_wanted_minor_version" \) \
          -o \( "$DOWN[]_config_major_version" -eq \
                        "$DOWN[]_wanted_major_version" \
            -a "$DOWN[]_config_minor_version" -eq \
                        "$DOWN[]_wanted_minor_version" \
            -a "$DOWN[]_config_micro_version" -lt \
                        "$DOWN[]_wanted_micro_version" \) ; then
          # older version found
          no_[]DOWN=yes
          echo -n "*** An old version of $1 "
          echo -n "($DOWN[]_config_major_version"
          echo -n ".$DOWN[]_config_minor_version"
          echo    ".$DOWN[]_config_micro_version) was found."
          echo -n "*** You need a version of $1 newer than "
          echo -n "$DOWN[]_wanted_major_version"
          echo -n ".$DOWN[]_wanted_minor_version"
          echo    ".$DOWN[]_wanted_micro_version."
          echo "***"
          echo "*** If you have already installed a sufficiently new version, this error"
          echo "*** probably means that the wrong copy of the $2 shell script is"
          echo "*** being found. The easiest way to fix this is to remove the old version"
          echo "*** of $1, but you can also set the UP[]_CONFIG environment to point to the"
          echo "*** correct copy of $2. (In this case, you will have to"
          echo "*** modify your LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf"
          echo "*** so that the correct libraries are found at run-time)"
        fi
     ])
  fi
  if test "x$no_[]DOWN" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$7], , :, [$7])
  else
     AC_MSG_RESULT(no)
     if test "$UP[]_CONFIG" = "no" ; then
       echo "*** The $2 script installed by $1 could not be found"
       echo "*** If $1 was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the UP[]_CONFIG environment variable to the"
       echo "*** full path to $2."
     fi
     UP[]_CFLAGS=""
     UP[]_LIBS=""
     ifelse([$8], , :, [$8])
  fi
  AC_SUBST(UP[]_CFLAGS)
  AC_SUBST(UP[]_LIBS)
  
  popdef([UP])
  popdef([DOWN])

])



AC_DEFUN(AC_PATH_GENERIC,
[
	pushdef([DOWN], translit([$1], [A-Z], [a-z]))
	AC_REAL_PATH_GENERIC($1, DOWN-config, prefix_supported, cflags, libs, $2, $3, $4)
])



#
# Check for the given datatype
#
# AC_DATATYPE_GENERIC(headers, type, replacements)

AC_DEFUN([AC_DATATYPE_GENERIC], 
[

AC_CHECK_TYPE([$2], , [

	AC_MSG_CHECKING([for $2 equivalent])
	s_replacement=""

	for t in $3; do
		AC_TRY_COMPILE([$1], [$t type;], s_replacement="$t" break)
	done

	if test "x$s_replacement" = x; then
		AC_MSG_ERROR([Cannot find a type to use in place of $2])
      	fi

	AC_MSG_RESULT($s_replacement)
	
	AC_DEFINE_UNQUOTED($2, [$s_replacement],
                           [type to use in place of $2 if not defined])

], [$1])
])

