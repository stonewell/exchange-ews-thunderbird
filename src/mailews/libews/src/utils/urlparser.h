#ifndef URLPARSER_H_
#define URLPARSER_H_

#include <string>
#include <vector>

//the url query <name, value> pair
typedef std::pair< std::string, std::string > UrlQueryPair;
typedef std::vector< UrlQueryPair > UrlQueryList;

class UrlParser
{
public:
    UrlParser( const char* url );
    bool GetQueryVaule( const std::string& queryname, std::string& queryvalue);
    static void strToLower( std::string& src );

    const std::string GetSchema() const
    {
        return m_schema;
    }
    const std::string GetHost() const
    {
        return m_host;
    }
    const std::string GetPort() const
    {
        return m_port;
    }
    const std::string GetPath() const
    {
        return m_path;
    }
    const std::string GetQuery() const
    {
        return m_query;
    }
    const std::string GetFragment() const
    {
        return m_fragment;
    }
    const std::string GetUserName() const
    {
        return m_username;
    }
    const std::string GetPassword() const
    {
        return m_password;
    }

private:
    bool Init( const char* url );
    bool IsSchemeChar( int c );
    bool ParseQuery();

private:
    std::string m_schema;               /* mandatory */
    std::string m_host;                 /* mandatory */
    std::string m_port;                 /* optional */
    std::string m_path;                 /* optional */
    std::string m_query;                /* optional */
    std::string m_fragment;             /* optional */
    std::string m_username;             /* optional */
    std::string m_password;             /* optional */
    UrlQueryList m_urlquerylist;
};


#endif /* URLPARSER_H_ */
