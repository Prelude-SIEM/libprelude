dnl @synopsis AC_PROTOTYPE(function, includes, code, TAG1, values1 [, TAG2, values2 [...]])
dnl
dnl Try all the combinations of <TAG1>, <TAG2>... to successfully compile <code>.
dnl <TAG1>, <TAG2>, ... are substituted in <code> and <include> with values found in
dnl <values1>, <values2>, ... respectively. <values1>, <values2>, ... contain a list of
dnl possible values for each corresponding tag and all combinations are tested.
dnl When AC_TRY_COMPILE(include, code) is successfull for a given substitution, the macro
dnl stops and defines the following macros: FUNCTION_TAG1, FUNCTION_TAG2, ... using AC_DEFINE()
dnl with values set to the current values of <TAG1>, <TAG2>, ...
dnl If no combination is successfull the configure script is aborted with a message.
dnl
dnl Intended purpose is to find which combination of argument types is acceptable for a
dnl given function <function>. It is recommended to list the most specific types first.
dnl For instance ARG1, [size_t, int] instead of ARG1, [int, size_t].
dnl
dnl Generic usage pattern:
dnl
dnl 1) add a call in configure.in
dnl
dnl  AC_PROTOTYPE(...)
dnl
dnl 2) call autoheader to see which symbols are not covered
dnl
dnl 3) add the lines in acconfig.h
dnl
dnl  /* Type of Nth argument of function */
dnl  #undef FUNCTION_ARGN
dnl
dnl 4) Within the code use FUNCTION_ARGN instead of an hardwired type
dnl
dnl Complete example:
dnl
dnl 1) configure.in
dnl
dnl  AC_PROTOTYPE(getpeername,
dnl  [
dnl   #include <sys/types.h>
dnl   #include <sys/socket.h>
dnl  ],
dnl  [
dnl   int a = 0;
dnl   ARG2 * b = 0;
dnl   ARG3 * c = 0;
dnl   getpeername(a, b, c);
dnl  ],
dnl  ARG2, [struct sockaddr, void],
dnl  ARG3, [socklen_t, size_t, int, unsigned int, long unsigned int])
dnl
dnl 2) call autoheader
dnl
dnl  autoheader: Symbol `GETPEERNAME_ARG2' is not covered by ./acconfig.h
dnl  autoheader: Symbol `GETPEERNAME_ARG3' is not covered by ./acconfig.h
dnl
dnl 3) acconfig.h
dnl
dnl  /* Type of second argument of getpeername */
dnl  #undef GETPEERNAME_ARG2
dnl
dnl  /* Type of third argument of getpeername */
dnl  #undef GETPEERNAME_ARG3
dnl
dnl 4) in the code
dnl      ...
dnl      GETPEERNAME_ARG2 name;
dnl      GETPEERNAME_ARG3 namelen;
dnl      ...
dnl      ret = getpeername(socket, &name, &namelen);
dnl      ...
dnl
dnl Implementation notes: generating all possible permutations of
dnl the arguments is not easily done with the usual mixture of shell and m4,
dnl that is why this macro is almost 100% m4 code. It generates long but simple
dnl to read code.
dnl
dnl @version $Id: ac_prototype.m4,v 1.1.1.1 2001/07/26 00:46:29 guidod Exp $
dnl @author Loic Dachary <loic@senga.org>
dnl

AC_DEFUN([AC_PROTOTYPE],[

AX_CFLAGS_GCC_OPTION(-Werror, ERROR_CPPFLAG)

dnl
dnl Upper case function name
dnl
 pushdef([function],translit([$1], [a-z], [A-Z]))
dnl
dnl Collect tags that will be substituted
dnl
 pushdef([tags],[AC_PROTOTYPE_TAGS(builtin([shift],builtin([shift],builtin([shift],$@))))])
dnl
dnl Wrap in a 1 time loop, when a combination is found break to stop the combinatory exploration
dnl
 for i in 1
 do
   AC_PROTOTYPE_LOOP(AC_PROTOTYPE_REVERSE($1, AC_PROTOTYPE_SUBST($2,tags),AC_PROTOTYPE_SUBST($3,tags),builtin([shift],builtin([shift],builtin([shift],$@)))))
   AC_MSG_ERROR($1 unable to find a working combination)
 done
 popdef([tags])
 popdef([function])
])

