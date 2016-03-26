/* -*- mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#pragma once
#ifndef __CONTACT_EWS_AUTOCOMPLETE_SEARCH_H__
#define __CONTACT_EWS_AUTOCOMPLETE_SEARCH_H__

#include "nsCOMPtr.h"
#include "nsIAutoCompleteSearch.h"
#include "nsIAbDirSearchListener.h"
#include "nsIAutoCompleteResult.h"
#include "nsIAutoCompleteSearch.h"
#include "nsStringGlue.h"

class ContactEwsAutoCompleteSearch : public nsIAutoCompleteSearch,
                                     public nsIAbDirSearchListener
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIAUTOCOMPLETESEARCH
  NS_DECL_NSIABDIRSEARCHLISTENER

  ContactEwsAutoCompleteSearch();

private:
  virtual ~ContactEwsAutoCompleteSearch();

protected:
  /* additional members */
  nsCOMPtr<nsIAutoCompleteResult> m_Result;
  nsCOMPtr<nsIAutoCompleteObserver> m_Listener;
  int32_t m_SearchContext;
  nsString m_SearchFinishMsg;
};

#endif //__CONTACT_EWS_AUTOCOMPLETE_SEARCH_H__
