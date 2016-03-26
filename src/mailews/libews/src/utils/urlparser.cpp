#include "urlparser.h"
#include <string.h>

UrlParser::UrlParser( const char* url ):
	m_schema(""), m_host(""), m_port(""),
	m_path(""), m_query(""), m_fragment(""),
	m_username(""), m_password(""), m_urlquerylist()
{
	Init(url);
	ParseQuery();
}

bool UrlParser::Init( const char* url )
{
	const char *tmpstr;
	const char *curstr;
	int len;
	int i;
	int userpass_flag;
	int bracket_flag;

	curstr = url;

	/*
	 * <scheme>:<scheme-specific-part>
	 * <scheme> := [a-z\+\-\.]+
	 *             upper case = lower case for resiliency
	 */
	/* Read scheme */
	// ET3417057: cover the url without scheme
	tmpstr = strchr(curstr, ':');
	if ( NULL != tmpstr ) {
		/* Get the scheme length */
		len = tmpstr - curstr;
		/* Check restrictions */
		for ( i = 0; i < len; i++ ) {
			if ( !IsSchemeChar(curstr[i]) ) {
				/* Invalid format */
				return false;
			}
		}
		/* Copy the scheme to the storage */
		m_schema.assign(curstr, len);

		/* Make the character to lower if it is upper case. */
		strToLower(m_schema);

		/* Skip ':' */
		tmpstr++;
		curstr = tmpstr;

		/* Eat "//" */
		for ( i = 0; i < 2; i++ ) {
			if ( '/' != *curstr ) {
				return false;
			}
			curstr++;
		}
	}

	/*
	 * //<user>:<password>@<host>:<port>/<url-path>
	 * Any ":", "@" and "/" must be encoded.
	 */

	/* Check if the user (and password) are specified. */
	userpass_flag = 0;
	tmpstr = curstr;
	while ( '\0' != *tmpstr ) {
		if ( '@' == *tmpstr ) {
			/* Username and password are specified */
			userpass_flag = 1;
			break;
		} else if ( '/' == *tmpstr ) {
			/* End of <host>:<port> specification */
			userpass_flag = 0;
			break;
		}
		tmpstr++;
	}

	/* User and password specification */
	tmpstr = curstr;
	if ( userpass_flag ) {
		/* Read username */
		while ( '\0' != *tmpstr && ':' != *tmpstr && '@' != *tmpstr ) {
			tmpstr++;
		}
		len = tmpstr - curstr;
		m_username.assign(curstr, len);

		/* Proceed current pointer */
		curstr = tmpstr;
		if ( ':' == *curstr ) {
			/* Skip ':' */
			curstr++;
			/* Read password */
			tmpstr = curstr;
			while ( '\0' != *tmpstr && '@' != *tmpstr ) {
				tmpstr++;
			}
			len = tmpstr - curstr;
			m_password.assign(curstr, len);
			curstr = tmpstr;
		}
		/* Skip '@' */
		if ( '@' != *curstr ) {
			return false;
		}
		curstr++;
	}

	if ( '[' == *curstr ) {
		bracket_flag = 1;
	} else {
		bracket_flag = 0;
	}
	/* Proceed on by delimiters with reading host */
	tmpstr = curstr;
	while ( '\0' != *tmpstr ) {
		if ( bracket_flag && ']' == *tmpstr ) {
			/* End of IPv6 address. */
			tmpstr++;
			break;
		} else if ( !bracket_flag && (':' == *tmpstr || '/' == *tmpstr) ) {
			/* Port number is specified. */
			break;
		}
		tmpstr++;
	}
	len = tmpstr - curstr;
	m_host.assign(curstr, len);
	strToLower(m_host);
	curstr = tmpstr;

	/* Is port number specified? */
	if ( ':' == *curstr ) {
		curstr++;
		/* Read port number */
		tmpstr = curstr;
		while ( '\0' != *tmpstr && '/' != *tmpstr ) {
			tmpstr++;
		}
		len = tmpstr - curstr;
		m_port.assign(curstr, len);
		curstr = tmpstr;
	}

	/* End of the std::string */
	if ( '\0' == *curstr ) {
		return true;
	}

	/* Skip '/' */
	//if ( '/' != *curstr ) {
	//return false;
	//}
	//curstr++;

	/* Parse path */
	tmpstr = curstr;
	while ( '\0' != *tmpstr && '#' != *tmpstr  && '?' != *tmpstr ) {
		tmpstr++;
	}
	len = tmpstr - curstr;

	m_path.assign(curstr, len);
	curstr = tmpstr;

	/* Is query specified? */
	if ( '?' == *curstr ) {
		/* Skip '?' */
		curstr++;
		/* Read query */
		tmpstr = curstr;
		while ( '\0' != *tmpstr && '#' != *tmpstr ) {
			tmpstr++;
		}
		len = tmpstr - curstr;
		m_query.assign(curstr, len);
		curstr = tmpstr;
	}

	/* Is fragment specified? */
	if ( '#' == *curstr ) {
		/* Skip '#' */
		curstr++;
		/* Read fragment */
		tmpstr = curstr;
		while ( '\0' != *tmpstr ) {
			tmpstr++;
		}
		len = tmpstr - curstr;
		m_fragment.assign(curstr, len);
		curstr = tmpstr;
	}

	return true;
}

bool UrlParser::IsSchemeChar( int c )
{
	return (!isalpha(c) && '+' != c && '-' != c && '.' != c) ? 0 : 1;
}

void UrlParser::strToLower( std::string& src )
{
	unsigned int i = 0;
	for ( ; i < src.length(); i++ ) {
		src[i] = tolower(src[i]);
	}
}

bool UrlParser::ParseQuery()
{
	if ( m_query.empty() ) {
		return false;
	}

	const char *tmpstr;
	const char *curstr;
	int len;

	curstr = m_query.c_str();

	/* query_name=query_valuse&query_name=query_valuse&... */
	while ( '\0' != *curstr)
	{
		UrlQueryPair tmpPair;
		tmpstr = strchr(curstr, '=');
		if ( NULL == tmpstr ) {
			break;
		}

		len = tmpstr - curstr;
		// read the query name
		tmpPair.first.assign(curstr, len);
		strToLower(tmpPair.first);

		// skip '='
		tmpstr++;
		curstr = tmpstr;

		while ( '&' != *tmpstr && '\0' != *tmpstr ) {
			tmpstr++;
		}
		len = tmpstr - curstr;
		// read the query value
		tmpPair.second.assign(curstr, len);

		// skip '&'
		if ( '&' == *tmpstr ) {
			tmpstr++;
		}

		curstr = tmpstr;

		m_urlquerylist.push_back(tmpPair);
	}

	return true;
}

bool UrlParser::GetQueryVaule( const std::string& queryname, std::string& queryvalue)
{
	if ( queryname.empty() ) {
		return false;
	}

	UrlQueryList::const_iterator iter = m_urlquerylist.begin();
	for ( ; iter != m_urlquerylist.end(); ++iter ) {
		if ( 0 == queryname.compare(iter->first) ) {
			queryvalue = iter->second;
			return true;
		}
	}

	return false;
}

