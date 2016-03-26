// stdafx.cpp : source file that includes just the standard includes
// MailSocket.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"



// TODO: reference any additional headers you need in STDAFX.H
// and not in this file
/*
int ReceiveLine(SOCKET soc, char *buf, int len)
{
	char lbuf[1025];
	int llen,
		count=0;

	while(llen=recv(soc,lbuf,sizeof(lbuf),0)>0)
	{
		if(strchr(lbuf,'\n')!=NULL)
		{
			buf[count]=0;
			return 0;
		}

		strncpy(&buf[count++],buf,len);
	}

	return WSAGetLastError();
}
*/
