#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.61])
AC_INIT([libews], [1.5], [])
AC_PROG_LIBTOOL
LT_INIT
AM_INIT_AUTOMAKE([-Wno-unsupported])
AC_CONFIG_SRCDIR([src/libews.h])
AC_CONFIG_MACRO_DIR([m4])
#AC_PROG_RANLIB

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

#CXXFLAGS="${CXXFLAGS} -fPIC -DBUILDING_DLL -DWITH_GZIP -DWITH_ZLIB"
CXXFLAGS="${CXXFLAGS} -fPIC -DBUILDING_DLL"
# Checks for libraries.
#AC_CHECK_LIB(ssl, SSL_library_init, [], [AC_MSG_ERROR('libssl-dev is needed')])
#AC_CHECK_LIB(curl, curl_global_init, [], [AC_MSG_ERROR('libcurl-dev is needed')])

# Checks for header files.

# Host specific
AC_MSG_NOTICE([host:$host])

case $host in
  *linux*) AM_CONDITIONAL([LINUX], [test "x" == x]) ;;
  *) AM_CONDITIONAL([LINUX], [test "x" != x]) ;;
esac

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# argument debug
#enable_debug=no
#with_gsoap_share=/usr/share/gsoap

AC_ARG_ENABLE([debug], [  --enable-debug enable debug build, default is no])
AC_ARG_WITH([gtest], [  --with-gtest, gtest source directory])
AC_ARG_WITH([gsoap-share], [  --with-gsoap-share, gsoap share directory, default is /usr/share/gsoap])
AC_ARG_WITH([install-name], [  --with-install-name, generate with the give install name, usefull on Mac])
AC_ARG_WITH([curl], [  --with-curl, curl source directory])
AC_ARG_WITH([openssl], [  --with-openssl, openssl source directory])

AS_IF([test "x$enable_debug" == xyes],
 [CXXFLAGS="${CXXFLAGS} -DDEBUG -g -O0"],
 [CXXFLAGS="${CXXFLAGS} -DNDEBUG -DWITH_ZLIB -DWITH_GZIP"])
AC_MSG_NOTICE([enable debug mode:$enable_debug])

# ask for gtest
AC_SUBST([GTEST_DIR], [$with_gtest])
AC_SUBST([CURL_DIR], [$with_curl])
AM_CONDITIONAL([HAS_CURL_DIR], [test "x$with_curl" != x])
AS_IF([test "x$CURL_DIR" != x], [AC_MSG_NOTICE([CURL_DIR set to $CURL_DIR])])
AC_SUBST([OPENSSL_DIR], [$with_openssl])
AM_CONDITIONAL([HAS_OPENSSL_DIR], [test "x$with_openssl" != x])
AS_IF([test "x$OPENSSL_DIR" != x], [AC_MSG_NOTICE([OPENSSL_DIR set to $OPENSSL_DIR])])

AS_IF([test "x$with_gsoap_share" == x],
 [GSOAP_SHARE=/usr/share/gsoap],
 [GSOAP_SHARE=$with_gsoap_share])
AC_SUBST([GSOAP_SHARE])
AC_MSG_NOTICE([gsoap share directory:$GSOAP_SHARE])

BUILD_WITH_MY_INSTALL_NAME=no

AS_IF([test "x$with_install_name" != x],
 [BUILD_WITH_MY_INSTALL_NAME=yes;MY_LIB_INSTALL_NAME=$with_install_name;AC_MSG_NOTICE([build with install name:$MY_LIB_INSTALL_NAME])],
 [AC_MSG_NOTICE([build with install name:no])])
AC_SUBST([MY_LIB_INSTALL_NAME])
AM_CONDITIONAL([BUILD_WITH_MY_INSTALL_NAME], [test x${BUILD_WITH_MY_INSTALL_NAME} != xno])

BUILT_WITH_TEST=yes

AS_IF([test "x$GTEST_DIR" == x],
	[BUILD_WITH_TEST=no;AC_MSG_NOTICE([build without test])],
	[AC_MSG_NOTICE([GTEST_DIR set to $GTEST_DIR])])
AM_CONDITIONAL([BUILD_WITH_TEST], [test x${BUILD_WITH_TEST} != xno])

# Checks for library functions.

AC_CONFIG_FILES([Makefile
gsoap/Makefile
ews/Makefile
src/Makefile
src/impl/Makefile
src/utils/Makefile
test/Makefile])
AC_OUTPUT
