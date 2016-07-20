#ifndef _HTTP_H_
#define _HTTP_H_
#include "Utils.h"

class CHttp
{
public:
	CHttp();
	~CHttp();

	void addHeader( const string& key, const string& value );
	void setPostData( const string& data ) { _postData = data; }
	bool request( const string& url, bool get = true );
	const string& getResponse() const { return _response; }

private:
	bool parseUrl( const string& url );
	bool doConnect();
	void doClose();
	bool doSend( const string& data );
	bool doRecv( string& data );
	bool parseResponse();
	void parseLine( const string& line, vector< string >& vCookie );
	void trimUrl( string& url );
private:
	bool _get;
#ifdef WIN32
	SOCKET _sock;
#else
	int _sock;
#endif
	string _postData;
	string _requestRes;
	string _host;
	vector< string > _vHeader;
	string _response;
	static string _cookie;
};


#endif //_HTTP_H_
