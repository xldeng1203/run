#ifndef _MIX_HTTP_H_
#define _MIX_HTTP_H_
#include "Utils.h"
#include "openssl/bio.h"
#include "openssl/ssl.h"
#include "openssl/err.h"

typedef struct _tag_COMM_HANDLE
{
#ifdef WIN32
	SOCKET sock;
#else
	int sock;
#endif
	SSL* ssl;
	SSL_CTX* ctx;
	BIO* bio;
	bool isSsl;
	_tag_COMM_HANDLE()
#ifdef WIN32
	: sock(INVALID_SOCKET)
#else
	: sock(-1)
#endif
	, ctx(NULL)
	, isSsl(false)
	, ssl(NULL)
	, bio(NULL)
	{
	}
}COMM_HANDLE;

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
	
	bool connectCommon();
	bool connectSsl();

	bool sendCommon( const string& data );
	bool sendSsl( const string& data );
	bool recvCommon( string& data );
	bool recvSsl( string& data );
private:
	bool _get;
#ifdef WIN32
	SOCKET _sock;
#else
	int _sock;
#endif
	bool _ssl;
	COMM_HANDLE _hComm;
	string _postData;
	string _requestRes;
	string _host;
	vector< string > _vHeader;
	string _response;
	static string _cookie;
};


#endif //_MIX_HTTP_H_
