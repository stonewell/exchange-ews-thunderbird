#pragma once
#ifndef __MAIL_EWS_LOG_H__
#define __MAIL_EWS_LOG_H__

#include <iostream>

#ifndef FORCE_PR_LOG
#define FORCE_PR_LOG 1/* Allow logging in the release build */
#endif

extern std::ostream mailews_logger;

#endif //__MAIL_EWS_LOG_H__
