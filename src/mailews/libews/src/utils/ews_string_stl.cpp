#include "libews_defs.h"
#include "ews_string_stl.h"

#include <string>
#include <string.h>

#define PTR(x) ((std::string *)x)

namespace ews {
CEWSString::~CEWSString() {
  delete PTR(m_pData);
}

CEWSString::CEWSString() : m_pData(new std::string()) {

}

CEWSString::CEWSString(const char * v) :
    m_pData(new std::string()) {
  PTR(m_pData)->append(v);
}

CEWSString::CEWSString(const char * v, long size) :
    m_pData(new std::string()) {
  PTR(m_pData)->append(v, size);
}

CEWSString::CEWSString(const CEWSString & v) :
    m_pData(new std::string()) {
  Append(v);
}

CEWSString & CEWSString::operator =(const CEWSString & v) {
  Clear();
  Append(v);

  return *this;
}

const char * CEWSString::GetData() const {
  return PTR(m_pData)->c_str();
}

void CEWSString::Reserve(long capacity) {
  PTR(m_pData)->reserve(capacity);
}

void CEWSString::Clear() {
  PTR(m_pData)->clear();
}

void CEWSString::CopyTo(char * buf, long len) {
  if (NULL == buf || len == 0)
    return;

  if (len == (long) -1) {
    len = PTR(m_pData)->length();
  }

  long copy_len = len > PTR(m_pData)->length() ? PTR(m_pData)->length() : len;

  memmove(buf, (const void *) GetData(), copy_len);
}

int CEWSString::CompareTo(const CEWSString & v) const {
  return PTR(m_pData)->compare(*PTR(v.m_pData));
}

void CEWSString::Append(const CEWSString & v) {
  PTR(m_pData)->append(v.GetData());
}

bool CEWSString::IsEmpty() const {
  return PTR(m_pData)->empty();
}

int CEWSString::ReplaceAll(const CEWSString & old_pattern, const CEWSString & new_pattern) {
  int count = 0;
  const long nsize = new_pattern.GetLength();
  const long psize = old_pattern.GetLength();

  for(long pos = PTR(m_pData)->find(*PTR(old_pattern.m_pData), 0);
      pos != std::string::npos;
      pos = PTR(m_pData)->find(*PTR(old_pattern.m_pData),pos + nsize)) {
    PTR(m_pData)->replace(pos, psize, *PTR(new_pattern.m_pData));
    count++;
  }

  return count;
}

long CEWSString::GetLength() const {
  return PTR(m_pData)->size(); 
}
}



