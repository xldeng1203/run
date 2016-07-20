#include <iostream>
#include <fstream>
#include "http.h"
#include <map>
#include <list>
#include "Utils.h"
#include "Locker.h"
#ifdef WIN32
#else
#include <pthread.h>
#endif
using namespace std;

struct ImageInfo
{
	string url;
	long sn;
};

string dateDir = Utils::getDateTimeString();
string IMG_DIR = "img";
string HTML_DIR = "html";
string PNG_DIR = "png";
string GIF_DIR = "gif";

map< string, bool > g_visitedUrl;
map< string, bool > g_visitedImg;
list< string > g_url;
list< ImageInfo* > g_img;
string g_header;
static Locker _locker;

const int THREAD_NUM = 1;
bool g_finished = false;

void parseWebUrl( const string& data );
void parseWebImg( const string& data );
void scan( const string& url );
void downloadImg();
string getImageName( ImageInfo* info );
void initLocker();
void lock();
void unlock();
void uninitLocker();
void addImg( const string& url );
ImageInfo* getImg();
#ifdef WIN32
UINT threadProc( void* p );
vector< HANDLE > _vThreads;
#else
void* threadProc( void* p );
vector< pthread_t > _vThreads;
#endif

int main( int argc, char** argv )
{
	if ( argc < 2 )
	{
		cout<<"usage:scan url"<<endl;
		return -1;
	}

#ifdef WIN32
	WSADATA wsaData;
	memset(&wsaData, 0, sizeof(wsaData));
	::WSAStartup(0x0202, &wsaData);
#endif

	string url = argv[1];
	size_t n = url.find( HTTP_HEADER );
	if ( 0 == n )
	{
		url.erase( 0, HTTP_HEADER.size() );
	}
	n = url.find( "/" );
	if ( string::npos != n )
	{
		g_header = HTTP_HEADER + url.substr( 0, n );
	}
	else
	{
		g_header = HTTP_HEADER + url;
	}
	g_header += "/";

	string rootDir = "./" + dateDir;
	Utils::mkDir( rootDir );

	IMG_DIR = rootDir + "/" + IMG_DIR;
	HTML_DIR = rootDir + "/" + HTML_DIR;
	PNG_DIR = rootDir + "/" + PNG_DIR;
	GIF_DIR = rootDir + "/" + GIF_DIR;

	Utils::mkDir(IMG_DIR);
	Utils::mkDir(HTML_DIR);
	Utils::mkDir(PNG_DIR);
	Utils::mkDir(GIF_DIR);

	initLocker();

	for ( int i=0; i<THREAD_NUM; ++i )
	{
#ifdef WIN32
		HANDLE h = ::CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)threadProc,
			NULL, 0, NULL );
		_vThreads.push_back(h);
#else
		pthread_t tid;
		pthread_create( &tid, NULL, threadProc, NULL );
		_vThreads.push_back(tid);
#endif
	}

	scan(argv[1]);

	while( !g_url.empty() )
	{
		string url = g_url.front();
		scan(url);
		g_url.pop_front();
		cout<<"left num:"<<g_url.size()<<endl;
	}

	g_finished = true;

	cout<<"send finished, now waiting for thread exit"<<endl;
	for ( size_t i=0; i<_vThreads.size(); ++i )
	{
#ifdef WIN32
		::WaitForSingleObject( _vThreads[0], INFINITE );
#else
		void* status = NULL;
		pthread_join( _vThreads[i], &status );
#endif
	}

	cout<<"uninitLocker start"<<endl;
	uninitLocker();
	cout<<"uninitLocker finished"<<endl;

#ifdef WIN32
	::WSACleanup();
#endif

	return 0;
}


