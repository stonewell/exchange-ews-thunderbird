/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "msgCore.h"
#include "prlog.h"
#include "prmem.h"
#include "MsgLineStreamBuffer.h"
#include "nsAlgorithm.h"
#include "MsgUtils.h"
#include "nsIInputStream.h" // used by MsgLineStreamBuffer
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////////////////////////
// This is a utility class used to efficiently extract lines from an input stream by buffering
// read but unprocessed stream data in a buffer. 
///////////////////////////////////////////////////////////////////////////////////////////////////

MsgLineStreamBuffer::MsgLineStreamBuffer(uint32_t aBufferSize, bool aAllocateNewLines, bool aEatCRLFs, char aLineToken) 
           : m_eatCRLFs(aEatCRLFs), m_allocateNewLines(aAllocateNewLines), m_lineToken(aLineToken)
{
  NS_PRECONDITION(aBufferSize > 0, "invalid buffer size!!!");
  m_dataBuffer = nullptr;
  m_startPos = 0;
    m_numBytesInBuffer = 0;

  // used to buffer incoming data by ReadNextLineFromInput
  if (aBufferSize > 0)
  {
    m_dataBuffer = (char *) PR_CALLOC(sizeof(char) * aBufferSize);
  }

  m_dataBufferSize = aBufferSize;
}

MsgLineStreamBuffer::~MsgLineStreamBuffer()
{
  PR_FREEIF(m_dataBuffer); // release our buffer...
}


nsresult MsgLineStreamBuffer::GrowBuffer(int32_t desiredSize)
{
  char* newBuffer = (char *) PR_REALLOC(m_dataBuffer, desiredSize);
  NS_ENSURE_TRUE(newBuffer, NS_ERROR_OUT_OF_MEMORY);
  m_dataBuffer = newBuffer;
  m_dataBufferSize = desiredSize;
  return NS_OK;
}

void MsgLineStreamBuffer::ClearBuffer()
{
  m_startPos = 0;
  m_numBytesInBuffer = 0;
}

// aInputStream - the input stream we want to read a line from
// aPauseForMoreData is returned as true if the stream does not yet contain a line and we must wait for more
// data to come into the stream.
// Note to people wishing to modify this function: Be *VERY CAREFUL* this is a critical function used by all of
// our mail protocols including imap, nntp, and pop. If you screw it up, you could break a lot of stuff.....

