SUBDIRS = utils impl

lib_LTLIBRARIES = libews.la

headersdir=${includedir}/ews
headers_DATA = libews.h

libews_la_SOURCES = libews.h \
	libews.cpp \
	libews_c.cpp \
	libews_c_folder.cpp \
	libews_c_item.cpp \
	libews_c_attachment.cpp \
	libews_c_autodiscover.cpp \
	libews_c_subscribe.cpp

libews_la_CXXFLAGS = -I${srcdir}/../ews \
	-DWITH_DOM -DWITH_OPENSSL -DWITH_COOKIES

libews_la_LDFLAGS =

if HAS_OPENSSL_DIR
libews_la_LDFLAGS += -L${OPENSSL_DIR}/lib
libews_la_CXXFLAGS += -I${OPENSSL_DIR}/include
endif

if HAS_CURL_DIR
libews_la_LDFLAGS += -L${CURL_DIR}/lib
libews_la_CXXFLAGS += -I${CURL_DIR}/include
endif

if BUILD_WITH_MY_INSTALL_NAME
libews_la_LDFLAGS += -Wl,-install_name,${MY_LIB_INSTALL_NAME}
endif

if LINUX
EXTRA_LIBS = -Wl,-Bstatic,-lcurl,-Bdynamic \
	-Wl,-Bstatic,-lssl,-Bdynamic \
	-Wl,-Bstatic,-lcrypto,-Bdynamic
else
EXTRA_LIBS = -lcurl -lssl -lcrypto
endif

libews_la_LDFLAGS += -avoid-version \
	-no-undefined \
	-fvisibility=hidden -fvisibility-inlines-hidden \
	-lz -ldl -lpthread \
	${EXTRA_LIBS}

libews_la_LIBADD = ../ews/libews_ws.la \
	../gsoap/libgsoap.la \
	impl/libews_impl.la \
	utils/libews_utils.la