void parseWebUrl( const string& data )
{
	string tag = "href=\"";
	size_t n = 0;
	size_t m = 0;
	n = data.find( tag, m );
	while( string::npos != n )
	{
		n += tag.size();
		size_t np = data.find( "\"", n );
		if ( string::npos != np )
		{
			string url = data.substr( n, np-n );
			m = np+1;
			if ( 0 != url.find( HTTP_HEADER ) )
			{
				url = g_header + url;
			}
			if ( g_visitedUrl.end() == g_visitedUrl.find(url) )
			{
				g_url.push_back(url);
			}
		}
		else
		{
			m = n;
		}

		n = data.find( tag, m );
	}
}
void parseWebImg( const string& data )
{
	string tag = "<img ";
	string att1 = "src=\"";
	string att2 = "lazy-src=\"";
	size_t n = 0;
	size_t m = 0;
	size_t n0 = 0;
	n0 = data.find( tag, m );
	while( string::npos != n0 )
	{
		n0 += tag.size();
		size_t n2 = data.find( att2, n0 );
		if ( string::npos == n2 || n2 > data.find( ">", n0 ) )
		{
			n = data.find( att1, n0 );
			if ( string::npos == n )
			{
				m = n0;
				n0 = data.find( tag, m );
				continue;
			}
			else
			{
				n += att1.size();
			}
		}
		else
		{
			n += n2 + att2.size();
		}

		size_t np = data.find( "\"", n );
		if ( string::npos != np )
		{
			string url = data.substr( n, np-n );
			if ( 0 != url.find( HTTP_HEADER ) )
			{
				url = g_header + url;
			}
			if ( g_visitedImg.end() == g_visitedImg.find(url) )
			{
				g_visitedImg[url] = true;
				addImg( url );
			}

			m = np+1;
		}
		else
		{
			m = n;
		}

		n0 = data.find( tag, m );
	}
}

void scan( const string& url )
{
	if ( g_visitedUrl.end() != g_visitedUrl.find(url) )
	{
		return ;
	}
	cout<<"scan:"<<url<<endl;
	g_visitedUrl[url] = true;
	CHttp http;
	if ( !http.request( url ) )
	{
		cout<<"request url failed:"<<url<<endl;
		return ;
	}
	cout<<"request finished:"<<url<<endl;

	string htmlFile = Utils::getFileByUrl( HTML_DIR, url );
	htmlFile += ".html";
//	cout<<"html:"<<htmlFile<<endl;
	ofstream fout( htmlFile.data() );
	if ( fout.is_open() )
	{
		fout<<http.getResponse();
		fout.close();
	}

	cout<<"parseWebUrl"<<endl;
	parseWebUrl(http.getResponse());

	cout<<"parseWebImg"<<endl;
	parseWebImg(http.getResponse());
	
	cout<<"parseWebImg"<<endl;
}

void doDownload( ImageInfo* info )
{
	string url = info->url;
	string filePath;
	if ( string::npos != url.find( ".gif" ) )
	{
		filePath = GIF_DIR + "/" + getImageName( info );
	}
	else if ( string::npos != url.find( ".png" ) )
	{
		filePath = PNG_DIR + "/" + getImageName( info );
	}
	else
	{
		filePath = IMG_DIR + "/" + getImageName( info );
	}
	CHttp http;
	cout<<"request:"<<url<<endl;
	if ( !http.request( url ) )
	{
		cout<<"request failed:"<<url<<endl;
		return ;
	}
	cout<<"request finished"<<endl;

	const string& data = http.getResponse();
	if ( data.size() >= 1024 )
	{
		ofstream fout( filePath.data(), ios::binary );
		if ( fout.is_open() )
		{
			fout.write( data.data(), data.size() );
			fout.close();
		}
	}
}

static long g_imgSn = 0;
string getImageName( ImageInfo* info )
{
	string url = info->url;
	size_t n = url.rfind( '.' );
	string suffix;
	if ( string::npos != n )
	{
		suffix = url.substr( n, url.size()-n );
		if ( suffix.size() > 6 )
		{
			suffix.clear();
		}
	}

	char szTemp[50] = {0};
	sprintf( szTemp, "%09ld", info->sn );
	string file = szTemp;
	file += suffix;
	return file;
}

void initLocker()
{
	_locker.create();
}
void lock()
{
	_locker.lock();
}
void unlock()
{
	_locker.unlock();
}
void uninitLocker()
{
	_locker.destroy();
}
void addImg( const string& url )
{
	ImageInfo* info = new ImageInfo;
	info->url = url;
	info->sn = ++g_imgSn;
	lock();
	g_img.push_back(info);
	unlock();
}

ImageInfo* getImg()
{
	lock();
	if ( g_img.empty() )
	{
		unlock();
		return NULL;
	}
	ImageInfo* info = g_img.front();
	g_img.pop_front();
	unlock();
	return info;
}

void doThreadProc()
{
	while( true )
	{
#ifdef WIN32
		::Sleep(10);
#else
		usleep(1000*10);
#endif
		ImageInfo* info;
		while( NULL != (info=getImg()) )
		{
			doDownload(info);
			delete info;
			info = NULL;
		}

		if ( g_finished )
		{
			break;
		}
	}

	cout<<"thread exit"<<endl;
}
#ifdef WIN32
UINT threadProc( void* p )
{
	doThreadProc();
	return 0;
}
#else
void* threadProc( void* p )
{
	doThreadProc();
	pthread_exit(NULL);
	return NULL;
}
#endif
