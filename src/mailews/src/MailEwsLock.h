#pragma once
#ifndef __MAILEWS_LOCK_H__
#define __MAILEWS_LOCK_H__

#include "nspr.h"
#include "nscore.h"

class MailEwsLock {
public:
	MailEwsLock(PRLock * lock)
			: m_Lock(lock) {
		PR_Lock(m_Lock);
	}

	~MailEwsLock() {
		PR_Unlock(m_Lock);
	}
public:
	PRLock * m_Lock;
};
#endif // __MAILEWS_LOCK_H__
