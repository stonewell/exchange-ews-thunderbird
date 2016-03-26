noinst_LTLIBRARIES = libews_ws.la libews_ws_client.la libews_ws_server.la

libews_ws_client_la_SOURCES = ewsC.cpp \
	ewsExchangeServiceBindingProxy.h  ewsStub.h \
	ewsExchangeServiceBindingProxy.cpp  ewsH.h
libews_ws_server_la_SOURCES = ewsnH.h \
	ewsnNotificationServiceBindingService.cpp ewsnNotificationServiceBindingService.h \
	ewsnNotificationServiceBinding12Service.cpp ewsnNotificationServiceBinding12Service.h \
	ewsnC.cpp ewsnH.h \
	ewsnStub.h
libews_ws_la_SOURCES =

libews_ws_client_la_CXXFLAGS = -DWITH_OPENSSL -DWITH_COOKIES \
	-fvisibility=hidden  -fvisibility-inlines-hidden \
	-I../gsoap
libews_ws_server_la_CXXFLAGS = -DWITH_OPENSSL -DWITH_COOKIES \
	-fvisibility=hidden  -fvisibility-inlines-hidden \
	-I../gsoap

libews_ws_client_la_LDFLAGS =
libews_ws_server_la_LDFLAGS =

libews_ws_la_LIBADD = libews_ws_client.la libews_ws_server.la

if HAS_OPENSSL_DIR
libews_ws_client_la_LDFLAGS += -L${OPENSSL_DIR}/lib
libews_ws_client_la_CXXFLAGS += -I${OPENSSL_DIR}/include
libews_ws_server_la_LDFLAGS += -L${OPENSSL_DIR}/lib
libews_ws_server_la_CXXFLAGS += -I${OPENSSL_DIR}/include
### libews_ws_la_LDFLAGS += -L${OPENSSL_DIR}/lib
### libews_ws_la_CXXFLAGS += -I${OPENSSL_DIR}/include
endif

if HAS_CURL_DIR
### libews_ws_la_LDFLAGS += -L${CURL_DIR}/lib
### libews_ws_la_CXXFLAGS += -I${CURL_DIR}/include
libews_ws_client_la_LDFLAGS += -L${CURL_DIR}/lib
libews_ws_client_la_CXXFLAGS += -I${CURL_DIR}/include
libews_ws_server_la_LDFLAGS += -L${CURL_DIR}/lib
libews_ws_server_la_CXXFLAGS += -I${CURL_DIR}/include
endif

EXTRA_DIST = Services.wsdl types.xsd messages.xsd \
	NotificationService.wsdl \
	ewsn.nsmap ews.nsmap ewsnC.cpp
BUILT_SOURCES = Services.h ewsExchangeServiceBindingProxy.cpp \
	NotificationService.h ewsnNotificationServiceBindingService.cpp

Services.h: Services.wsdl
	wsdl2h -k -news Services.wsdl

ewsExchangeServiceBindingProxy.cpp: Services.h
	soapcpp2 Services.h -I$(GSOAP_SHARE)/import:$(GSOAP_SHARE) -n -pews -C -i -x

NotificationService.h: NotificationService.wsdl
	wsdl2h -k -newsn -qewsn -Newsn NotificationService.wsdl

ewsnNotificationServiceBindingService.cpp: NotificationService.h
	soapcpp2 NotificationService.h -I$(GSOAP_SHARE)/import:$(GSOAP_SHARE) -n -pewsn -S -i -x

clean-gen:
	-rm -f ews*
	-rm -f Services.h
	-rm -f NotificationService.h
	-rm -f *.nsmap
