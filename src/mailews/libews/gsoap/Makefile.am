noinst_LTLIBRARIES = libgsoap.la
nodist_libgsoap_la_SOURCES = stdsoap2.cpp \
	stdsoap2.h

libgsoap_la_CXXFLAGS = -DWITH_OPENSSL -DWITH_COOKIES \
	-DWITH_NONAMESPACES \
	-fvisibility=hidden  -fvisibility-inlines-hidden

libgsoap_la_LDFLAGS =

if HAS_OPENSSL_DIR
libgsoap_la_LDFLAGS += -L${OPENSSL_DIR}/lib
libgsoap_la_CXXFLAGS += -I${OPENSSL_DIR}/include
endif

if HAS_CURL_DIR
libgsoap_la_LDFLAGS += -L${CURL_DIR}/lib
libgsoap_la_CXXFLAGS += -I${CURL_DIR}/include
endif

