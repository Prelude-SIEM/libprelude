includedir = $(prefix)/include/libprelude

EXTRA_DIST = mkstrtable.awk err-sources.h.in err-codes.h.in \
	mkerrnos.awk errnos.in mkerrcodes.awk mkerrcodes1.awk mkerrcodes2.awk mkerrcodes.c \
	mkheader.awk prelude-error.h.in err-sources.h err-codes.h

BUILT_SOURCES = err-sources.h err-codes.h code-from-errno.h code-to-errno.h err-sources-sym.h err-codes-sym.h errnos-sym.h prelude-error.h 

nodist_include_HEADERS = prelude-error.h

noinst_LTLIBRARIES = libprelude-error.la
libprelude_error_la_SOURCES = prelude-error.h gettext.h strsource.c strerror.c code-to-errno.c code-from-errno.c

err-sources.h: Makefile mkstrtable.awk err-sources.h.in
	$(AWK) -f $(srcdir)/mkstrtable.awk -v textidx=3 \
		$(srcdir)/err-sources.h.in >$@

err-sources-sym.h: Makefile mkstrtable.awk err-sources.h.in
	$(AWK) -f $(srcdir)/mkstrtable.awk -v textidx=2 -v nogettext=1 \
	$(srcdir)/err-sources.h.in >$@

err-codes.h: Makefile mkstrtable.awk err-codes.h.in
	$(AWK) -f $(srcdir)/mkstrtable.awk -v textidx=3 \
	$(srcdir)/err-codes.h.in >$@

err-codes-sym.h: Makefile mkstrtable.awk err-codes.h.in
	$(AWK) -f $(srcdir)/mkstrtable.awk -v textidx=2 -v nogettext=1 \
		$(srcdir)/err-codes.h.in >$@

code-to-errno.h: Makefile mkerrnos.awk errnos.in
	$(AWK) -f $(srcdir)/mkerrnos.awk $(srcdir)/errnos.in >$@


mkerrcodes.h: Makefile mkerrcodes.awk
	$(AWK) -f $(srcdir)/mkerrcodes1.awk $(srcdir)/errnos.in >_$@
	$(CPP) _$@ | grep PRELUDE_ERROR_ | $(AWK) -f $(srcdir)/mkerrcodes.awk >$@
	-rm _$@

# It is correct to use $(CC_FOR_BUILD) here.  We want to run the
# program at build time.
mkerrcodes: mkerrcodes.c mkerrcodes.h Makefile
	$(CC) -I. -I$(srcdir) -o $@ $(srcdir)/mkerrcodes.c

code-from-errno.h: mkerrcodes Makefile
	./mkerrcodes | $(AWK) -f $(srcdir)/mkerrcodes2.awk >$@

errnos-sym.h: Makefile mkstrtable.awk errnos.in
	$(AWK) -f $(srcdir)/mkstrtable.awk -v textidx=2 -v nogettext=1 \
	-v prefix=GPG_ERR_ -v namespace=errnos_ \
	$(srcdir)/errnos.in >$@

prelude-error.h: Makefile mkheader.awk err-sources.h.in err-codes.h.in errnos.in prelude-error.h.in
	$(AWK) -f $(srcdir)/mkheader.awk \
		$(srcdir)/err-sources.h.in \
		$(srcdir)/err-codes.h.in \
		$(srcdir)/errnos.in \
		$(srcdir)/prelude-error.h.in > $@

