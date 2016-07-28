#include "http.h"
#include <iostream>
#include <time.h>
#include "Utils.h"
#include "Locker.h"
/* Initializing OpenSSL */
using namespace std;

string CHttp::_cookie;

static Locker* _hostLocker = NULL;
map< string, in_addr > g_mapHost;

void hostLock();
void hostUnlock();

CHttp::CHttp()
: _get(true)
, _sock(-1)
, _ssl(false)
{
}

CHttp::~CHttp()
{
}

void CHttp::addHeader( const string& key, const string& value )
{
	string s = key + ": " + value + "\r\n";
	_vHeader.push_back(s);
}

bool CHttp::request( const string& url, bool get )
{
	if ( !parseUrl( url ) )
	{
		return false;
	}

	if ( !doConnect() )
	{
		return false;
	}

	_get = get;

	string request;
	if ( _get )
	{
		request = "GET ";
	}
	else
	{
		request = "POST ";
	}
	request += _requestRes;
	request += " HTTP/1.1\r\n";

	request += "Host: " + _host + "\r\n";

	request += "Connection: Close\r\n";

	request += "User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/48.0.2564.116 Safari/537.36\r\n";

	if ( !_cookie.empty() )
	{
		request += "Cookie: " + _cookie + "\r\n";
	}

	if ( !_get )
	{
		request += "Content-Length: ";
		char szTemp[50] = {0};
		sprintf( szTemp, "%lu", _postData.size() );
		request += szTemp;
		request += "\r\n";
	}

	request += "\r\n";

	if ( !_get )
	{
		request += _postData;
	}

//	cout<<"request:"<<request<<endl;

//	cout<<"doSend"<<endl;
	if ( !doSend( request ) )
	{
		cout<<"send failed"<<endl;
		doClose();
		return false;
	}

//	cout<<"doRecv"<<endl;
	_response.clear();
	if ( !doRecv( _response ) )
	{
		cout<<"recv failed"<<endl;
		doClose();
		return false;
	}

	doClose();

//	cout<<"ParseResponse"<<endl;
	if ( !parseResponse() )
	{
		cout<<"response failed"<<endl;
		return false;
	}

//	cout<<"cookie:"<<_cookie<<endl;

	return true;
}

bool CHttp::parseUrl( const string& url )
{
	string tmp;
	if ( 0 == url.find( HTTP_HEADER ) )
	{
		_ssl = false;
		tmp = url.substr( HTTP_HEADER.size(), url.size()-HTTP_HEADER.size() );
	}
	else if ( 0 == url.find( HTTPS_HEADER ) )
	{
		_ssl = true;
		tmp = url.substr( HTTPS_HEADER.size(), url.size()-HTTPS_HEADER.size() );
	}
	else
	{
		cout<<"http format error:"<<url<<endl;
		return false;
	}

	trimUrl( tmp );

	size_t n = tmp.find( "/" );
	if ( string::npos == n )
	{
		_host = tmp;
		_requestRes = "/";
	}
	else
	{
		_host = tmp.substr( 0, n );
		_requestRes = tmp.substr( n, tmp.size()-n );
	}

	return true;
}

bool CHttp::connectCommon()
{
	_hComm.isSsl = false;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	hostLock();
	map< string, in_addr >::iterator itr = g_mapHost.find( _host );
	if ( g_mapHost.end() == itr )
	{
		hostUnlock();
		hostent* h = gethostbyname( _host.c_str() );
		if ( NULL == h )
		{
			cout<<" can not find host:"<<_host<<endl;
			return false;
		}
		memcpy( &(addr.sin_addr), h->h_addr_list[0], sizeof(addr.sin_addr) );
		hostLock();
		g_mapHost[_host] = addr.sin_addr;
		hostUnlock();
	}
	else
	{
		memcpy( &(addr.sin_addr), &(itr->second), sizeof(addr.sin_addr) );
		hostUnlock();
	}

	_hComm.sock = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( _hComm.sock < 0 )
	{
		cout<<"create socket failed:"<<errno<<endl;
		return false;
	}

	int timeout = 1000*30;
	setsockopt( _hComm.sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout) );
	setsockopt( _hComm.sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout) );

	if ( 0 != connect( _hComm.sock, (sockaddr* )&addr, sizeof(addr) ) )
	{
		cout<<"connect faild:"<<_host<<", error:"<<errno<<endl;
		doClose();
		return false;
	}

	return true;
}
bool CHttp::connectSsl()
{
	SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
	BIO* bio = BIO_new_ssl_connect(ctx);
	SSL* ssl = NULL;
	BIO_get_ssl( bio, &ssl );
	SSL_set_mode( ssl, SSL_MODE_AUTO_RETRY );
	string conn = _host + ":443";
	BIO_set_conn_hostname( bio, conn.data() );
	int timeout = 1000*30;
	if ( BIO_do_connect(bio) <= 0 )
	{
		cout<<"connect failed:"<<ERR_get_error()<<endl;
		return false;
	}
	if(SSL_get_verify_result(ssl) != X509_V_OK)
	{
		cout<<"verify cert failed:"<<ERR_get_error()<<endl;
	}
	int sock = -1;
	BIO_get_fd( bio, &sock );
	setsockopt( sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout) );
	setsockopt( sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout) );
	_hComm.isSsl = true;
	_hComm.ssl = ssl;
	_hComm.ctx = ctx;
	_hComm.bio = bio;
	return true;
}

