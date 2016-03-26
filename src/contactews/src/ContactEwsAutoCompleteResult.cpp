/* -*- mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
#include "ContactEwsAutoCompleteResult.h"

#include "nsServiceManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsArrayUtils.h"

#include "nsIAbCard.h"

/* Implementation file */
NS_IMPL_ISUPPORTS(ContactEwsAutoCompleteResult,
                  nsIAutoCompleteResult,
                  nsIAbAutoCompleteResult,
                  IContactEwsAutoCompleteResult)

ContactEwsAutoCompleteResult::ContactEwsAutoCompleteResult(const nsAString & aSearchString)
: m_SearchString(aSearchString)
    , m_SearchResult(nsIAutoCompleteResult::RESULT_NOMATCH)
    , m_Cards(do_CreateInstance(NS_ARRAY_CONTRACTID))
{
  /* member initializers and constructor code */
}

ContactEwsAutoCompleteResult::~ContactEwsAutoCompleteResult()
{
  /* destructor code */
}

/* readonly attribute AString searchString; */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetSearchString(nsAString & aSearchString)
{
  aSearchString = m_SearchString;
  return NS_OK;
}

/* readonly attribute unsigned short searchResult; */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetSearchResult(uint16_t *aSearchResult)
{
  *aSearchResult = m_SearchResult;
  return NS_OK;
}

/* readonly attribute long defaultIndex; */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetDefaultIndex(int32_t *aDefaultIndex)
{
  *aDefaultIndex = 0;
  return NS_OK;
}

/* readonly attribute AString errorDescription; */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetErrorDescription(nsAString & aErrorDescription)
{
  aErrorDescription.Assign(NS_LITERAL_STRING(""));
  return NS_OK;
}

/* readonly attribute unsigned long matchCount; */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetMatchCount(uint32_t *aMatchCount)
{
  m_Cards->GetLength(aMatchCount);
  return NS_OK;
}

/* readonly attribute boolean typeAheadResult; */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetTypeAheadResult(bool *aTypeAheadResult)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* AString getValueAt (in long index); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetValueAt(int32_t index, nsAString & _retval)
{
  nsCOMPtr<nsIAbCard> card;

  nsresult rv = GetCardAt(index, getter_AddRefs(card));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString displayName;
  rv = card->GetDisplayName(displayName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsString primaryEmail;
  rv = card->GetPrimaryEmail(primaryEmail);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!displayName.IsEmpty()) {
    _retval.Append(displayName);
    _retval.Append(NS_LITERAL_STRING(" <"));
  }

  _retval.Append(primaryEmail);
  
  if (!displayName.IsEmpty()) {
    _retval.Append(NS_LITERAL_STRING(">"));
  }
  
  return NS_OK;
}

/* AString getLabelAt (in long index); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetLabelAt(int32_t index, nsAString & _retval)
{
  return GetValueAt(index, _retval);
}

/* AString getCommentAt (in long index); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetCommentAt(int32_t index, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* AString getStyleAt (in long index); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetStyleAt(int32_t index, nsAString & _retval)
{
  if (m_SearchResult == nsIAutoCompleteResult::RESULT_FAILURE) {
    _retval.Assign(NS_LITERAL_STRING("remote-err"));
  } else {
    _retval.Assign(NS_LITERAL_STRING("remote-abook"));
  }
  return NS_OK;
}

/* AString getImageAt (in long index); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetImageAt(int32_t index, nsAString & _retval)
{
  _retval.Assign(NS_LITERAL_STRING(""));
  return NS_OK;
}

/* AString getFinalCompleteValueAt (in long index); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetFinalCompleteValueAt(int32_t index, nsAString & _retval)
{
  return GetValueAt(index, _retval);
}

/* void removeValueAt (in long rowIndex, in boolean removeFromDb); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::RemoveValueAt(int32_t rowIndex, bool removeFromDb)
{
  return NS_OK;
}

/* nsIAbCard getCardAt (in long index); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetCardAt(int32_t index, nsIAbCard * *_retval)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIAbCard> card = do_QueryElementAt(m_Cards, index, &rv);

  NS_ENSURE_SUCCESS(rv, rv);

  NS_IF_ADDREF(*_retval = card);

  return NS_OK;
}

/* AString getEmailToUse (in long index); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::GetEmailToUse(int32_t index, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP ContactEwsAutoCompleteResult::SetSearchResult(uint16_t aSearchResult)
{
  m_SearchResult = aSearchResult;
  return NS_OK;
}

/* void addCard (in nsIAbCard aCard); */
NS_IMETHODIMP ContactEwsAutoCompleteResult::AddCard(nsIAbCard *aCard)
{
  m_Cards->AppendElement(aCard, false);
  return NS_OK;
}
