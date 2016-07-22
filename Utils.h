#ifndef _UTILS_H_
#define _UTILS_H_
#include <vector>
#include <string>
#include <map>
#ifndef WIN32
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#else
#include <WinSock2.h>
#include <direct.h>
#endif
#include <time.h>
using namespace std;

const string HTTP_HEADER = "http://";
const string HTTPS_HEADER = "https://";

class Utils
{
public:
	static string createDirByUrl( const string& rootDir, const string& url );
	static string getFileByUrl( const string& rootDir, const string& url );
	static string getDirByUrl( const string& rootDir, const string& url );
	static vector< string > split( const string& data, const string& s );
	static string replace( const string& data, const string& Old, const string& New );
	static void trimAll( string& tmp );
	static string getDateTimeString();
	static void mkDir(const string& dir);
};


#endif //_UTILS_H_
