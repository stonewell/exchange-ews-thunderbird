/* -*- mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#pragma once
#ifndef __CONTACT_EWS_AUTO_COMPLETE_RESULT_H__
#define __CONTACT_EWS_AUTO_COMPLETE_RESULT_H__

#include "nsIAbAutoCompleteResult.h"
#include "nsStringGlue.h"
#include "nsIMutableArray.h"
#include "nsCOMPtr.h"

#include "IContactEwsAutoCompleteResult.h"

/* Header file */
class ContactEwsAutoCompleteResult : public nsIAbAutoCompleteResult
                                   , public IContactEwsAutoCompleteResult
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIAUTOCOMPLETERESULT
  NS_DECL_NSIABAUTOCOMPLETERESULT
  NS_DECL_ICONTACTEWSAUTOCOMPLETERESULT

  ContactEwsAutoCompleteResult(const nsAString & aSearchString);

private:
  virtual ~ContactEwsAutoCompleteResult();

protected:
  /* additional members */
  nsString m_SearchString;
  uint16_t m_SearchResult;
  nsCOMPtr<nsIMutableArray> m_Cards;
};

#endif // __CONTACT_EWS_AUTO_COMPLETE_RESULT_H__
