#include "Locker.h"

Locker::Locker()
: _locker(NULL)
{
}
Locker::~Locker()
{
	destroy();
}
void Locker::create()
{
	if ( NULL == _locker )
	{
#ifdef WIN32
		_locker = new CRITICAL_SECTION;
		::InitializeCriticalSection( _locker );
#else
		_locker = new pthread_mutex_t;
		pthread_mutex_init( _locker, NULL );
#endif
	}
}
void Locker::destroy()
{
	if ( NULL != _locker )
	{
#ifdef WIN32
		::DeleteCriticalSection( _locker );
#else
		pthread_mutex_destroy( _locker );
#endif
		delete _locker;
		_locker = NULL;
	}
}
void Locker::lock()
{
	if ( NULL == _locker )
	{
		return ;
	}
#ifdef WIN32
	::EnterCriticalSection( _locker );
#else
	pthread_mutex_lock( _locker );
#endif
}
void Locker::unlock()
{
	if ( NULL == _locker )
	{
		return ;
	}
#ifdef WIN32
	::LeaveCriticalSection( _locker );
#else
	pthread_mutex_unlock( _locker );
#endif
}
