#include "Utils.h"
#ifdef WIN32
#pragma comment( lib, "ws2_32.lib")
#endif

string Utils::createDirByUrl( const string& rootDir, const string& url )
{
	string tmp = url;
	if ( 0 == tmp.find( HTTP_HEADER ) )
	{
		tmp.erase( 0, HTTP_HEADER.size() );
	}

	replace( tmp, "-", "_" );
	replace( tmp, ":", "_" );
	replace( tmp, " ", "_" );
	replace( tmp, "	", "_" );
	replace( tmp, "\\", "_" );
	replace( tmp, "*", "_" );
	replace( tmp, "?", "_" );
	replace( tmp, "\"", "_" );
	replace( tmp, "<", "_" );
	replace( tmp, ">", "_" );
	replace( tmp, "|", "_" );

	vector< string > vDir = split( tmp, "/" );
	string dir = rootDir;
	for ( size_t i=0; i<vDir.size()-1; ++i )
	{
		dir += "/";
		dir += vDir[i];
		mkDir( dir );
	}


	return ( dir + "/" + vDir[vDir.size()-1] );
}
string Utils::getFileByUrl( const string& rootDir, const string& url )
{
	string tmp = url;
	if ( 0 == tmp.find( HTTP_HEADER ) )
	{
		tmp.erase( 0, HTTP_HEADER.size() );
	}

	trimAll( tmp );

	string file = rootDir + "/" + tmp;

	return file;
}
string Utils::getDirByUrl( const string& rootDir, const string& url )
{
	string tmp = url;
	if ( 0 == tmp.find( HTTP_HEADER ) )
	{
		tmp.erase( 0, HTTP_HEADER.size() );
	}

	trimAll( tmp );

	string dir = rootDir + "/" + tmp;
	mkDir(dir);
	
	return dir;
}
vector< string > Utils::split( const string& data, const string& s )
{
	vector< string > v;
	size_t n = 0;
	size_t m = 0;
	n = data.find( s, m );
	while( string::npos != n )
	{
		string tmp = data.substr( m, n-m );
		m = n + s.size();
		n = data.find( s, m );
		v.push_back( tmp );
	}

	if ( m < data.size() )
	{
		v.push_back( data.substr( m, data.size()-m ) );
	}

	return v;
}

string Utils::replace( const string& data, const string& Old, const string& New )
{
	string ret;
	size_t n = 0;
	size_t m = 0;
	n = data.find( Old, m );
	while( string::npos != n )
	{
		string tmp = data.substr( m, n-m );
		ret += tmp;
		ret += New;
		m = n + Old.size();
		n = data.find( Old, m );
	}

	if ( m < data.size() )
	{
		ret += data.substr( m, data.size()-m );
	}

	return ret;
}
void Utils::trimAll( string& tmp )
{
	tmp = replace( tmp, "-", "_" );
	tmp = replace( tmp, ":", "_" );
	tmp = replace( tmp, " ", "_" );
	tmp = replace( tmp, "	", "_" );
	tmp = replace( tmp, "\\", "_" );
	tmp = replace( tmp, "*", "_" );
	tmp = replace( tmp, "?", "_" );
	tmp = replace( tmp, "\"", "_" );
	tmp = replace( tmp, "<", "_" );
	tmp = replace( tmp, ">", "_" );
	tmp = replace( tmp, "|", "_" );
	tmp = replace( tmp, "/", "_" );
	tmp = replace( tmp, ";", "_" );
	tmp = replace( tmp, "&", "_" );
	tmp = replace( tmp, "%", "_" );
	tmp = replace( tmp, "\r", "_" );
	tmp = replace( tmp, "\n", "_" );
}

string Utils::getDateTimeString()
{
	char szTime[100] = { 0 };
#ifdef WIN32
	SYSTEMTIME st;
	memset(&st, 0, sizeof(st));
	::GetLocalTime(&st);
	sprintf(szTime, "%04d_%02d_%02d_%02d_%02d_%02d",
		st.wYear,
		st.wMonth,
		st.wDay,
		st.wHour,
		st.wMinute,
		st.wSecond );
#else
	time_t ts = time(NULL);
	tm t;
	localtime_r( &ts, &t );
	sprintf( szTime, "%04d_%02d_%02d_%02d_%02d_%02d",
			t.tm_year+1900,
			t.tm_mon+1,
			t.tm_mday,
			t.tm_hour,
			t.tm_min,
			t.tm_sec );
#endif

	return string( szTime );
}

void Utils::mkDir(const string& dir)
{
#ifdef WIN32
	_mkdir(dir.data());
#else
	mkdir(dir.data(), 0777);
#endif
}
