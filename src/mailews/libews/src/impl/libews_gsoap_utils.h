/*
 * libews_gsoap_utils.h
 *
 *  Created on: Jul 25, 2014
 *      Author: stone
 */

#ifndef LIBEWS_GSOAP_UTILS_H_
#define LIBEWS_GSOAP_UTILS_H_

namespace ews {
DLL_LOCAL CEWSString XsdBase64BinaryToEWSString(struct soap *soap,
		xsd__base64Binary * pBinary);
DLL_LOCAL xsd__base64Binary * EWSStringToXsdBase64Binary(struct soap *soap,
		const CEWSString & v);
DLL_LOCAL ews2__DistinguishedFolderIdNameType ConvertDistinguishedFolderIdName(
		CEWSFolder::EWSDistinguishedFolderIdNameEnum name);

DLL_LOCAL void convertNotifyTypeList(int notifyTypeFlags, CEWSIntList & notifyTypesSoap);

#define SAVE_ERROR(pError, responseMessage)                             \
    if (pError) {                                                       \
        pError->SetErrorMessage(responseMessage                         \
                                ->__ResponseMessageType_sequence        \
                                ->MessageText->c_str());                \
        pError->SetErrorCode((int)(*(responseMessage                    \
                                     ->__ResponseMessageType_sequence   \
                                     ->ResponseCode)));                 \
    }

#define SET_INVALID_RESPONSE	  \
	if (pError) { \
		pError->SetErrorMessage("Invalid Response Message."); \
		pError->SetErrorCode(EWS_FAIL); \
	}

class DLL_LOCAL _BaseEntry {
public:
	_BaseEntry() {

	}

	virtual ~_BaseEntry() {

	}
};

template<typename T>
class DLL_LOCAL _Entry : public _BaseEntry{
public:
	T * p;

	_Entry(T * _p) :
			p(_p) {

	}

	virtual ~_Entry() {
		delete p;
	}
};

template<typename T>
class DLL_LOCAL _ArrayEntry : public _BaseEntry{
public:
	T * p;

	_ArrayEntry(T * _p) :
			p(_p) {

	}

	virtual ~_ArrayEntry() {
		delete[] p;
	}
};

class DLL_LOCAL CEWSObjectPool {

	CEWSList<_BaseEntry> v;

public:
	CEWSObjectPool() {

	}

	~CEWSObjectPool() {
	}

	template<typename T>
	T * Create() {
		typedef _Entry<T> TEntry;
		T * p = new T();
		TEntry * pp = new TEntry(p);
		v.push_back(pp);

		return p;
	}

	template<typename T>
	T * CreateArray(size_t s) {
		typedef _ArrayEntry<T> TArrayEntry;
		T * p = new T[s];
		TArrayEntry * pp = new TArrayEntry(p);
		v.push_back(pp);

		return p;
	}

	template<typename T>
	T * Create(const T & vv) {
		typedef _Entry<T> TEntry;
		T * p = new T(vv);

		TEntry * pp = new TEntry(p);
		v.push_back(pp);

		return p;
	}

	template<typename T>
	void Add(T * p) {
		typedef _Entry<T> TEntry;
		TEntry * pp = new TEntry(p);
		v.push_back(pp);
	}

	template<typename T>
	void AddArray(T * p) {
		typedef _ArrayEntry<T> TEntry;
		TEntry * pp = new TEntry(p);
		v.push_back(pp);
	}
};
}

#endif /* LIBEWS_GSOAP_UTILS_H_ */
