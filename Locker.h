#ifndef _LOCKER_H_
#define _LOCKER_H_
#ifdef WIN32
#include <Windows.h>
#else
#include <pthread.h>
#endif

class Locker
{
public:
	Locker();
	~Locker();
public:
	void create();
	void destroy();
	void lock();
	void unlock();
private:
#ifdef WIN32
	CRITICAL_SECTION* _locker;
#else
	pthread_mutex_t* _locker;
#endif
};


#endif //_LOCKER_H_
