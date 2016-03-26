if BUILD_WITH_TEST
bin_PROGRAMS = test_libews
test_libews_SOURCES = test_libews.cpp
nodist_test_libews_SOURCES = $(GTEST_DIR)/src/gtest-all.cc 

test_libews_CXXFLAGS = -I$(srcdir)/../src \
	-isystem $(GTEST_DIR)/include \
	-I$(GTEST_DIR) \
	-g -Wall -Wextra

test_libews_LDFLAGS = -lpthread
test_libews_LDADD = ../src/libews.la
endif
