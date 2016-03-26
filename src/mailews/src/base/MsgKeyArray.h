/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef MsgKeyArray_h__
#define MsgKeyArray_h__

#include "nsIMsgKeyArray.h"
#include "nsTArray.h"

/*
 * This class is a thin wrapper around an nsTArray<nsMsgKey>
 */
class MsgKeyArray : public nsIMsgKeyArray
{
public:
  MsgKeyArray();

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMSGKEYARRAY

  nsTArray<nsMsgKey> m_keys;
protected:
  virtual ~MsgKeyArray();
};

#endif
