utilsheadersdir=${includedir}/ews/utils
utilsheaders_DATA = ews_list_stl.h \
	ews_string_stl.h

noinst_LTLIBRARIES = libews_utils.la
libews_utils_la_CXXFLAGS = -I${srcdir}/../../ews -I${srcdir}/../../utils/src -I${srcdir}/.. \
	-I${srcdir}/../../gsoap \
	-fvisibility=hidden  -fvisibility-inlines-hidden
libews_utils_la_CFLAGS = -I${srcdir}/../../ews -I${srcdir}/../../utils/src -I${srcdir}/.. \
	-I${srcdir}/../../gsoap \
	-fvisibility=hidden
libews_utils_la_SOURCES = ews_list_stl.h \
	ews_string_stl.h ews_string_stl.cpp \
	curl_help.cpp curl_help.h \
	duration.cpp duration.h \
	urlparser.cpp urlparser.h \
	Event.h
