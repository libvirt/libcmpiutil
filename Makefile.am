# Copyright IBM Corp. 2007
AUTOMAKE_OPTIONS=dist-bzip2

SUBDIRS = tools

EXTRA_DIST = libcmpiutil.spec.in libcmpiutil.spec COPYING		\
	     libcmpiutil.pc.in libcmpiutil.pc				\
	     doc/doxygen.conf doc/mainpage doc/SubmittingPatches README

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcmpiutil.pc

libcmpiutilincdir = $(includedir)/libcmpiutil

noinst_HEADERS = eo_parser_xml.h

libcmpiutilinc_HEADERS = libcmpiutil.h \
                  std_invokemethod.h \
                  std_association.h \
                  std_indication.h \
                  std_instance.h

libcmpiutil_la_SOURCES = args_util.c instance_util.c std_invokemethod.c \
                         std_association.c inst_list.c std_indication.c \
                         debug_util.c eo_parser_xml.c
libcmpiutil_la_CFLAGS = $(CFLAGS) $(CFLAGS_STRICT)
libcmpiutil_la_LIBADD = -lxml2
libcmpiutil_la_DEPENDENCIES =

AM_LFLAGS = -Peo_parse_
AM_YFLAGS = -d -p eo_parse_

if build_eoparser
LEX_OUTPUT_ROOT = lex.eo_parse_
libcueoparser_la_SOURCES = eo_util_parser.y eo_util_lexer.l eo_parser.c
libcmpiutil_la_DEPENDENCIES += libcueoparser.la
libcmpiutil_la_LIBADD += -lcueoparser
EOPARSER = libcueoparser.la
BUILT_SOURCES = eo_util_lexer.c eo_util_parser.c eo_util_parser.h
endif

lib_LTLIBRARIES = $(EOPARSER) libcmpiutil.la 

docs:
if found_doxygen
	doxygen doc/doxygen.conf
else
	@echo Cannot build documentation without doxygen!
endif

clean-local:
	rm -f $(BUILT_SOURCES) *~

rpm: clean
	@(unset CDPATH ; chmod 0644 libcmpiutil.spec ; $(MAKE) dist && rpmbuild -ta $(distdir).tar.gz)

