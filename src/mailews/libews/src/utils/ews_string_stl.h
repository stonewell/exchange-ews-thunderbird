/*
 * ews_string_stl.h
 *
 *  Created on: Apr 2, 2014
 *      Author: stone
 */

#ifndef EWS_STRING_STL_H_
#define EWS_STRING_STL_H_

namespace ews {
	class DLL_PUBLIC CEWSString {
 private:
    void * m_pData;
 public:
    virtual ~CEWSString();

    CEWSString();

    CEWSString(const char * v);

    CEWSString(const char * v, long size);

    CEWSString(const CEWSString & v);

 public:
	CEWSString & operator =(const CEWSString & v);

	operator const char *() const {
      return GetData();
	}

	const char * GetData() const;

    void Reserve(long capacity);

    void Clear();

	void CopyTo(char * buf, long len = (long) -1);

	int CompareTo(const CEWSString & v) const;

	void Append(const CEWSString & v);
	bool IsEmpty() const;

    int ReplaceAll(const CEWSString & old_pattern, const CEWSString & new_pattern);

    long GetLength() const;
  };
}
;

#endif /* EWS_STRING_STL_H_ */
