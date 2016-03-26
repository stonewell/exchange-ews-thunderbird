BOOL ProcessHELO();
BOOL ProcessRCPT();
BOOL ProcessMAIL();
BOOL ProcessRSET();
BOOL ProcessNOOP();
BOOL ProcessQUIT();
BOOL ProcessDATA();
BOOL ProcessNotImplemented(BOOL bParam=FALSE);
BOOL ProcessLine(char *buf, int len);