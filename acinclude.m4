#
# Check for the given datatype
#
# AC_DATATYPE_GENERIC(headers, type, replacements)

AC_DEFUN([AC_DATATYPE_GENERIC], 
[

AC_CHECK_TYPE([$2], ,[

 	AC_MSG_CHECKING([for $2 equivalent])
	s_replacement=""

	for t in $3; do
		AC_TRY_COMPILE([$1], [$t type;], s_replacement="$t" break)
	done

	if test "x$s_replacement" = x; then
		AC_MSG_ERROR([Cannot find a type to use in place of $2])
      	fi

	AC_MSG_RESULT($s_replacement)

	AC_DEFINE_UNQUOTED($2, $s_replacement,
                           [type to use in place of $2 if not defined])],,)
 ])
])

