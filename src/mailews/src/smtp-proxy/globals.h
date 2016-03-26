#pragma once
#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#define SMTP_DATA_TERMINATOR ".\r\n"

#define SMTP_STATUS_INIT 1
#define SMTP_STATUS_HELO 2
#define SMTP_STATUS_DATA 16
#define SMTP_STATUS_DATA_END 32

#define SMTP_CMD_HELO "HELO"
#define SMTP_CMD_EHLO "EHLO"
#define SMTP_CMD_MAIL "MAIL"
#define SMTP_CMD_RCPT "RCPT"
#define SMTP_CMD_DATA "DATA"
#define SMTP_CMD_RSET "RSET"
#define SMTP_CMD_NOOP "NOOP"
#define SMTP_CMD_QUIT "QUIT"

#define SMTP_CMD_VRFY "VRFY"
#define SMTP_CMD_EXPN "EXPN"


//SMTP (RFC 821) DEFINED VALUES
#define MAX_USER_LENGTH 64 
#define MAX_DOMAIN_LENGTH 64
#define MAX_ADDRESS_LENGTH 256
#define MAX_CMD_LENGTH 512
#define MAX_RCPT_ALLOWED 100

#ifndef _WIN32
#define MAX_PATH 255
#endif

#endif //__GLOBAL_H__