dnl
dnl AC_PROTOTYPE_REVERSE(list)
dnl
dnl Reverse the order of the <list>
dnl
AC_DEFUN([AC_PROTOTYPE_REVERSE],[ifelse($#,0,,$#,1,[[$1]],[AC_PROTOTYPE_REVERSE(builtin([shift],$@)),[$1]])])

dnl
dnl AC_PROTOTYPE_SUBST(string, tag)
dnl
dnl Substitute all occurence of <tag> in <string> with <tag>_VAL.
dnl Assumes that tag_VAL is a macro containing the value associated to tag.
dnl
AC_DEFUN([AC_PROTOTYPE_SUBST],[ifelse($2,,[$1],[AC_PROTOTYPE_SUBST(patsubst([$1],[$2],[$2[]_VAL]),builtin([shift],builtin([shift],$@)))])])

dnl
dnl AC_PROTOTYPE_TAGS([tag, values, [tag, values ...]])
dnl
dnl Generate a list of <tag> by skipping <values>.
dnl
AC_DEFUN([AC_PROTOTYPE_TAGS],[ifelse($1,,[],[$1, AC_PROTOTYPE_TAGS(builtin([shift],builtin([shift],$@)))])])

dnl
dnl AC_PROTOTYPE_DEFINES(tags)
dnl
dnl Generate a AC_DEFINE(function_tag, tag_VAL) for each tag in <tags> list
dnl Assumes that function is a macro containing the name of the function in upper case
dnl and that tag_VAL is a macro containing the value associated to tag.
dnl
AC_DEFUN([AC_PROTOTYPE_DEFINES],[ifelse($1,,[],[AC_DEFINE(function[]_$1, $1_VAL, description) AC_PROTOTYPE_DEFINES(builtin([shift],$@))])])

dnl
dnl AC_PROTOTYPE_STATUS(tags)
dnl
dnl Generates a message suitable for argument to AC_MSG_* macros. For each tag
dnl in the <tags> list the message tag => tag_VAL is generated.
dnl Assumes that tag_VAL is a macro containing the value associated to tag.
dnl
AC_DEFUN([AC_PROTOTYPE_STATUS],[ifelse($1,,[],[$1 => $1_VAL AC_PROTOTYPE_STATUS(builtin([shift],$@))])])

dnl
dnl AC_PROTOTYPE_EACH(tag, values)
dnl
dnl Call AC_PROTOTYPE_LOOP for each values and define the macro tag_VAL to
dnl the current value.
dnl
AC_DEFUN([AC_PROTOTYPE_EACH],[
  ifelse($2,, [
  ], [
    pushdef([$1_VAL], $2)
    AC_PROTOTYPE_LOOP(rest)
    popdef([$1_VAL])
    AC_PROTOTYPE_EACH($1, builtin([shift], builtin([shift], $@)))
  ])
])

dnl
dnl AC_PROTOTYPE_LOOP([tag, values, [tag, values ...]], code, include, function)
dnl
dnl If there is a tag/values pair, call AC_PROTOTYPE_EACH with it.
dnl If there is no tag/values pair left, tries to compile the code and include
dnl using AC_TRY_COMPILE. If it compiles, AC_DEFINE all the tags to their
dnl current value and exit with success.
dnl
AC_DEFUN([AC_PROTOTYPE_LOOP],[
 ifelse(builtin([eval], $# > 3), 1,
   [
     pushdef([rest],[builtin([shift],builtin([shift],$@))])
     AC_PROTOTYPE_EACH($2,$1)
     popdef([rest])
   ], [
     AC_MSG_CHECKING($3 AC_PROTOTYPE_STATUS(tags))
dnl
dnl Activate fatal warnings if possible, gives better guess
dnl
     ac_save_CPPFLAGS="$CPPFLAGS"
     CPPFLAGS="$CPPFLAGS $ERROR_CPPFLAG"

     AC_TRY_COMPILE($2, $1, [
      CPPFLAGS="$ac_save_CPPFLAGS"
      AC_MSG_RESULT(ok)
      AC_PROTOTYPE_DEFINES(tags)
      break;
     ], [
      CPPFLAGS="$ac_save_CPPFLAGS"
      AC_MSG_RESULT(not ok)
     ])
   ]
 )
])
