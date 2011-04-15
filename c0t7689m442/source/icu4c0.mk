NEWLIBDIR:=$(LIBDIR)_c

ifneq ($(ICU4C0),)

LIBDIR:=$(NEWLIBDIR)
CPPFLAGS+=-DUCONFIG_NO_USET -DICU4C0 -DUCONFIG_NO_FORMATTING

# use C compiler
SHLIB.cc=$(SHLIB.c)

INVOKE = $(LDLIBRARYPATH_ENVVAR)=$(LIBRARY_PATH_PREFIX)$(NEWLIBDIR):$(LIBDIR):$(top_builddir)/stubdata:$(top_builddir)/tools/ctestfw:$$$(LDLIBRARYPATH_ENVVAR) $(LEAK_CHECKER)


endif