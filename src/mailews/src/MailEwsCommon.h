#pragma once
#ifndef __MAILEWS_COMMON_H__
#define __MAILEWS_COMMON_H__

#include <memory>

#if __linux__ && __cplusplus >= 201103L
#define auto_ptr unique_ptr
#endif

#include <vector>
#include <string>
#include <map>

#include <sstream>
#include <string>

#define NS_FAILED_WARN(res)	  \
	{ \
		nsresult __rv = res; \
		if (NS_FAILED(__rv)) { \
			NS_ENSURE_SUCCESS_BODY_VOID(res); \
		} \
	}

#endif // __MAILEWS_COMMON_H__