bool CHttp::doConnect()
{
	if ( !_ssl )
	{
		return connectCommon();
	}
	else
	{
		return connectSsl();
	}
}

void CHttp::doClose()
{
	if ( _hComm.isSsl )
	{
		if ( NULL != _hComm.ctx )
		{
			SSL_CTX_free( _hComm.ctx );
			_hComm.ctx = NULL;
		}
	}
	else
	{
		if ( -1 != _hComm.sock )
		{
#ifdef WIN32
			closesocket(_hComm.sock);
			_hComm.sock = INVALID_SOCKET;
#else
			close( _hComm.sock );
			_hComm.sock = -1;
#endif
		}
	}
}

bool CHttp::sendCommon( const string& data )
{
	int sending = data.size();
	int sent = 0;
	while( sending > 0 )
	{
		int ret = send( _hComm.sock, data.data()+sent, sending, 0 );
		if ( ret <= 0 )
		{
			cout<<"send error:"<<errno<<endl;
			doClose();
			return false;
		}

		sent += ret;
		sending -= ret;
	}

	return true;
}
bool CHttp::sendSsl( const string& data )
{
	int sending = data.size();
	int sent = 0;
	while( sending > 0 )
	{
		int ret = BIO_write( _hComm.bio, data.data()+sent, sending );
		if ( ret <= 0 )
		{
			cout<<"send error:"<<ERR_get_error()<<endl;
			doClose();
			return false;
		}

		sent += ret;
		sending -= ret;
	}

	return true;
}
bool CHttp::recvCommon( string& data )
{
	int ret = 1;
	while( ret > 0 )
	{
		char szTemp[1024] = {0};
		ret = recv( _hComm.sock, szTemp, sizeof(szTemp), 0 );
		if ( ret > 0 )
		{
			data.append( (char*)szTemp, ret );
		}
		else
		{
			cout<<"recv end:"<<ret<<", errno:"<<errno<<endl;
		}
	}

	return true;
}
bool CHttp::recvSsl( string& data )
{
	int ret = 1;
	while( ret > 0 )
	{
		char szTemp[1024] = {0};
		ret = BIO_read( _hComm.bio, szTemp, sizeof(szTemp) );
		if ( ret > 0 )
		{
			data.append( (char*)szTemp, ret );
		}
		else
		{
			cout<<"recv end:"<<ret<<", err:"<<ERR_get_error()<<endl;
		}
	}

	return true;
}

bool CHttp::doSend( const string& data )
{
	if ( _hComm.isSsl )
	{
		return sendSsl( data );
	}
	else
	{
		return sendCommon( data );
	}
}
bool CHttp::doRecv( string& data )
{
	if ( _hComm.isSsl )
	{
		return recvSsl( data );
	}
	else
	{
		return recvCommon( data );
	}
}

bool CHttp::parseResponse()
{
//	cout<<"response size:"<<_response.size()<<endl;
	vector< string > vCookie;
	string tmp = _response;
	size_t n = tmp.find( "\r\n" );
	size_t m = 0;
	while( string::npos != n )
	{
		if ( m == n )
		{
			n += 2;
			break;
		}
		string line = tmp.substr( m, n-m );
		parseLine( line, vCookie );
		m = n + 2;
		n = tmp.find( "\r\n", m );
	}

	if ( !vCookie.empty() )
	{
		_cookie.clear();
		for ( size_t i=0; i<vCookie.size(); ++i )
		{
			_cookie += vCookie[i];
			if ( (i+1) != vCookie.size() )
			{
				_cookie += ";";
			}
		}
	}

	if ( string::npos == n )
	{
		return false;
	}

//	cout<<"n:"<<n<<", size:"<<tmp.size()<<endl;
	_response = tmp.substr( n, tmp.size()-n );
	return true;
}

void CHttp::parseLine( const string& line, vector< string >& vCookie )
{
	string cookieHeader = "Set-Cookie:";
	size_t n = line.find( cookieHeader );
	if ( 0 != n )
	{
		return ;
	}

	n += cookieHeader.size();
	string s = line.substr( n, line.size()-n );
	while( ' ' == s[0] || '	' == s[0] )
	{
		s.erase( 0, 1 );
	}

	vCookie.push_back(s);
}

void CHttp::trimUrl( string& url )
{
	url = Utils::replace( url, "\r", "" );
	url = Utils::replace( url, "\n", "" );
}

void hostLock()
{
	if ( NULL == _hostLocker )
	{
		_hostLocker = new Locker;
		_hostLocker->create();
	}
	_hostLocker->lock();
}
void hostUnlock()
{
	if ( NULL == _hostLocker )
	{
		return ;
	}
	_hostLocker->unlock();
}