char * MsgLineStreamBuffer::ReadNextLine(nsIInputStream * aInputStream, uint32_t &aNumBytesInLine, bool &aPauseForMoreData, nsresult *prv, bool addLineTerminator)
{
  // try to extract a line from m_inputBuffer. If we don't have an entire line, 
  // then read more bytes out from the stream. If the stream is empty then wait
  // on the monitor for more data to come in.
  
  NS_PRECONDITION(m_dataBuffer && m_dataBufferSize > 0, "invalid input arguments for read next line from input");

  if (prv)
    *prv = NS_OK;
  // initialize out values
  aPauseForMoreData = false;
  aNumBytesInLine = 0;
  char * endOfLine = nullptr;
  char * startOfLine = m_dataBuffer+m_startPos;
  
  if (m_numBytesInBuffer > 0) // any data in our internal buffer?
    endOfLine = PL_strchr(startOfLine, m_lineToken); // see if we already have a line ending...
  
  // it's possible that we got here before the first time we receive data from the server
  // so aInputStream will be nullptr...
  if (!endOfLine && aInputStream) // get some more data from the server
  {
    nsresult rv;
    uint64_t numBytesInStream = 0;
    uint32_t numBytesCopied = 0;
    bool nonBlockingStream;
    aInputStream->IsNonBlocking(&nonBlockingStream);
    rv = aInputStream->Available(&numBytesInStream);
    if (NS_FAILED(rv))
    {
      if (prv)
        *prv = rv;
      aNumBytesInLine = -1;
      return nullptr;
    }
    if (!nonBlockingStream && numBytesInStream == 0) // if no data available,
      numBytesInStream = m_dataBufferSize / 2; // ask for half the data buffer size.

    // if the number of bytes we want to read from the stream, is greater than the number
    // of bytes left in our buffer, then we need to shift the start pos and its contents
    // down to the beginning of m_dataBuffer...
    uint32_t numFreeBytesInBuffer = m_dataBufferSize - m_startPos - m_numBytesInBuffer;
    if (numBytesInStream >= numFreeBytesInBuffer)
    {
      if (m_startPos)
      {
        memmove(m_dataBuffer, startOfLine, m_numBytesInBuffer);
        // make sure the end of the buffer is terminated
        m_dataBuffer[m_numBytesInBuffer] = '\0';
        m_startPos = 0;
        startOfLine = m_dataBuffer;
        numFreeBytesInBuffer = m_dataBufferSize - m_numBytesInBuffer;
        //printf("moving data in read line around because buffer filling up\n");
      }
      // If we didn't make enough space (or any), grow the buffer
      if (numBytesInStream >= numFreeBytesInBuffer)
      {
        int64_t growBy = (numBytesInStream - numFreeBytesInBuffer) * 2 + 1;
        // GrowBuffer cannot handles over 4GB size
        if (m_dataBufferSize + growBy > PR_UINT32_MAX)
          return nullptr;
        // try growing buffer by twice as much as we need.
        nsresult rv = GrowBuffer(m_dataBufferSize + growBy);
        // if we can't grow the buffer, we have to bail.
        if (NS_FAILED(rv))
          return nullptr;
        startOfLine = m_dataBuffer;
        numFreeBytesInBuffer += growBy;
      }
      NS_ASSERTION(m_startPos == 0, "m_startPos should be 0 .....\n");
    }
    
    uint32_t numBytesToCopy = std::min(uint64_t(numFreeBytesInBuffer - 1) /* leave one for a null terminator */, numBytesInStream);
    if (numBytesToCopy > 0)
    {
      // read the data into the end of our data buffer
      char *startOfNewData = startOfLine + m_numBytesInBuffer;
      rv = aInputStream->Read(startOfNewData, numBytesToCopy, &numBytesCopied);
      if (prv)
        *prv = rv;
      uint32_t i;
      for (i = 0; i < numBytesCopied; i++)  // replace nulls with spaces
      {
        if (!startOfNewData[i])
          startOfNewData[i] = ' ';
      }
      m_numBytesInBuffer += numBytesCopied;
      m_dataBuffer[m_startPos + m_numBytesInBuffer] = '\0';

      // okay, now that we've tried to read in more data from the stream,
      // look for another end of line character in the new data
      endOfLine = PL_strchr(startOfNewData, m_lineToken);
    }
  }
  
  // okay, now check again for endOfLine.
  if (endOfLine)
  {
    if (!m_eatCRLFs)
      endOfLine += 1; // count for LF or CR
    
    aNumBytesInLine = endOfLine - startOfLine;
    
    if (m_eatCRLFs && aNumBytesInLine > 0 && startOfLine[aNumBytesInLine-1] == '\r') // Remove the CR in a CRLF sequence
      aNumBytesInLine--;
    
    // PR_CALLOC zeros out the allocated line
    char* newLine = (char*) PR_CALLOC(aNumBytesInLine + (addLineTerminator ? MSG_LINEBREAK_LEN : 0) + 1);
    if (!newLine)
    {
      aNumBytesInLine = 0;
      aPauseForMoreData = true;
      return nullptr;
    }
    
    memcpy(newLine, startOfLine, aNumBytesInLine); // copy the string into the new line buffer
    if (addLineTerminator)
    {
      memcpy(newLine + aNumBytesInLine, MSG_LINEBREAK, MSG_LINEBREAK_LEN);
      aNumBytesInLine += MSG_LINEBREAK_LEN;
    }
    
    if (m_eatCRLFs)
      endOfLine += 1; // advance past LF or CR if we haven't already done so...
    
    // now we need to update the data buffer to go past the line we just read out. 
    m_numBytesInBuffer -= (endOfLine - startOfLine);
    if (m_numBytesInBuffer)
      m_startPos = endOfLine - m_dataBuffer;
    else
      m_startPos = 0;
    
    return newLine;
  }
  
  aPauseForMoreData = true;
  return nullptr; // if we somehow got here. we don't have another line in the buffer yet...need to wait for more data...
}

bool MsgLineStreamBuffer::NextLineAvailable()
{
  return (m_numBytesInBuffer > 0 && PL_strchr(m_dataBuffer+m_startPos, m_lineToken));
}

