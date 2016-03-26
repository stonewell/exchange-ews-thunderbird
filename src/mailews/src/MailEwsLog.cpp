#include "MailEwsLog.h"
#include <sstream>
#include "prlog.h"

static PRLogModuleInfo * mailewsLogModule = PR_NewLogModule( "MailEws" );
PRLogModuleInfo* DBLog = mailewsLogModule;

class MailEwsStringBuf : public std::stringbuf {
protected:
    int sync() {
        int ret = std::stringbuf::sync();

        std::string s = str();

        if (s.length() > 0) {
            const char * msg = s.c_str();

            if (msg && mailewsLogModule) {
                PR_LOG( mailewsLogModule, PR_LOG_DEBUG, ("%s", msg));
            }
            
            str("");
        }

        return ret;
    }
};

static MailEwsStringBuf sb;

std::ostream mailews_logger(&sb);


