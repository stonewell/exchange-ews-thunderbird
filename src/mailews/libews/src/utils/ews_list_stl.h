/*
 * ews_list_stl.h
 *
 *  Created on: Apr 2, 2014
 *      Author: stone
 */

#ifndef EWS_LIST_STL_H_
#define EWS_LIST_STL_H_

#include <vector>

namespace ews {
template<typename T>
class CEWSList: public std::vector<T *> {
public:
	CEWSList() {

	}

	virtual ~CEWSList() {
		Clear();
	}

public:
	void Clear() {
		typename CEWSList::iterator it = this->begin();
		while (it != this->end()) {
			typename CEWSList::iterator it2 = it + 1;
			delete (*it);
			it = it2;
		}

		this->clear();
	}
private:
	CEWSList<T> & operator =(const CEWSList<T> & v) {
		(void)v;
		return *this;
	}

	CEWSList(const CEWSList<T> & v) {
		(void)v;
	}
};

#define DEFINE_LIST(T) typedef CEWSList<T> T##List;

typedef std::vector<CEWSString> CEWSStringList;
typedef std::vector<int> CEWSIntList;
}
;
#endif /* EWS_LIST_STL_H_ */
