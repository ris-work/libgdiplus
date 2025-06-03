ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src tests
DIST_SUBDIRS = src tests

pkgconfigdir = $(libdir)/pkgconfig

# Add HarfBuzz include flags to your compiler flags
AM_CPPFLAGS = $(HARFBUZZ_CFLAGS) $(AM_CPPFLAGS)

# Append HarfBuzz libraries to the linker flags.
# If you're building a library (say, libgdiplus.la),
# ensure its link command includes $(HARFBUZZ_LIBS). For example:
libgdiplus_la_LIBADD = $(HARFBUZZ_LIBS) $(LDADD)


pkgconfig_DATA= libgdiplus.pc

DISTCLEANFILES= libgdiplus.pc

EXTRA_DIST = libgdiplus.pc.in README.md LICENSE libgdiplus.sln winconfig.h.in update_submodules.sh external/googletest

all: update_submodules

update_submodules:
	@cd $(top_srcdir) && ./update_submodules.sh

.PHONY: update_submodules
