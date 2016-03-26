noinst_LTLIBRARIES = libews_impl.la
libews_impl_la_SOURCES = ews_session_gsoap_impl.cpp \
	ews_folder_gsoap_impl.cpp \
	ews_item_gsoap_impl.cpp \
	ews_item_message_gsoap_impl.cpp \
	libews_gsoap.h \
	ews_error_impl.cpp \
	ews_attachment_gsoap_impl.cpp \
	libews_gsoap_utils.cpp libews_gsoap_utils.h \
	ews_recipient_gsoap_impl.cpp \
	ews_proxyinfo_impl.cpp \
	ews_subscription_gsoap_impl.cpp \
	ews_push_notification_server.cpp \
	ews_item_contact_gsoap_impl.cpp \
	ews_item_calendar_gsoap_impl.cpp \
	ews_item_task_gsoap_impl.cpp

libews_impl_la_CXXFLAGS = -I${srcdir}/.. -I${srcdir}/../../ews $(GSOAP_SSL_PLUSPLUS_CFLAGS) \
	-I${srcdir}/../utils \
	-I${srcdir}/../../gsoap \
	-DWITH_DOM -DWITH_OPENSSL -DWITH_COOKIES \
	-fvisibility=hidden  -fvisibility-inlines-hidden

libews_impl_la_LDFLAGS =

if HAS_OPENSSL_DIR
libews_impl_la_LDFLAGS += -L${OPENSSL_DIR}/lib
libews_impl_la_CXXFLAGS += -I${OPENSSL_DIR}/include
endif

if HAS_CURL_DIR
libews_impl_la_LDFLAGS += -L${CURL_DIR}/lib
libews_impl_la_CXXFLAGS += -I${CURL_DIR}/include
endif
