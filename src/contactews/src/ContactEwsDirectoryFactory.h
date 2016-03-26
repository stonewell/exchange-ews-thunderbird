#pragma once
#ifndef __CONTACT_EWS_DIRECTORY_FACTORY_H__
#define __CONTACT_EWS_DIRECTORY_FACTORY_H__

#include "nsIAbDirFactory.h"

class ContactEwsDirectoryFactory : public nsIAbDirFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIABDIRFACTORY

  ContactEwsDirectoryFactory();
protected:
  virtual ~ContactEwsDirectoryFactory();  
};

#endif // __CONTACT_EWS_DIRECTORY_FACTORY_H__

