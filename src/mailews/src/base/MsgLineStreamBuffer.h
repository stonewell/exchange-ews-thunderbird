/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef _MsgLineStreamBuffer_H
#define _MsgLineStreamBuffer_H

#include "msgCore.h"    // precompiled header...

class nsIInputStream;

class NS_MSG_BASE MsgLineStreamBuffer
{
public:
  // aBufferSize -- size of the buffer you want us to use for buffering stream data
  // aEndOfLinetoken -- The delimiter string to be used for determining the end of line. This 
  //              allows us to parse platform specific end of line endings by making it
  //            a parameter.
  // aAllocateNewLines -- true if you want calls to ReadNextLine to allocate new memory for the line. 
  //            if false, the char * returned is just a ptr into the buffer. Subsequent calls to
  //            ReadNextLine will alter the data so your ptr only has a life time of a per call.
  // aEatCRLFs  -- true if you don't want to see the CRLFs on the lines returned by ReadNextLine. 
  //         false if you do want to see them.
  // aLineToken -- Specify the line token to look for, by default is LF ('\n') which cover as well CRLF. If
  //            lines are terminated with a CR only, you need to set aLineToken to CR ('\r')
  MsgLineStreamBuffer(uint32_t aBufferSize, bool aAllocateNewLines, 
                        bool aEatCRLFs = true, char aLineToken = '\n'); // specify the size of the buffer you want the class to use....
  virtual ~MsgLineStreamBuffer();
  
  // Caller must free the line returned using PR_Free
  // aEndOfLinetoken -- delimiter used to denote the end of a line.
  // aNumBytesInLine -- The number of bytes in the line returned
  // aPauseForMoreData -- There is not enough data in the stream to make a line at this time...
  char * ReadNextLine(nsIInputStream * aInputStream, uint32_t &anumBytesInLine, bool &aPauseForMoreData, nsresult *rv = nullptr, bool addLineTerminator = false);
  nsresult GrowBuffer(int32_t desiredSize);
  void ClearBuffer();
  bool NextLineAvailable();
protected:
  bool m_eatCRLFs;
  bool m_allocateNewLines;
  char * m_dataBuffer;
  uint32_t m_dataBufferSize;
  uint32_t m_startPos;
  uint32_t m_numBytesInBuffer;
  char m_lineToken;
};


#endif
