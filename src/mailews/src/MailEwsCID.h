#pragma once
#ifndef __MAIL_EWS_CID_H__
#define __MAIL_EWS_CID_H__

#include "nsIProtocolHandler.h"
#include "nsMsgBaseCID.h"

#define MAIL_EWS_MSG_FOLDER_CONTRACTID  "@mozilla.org/rdf/resource-factory;1?name=ews"

#define MAIL_EWS_SERVICE_CONTRACTID  "@angsto-tech.com/mailews/service;1"

#define MAIL_EWS_MSG_INCOMING_SERVER_CONTRACTID  NS_MSGINCOMINGSERVER_CONTRACTID_PREFIX "ews"

#define MAIL_EWS_SERVER_EXT_CID                  \
  { 0xa209beea, 0xcb4b, 0x4d15,                         \
  { 0xa0, 0xc7, 0x24, 0xf5, 0x9b, 0x75, 0x60, 0x92}}

#define MAIL_EWS_SERVER_EXT_CONTRACTID  "@mozilla.org/accountmanager/extension;1?name=ewsserver"

// generate unique ID here with uuidgen
//3e0d973d-370c-4b91-85e4-34814033b061
#define MAIL_EWS_MSG_PROTOCOL_INFO_CID                  \
  { 0x3e0d973d, 0x370c, 0x4b91,                         \
  { 0x85, 0xe4, 0x34, 0x81, 0x40, 0x33, 0xb0, 0x61}}

#define MAIL_EWS_MSG_PROTOCOL_INFO_CONTRACTID  "@mozilla.org/messenger/protocol/info;1?type=ews"

#define MAIL_EWS_AUTH_EXT_CID                  \
  { 0xb5ffe120, 0x6729, 0x4997,                         \
  { 0xb7, 0x0b, 0x0a, 0xa4, 0x13, 0xe0, 0x8c, 0x51}}

#define MAIL_EWS_AUTH_EXT_CONTRACTID \
    "@mozilla.org/accountmanager/extension;1?name=ewsauth"

#define MAIL_EWS_MSG_MESSAGE_SERVICE_CID \
    { \
        0x41994f48, 0x0838, 0x4548,             \
        { \
         0x92, 0x5d, 0x91, 0x22, 0x01, 0x21, 0xb4, 0x6a \
        }                                               \
    }

#define MAIL_EWS_MSG_MESSAGE_SERVICE_CONTRACTID \
    "@mozilla.org/messenger/messageservice;1?type=ews-message"

#define MAIL_EWS_MSG_MESSAGE_SERVICE_CONTRACTID2  \
  "@mozilla.org/messenger/ewsservice;1"

#define MAIL_EWS_MSG_MESSAGE_SERVICE_CONTRACTID3 \
  "@mozilla.org/messenger/messageservice;1?type=ews"

#define MAIL_EWS_MSG_MESSAGE_SERVICE_CONTRACTID4 \
  NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "ews"

#define MAIL_EWS_MSG_URL_CONTRACTID  \
  "@mozilla.org/messenger/ewsurl;1"

#define MAIL_EWS_MSG_URL_CID \
    { \
        0xcd49203f, 0x82ad, 0x48af, \
        { \
            0xa5, 0x29, 0xc3, 0x0c, 0x69, 0xd5, 0x73, 0x2b \
        }                           \
    }

#define MAIL_EWS_ITEMS_CALLBACK_CONTRACTID  "@angsto-tech.com/mailews/itemscallback;1"

#endif //__MAIL_EWS_CID_H__
