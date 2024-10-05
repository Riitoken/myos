// myos.cpp

/*
The bare basics for udp sockets, and threads, and clocks, and locks.
2024-Summer - Wrapper from original source circa 2004 - Ray Bornert
*/

#include "myos.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <process.h>
#endif

#ifdef __linux
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#endif

struct global_s
{
	#ifdef _WIN32
	int ret;
	#endif
	global_s()
	{
#ifdef _WIN32
		static WSADATA _wsadata;
		ret = ::WSAStartup( MAKEWORD(2,2), &_wsadata ); // expect 0 (zero) on success
		// https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-wsastartup
		// https://learn.microsoft.com/en-us/windows/win32/winsock/windows-sockets-error-codes-2
#endif
	}

	~global_s()
	{
#ifdef _WIN32
		::WSACleanup();
#endif
	}
};
global_s g;

///////////////////////////////////////////
// ERRORS
///////////////////////////////////////////
int my_get_last_error()
{
	#ifdef _WIN32
	return (int)::WSAGetLastError();
	#endif

	#ifdef __linux
	return errno;
	#endif
}

///////////////////////////////////////////
// SOCKETS
///////////////////////////////////////////

int my_socket_okay()
{
	#ifdef _WIN32
		return g.ret;
	#endif
	return my_ok;
}

int my_setsockopt_broadcast( int s, const bool yn )
{
	int enable=(true==yn);
	int ret = ::setsockopt(s, SOL_SOCKET, SO_BROADCAST, (char*)&enable, sizeof(enable) );
	if(0>ret)
		return my_error;
	return my_ok;
}
int my_setsockopt_send_timeout( int s, const int nms )
{
	timeval tv;
	tv.tv_sec = nms/1000;
	tv.tv_usec = nms%1000;
	int ret = ::setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv) );
	if(0>ret)
		return my_error;
	return my_ok;
}

/*
Set the socket I/O mode.
The FIONBIO cmd enables or disables the socket blocking mode.
mode == 0, blocking is enabled; 
mode != 0, non-blocking mode is enabled.
*/
int set_nonblocking( int s, const bool val )
{
	#ifdef _WIN32
	unsigned long mode = (val) ? 1 : 0;
	int res = ioctlsocket(s, FIONBIO, &mode);
	if (res != NO_ERROR)
		return my_error;
	#endif
	return my_ok;
}

int my_setsockopt_recv_timeout( int s, const int nms )
{
	set_nonblocking(s,true);

	timeval tv;
	tv.tv_sec = nms/1000;
	tv.tv_usec = nms%1000;
	int ret = ::setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(tv) );
	if(0>ret)
		return my_error;
	return my_ok;
}

int my_make_address6(mysockadr_s& msa, const char* ipaddr6, const char* sport)
{
	memset(&msa, 0, sizeof(msa));

	sockaddr_in6* psa = (sockaddr_in6*)(&msa);
	sockaddr_in6& sa = *psa;

	sa.sin6_family = AF_INET6;
	unsigned short port = atoi(sport);
	sa.sin6_port = htons(port);
	int res = inet_pton(AF_INET6, ipaddr6, &sa.sin6_addr);
	if(0>res)
		return my_error; // error
	if(0==res)
		return my_invalid_address; // invalid address
	return my_ok;
};

int my_make_address( mysockadr_s& msa, const char* ipaddr, const char* sport )
{
	memset(&msa,0,sizeof(msa));

	sockaddr_in* psa = (sockaddr_in*)(&msa);
	sockaddr_in& sa = *psa;

	sa.sin_family = AF_INET;
	unsigned short port = atoi(sport);
	sa.sin_port = htons(port);
	int res = inet_pton(AF_INET, ipaddr, &sa.sin_addr);
	if(0> res)
		return my_error; // error
	if(0==res)
		return my_invalid_address; // invalid address
	return my_ok;
};

int my_sockadr_to_text( const mysockadr_s& msa, char sip[16], char sport[6] )
{
	// Convert the address from mysockadr_s to sockaddr_in
	const sockaddr_in* addr_in = (const sockaddr_in*)(&msa);

	// Convert IP address from binary to string form
	if (NULL == inet_ntop(AF_INET, &(addr_in->sin_addr), sip, 16))
		return my_error; // Conversion failed

	// Convert port from network byte order to host byte order
	unsigned short port = ntohs(addr_in->sin_port);
	sprintf(sport, "%u", port);

	return my_ok;
}

int my_sockadr_to_text6( const mysockadr_s& msa, char sip[48], char sport[6] )
{
	// Convert the address from mysockadr_s to sockaddr_in
	const sockaddr_in6* addr_in = (const sockaddr_in6*)(&msa);

	// Convert IP address from binary to string form
	if (NULL == inet_ntop(AF_INET6, &(addr_in->sin6_addr), sip, 48))
		return my_error; // Conversion failed

	// Convert port from network byte order to host byte order
	unsigned short port = ntohs(addr_in->sin6_port);
	sprintf(sport, "%u", port);

	return my_ok;
}

int my_bind_socket( int& s, const mysockadr_s& msa )
{
	int res = ::bind( s, (const sockaddr*)(&msa), sizeof(mysockadr_s) );
	if(0>res)
		return my_error;
	return my_ok;
}

int my_make_udp_socket( int& s )
{
	s = (int)::socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP );
	if(0>s)
		return my_error; // call my_get_last_error();
	return my_ok;
}
int my_make_udp_socket6( int& s )
{
	s = (int)::socket( AF_INET6, SOCK_DGRAM, IPPROTO_UDP );
	if(0>s)
		return my_error; // call my_get_last_error();
	return my_ok;
}

/*
	This is a user friendly courtesy function.
	It handles the 3 functions that are most typical:
	  forming the socket local address
	  creating the socket
	  binding the socket to the local address
*/
int my_make_udp_bound_socket( int& s, const char* ipaddr, const char* port )
{
	mysockadr_s msa;
	int ret = my_make_address( msa,ipaddr,port );
	if(my_ok != ret)
		return ret;

	ret = my_make_udp_socket(s);
	if(my_ok != ret)
		return ret;

	ret = my_bind_socket(s,msa);
	if(my_ok != ret)
		return ret;

	return my_ok;
}

/*
	This is a user friendly courtesy function.
	It handles the 3 functions that are most typical:
	  forming the socket local address
	  creating the socket
	  binding the socket to the local address
*/
int my_make_udp_bound_socket6( int& s, const char* ipaddr, const char* port )
{
	mysockadr_s msa;
	int ret = my_make_address6( msa,ipaddr,port );
	if(my_ok != ret)
		return ret;

	ret = my_make_udp_socket6(s);
	if(my_ok != ret)
		return ret;

	ret = my_bind_socket(s,msa);
	if(my_ok != ret)
		return ret;

	return my_ok;
}


int my_close_socket( int s )
{
	#ifdef _WIN32
	int ret = ::closesocket(s);
	#endif

	#ifdef __linux
	int ret = close(s);
	#endif

	if(0>ret)
		return my_error;

	return my_ok;
}

//int my_sendto( int s, const char* buf, const size_t nbytes, const mysockadr_s& to )
int my_sendto( int s, const char* buf, const int nbytes, const mysockadr_s& to )
{
	int flags = 0;
	int iresult = ::sendto( s, buf, (int)nbytes, flags, (const sockaddr*)(&to), sizeof(mysockadr_s) );
	return iresult;
}

//int my_recvfrom( int s, char* buf, const size_t nbytes, mysockadr_s& from )
int my_recvfrom( int s, char* buf, const int nbytes, mysockadr_s& from )
{
	int flags = 0;

	#ifdef _WIN32
	int len=sizeof(mysockadr_s);
	int ret = ::recvfrom( s, buf, (int)nbytes, flags, (sockaddr*)(&from), &len );
	#endif

	#ifdef __linux
	socklen_t len = sizeof(mysockadr_s);
	ssize_t ret = recvfrom( s,buf,nbytes,flags, (sockaddr*)(&from), &len );
	#endif

	if(0>ret)
		return my_error; // some socket eror

	if(0==ret)
		return my_socket_shutdown; // socket was gracefully shutdown

	return ret; // nbytes received
}

///////////////////////////////////////////
// THREADS
///////////////////////////////////////////
#ifdef _WIN32
struct osthread
{
	uintptr_t tid;
	mythread_s mt;
	void reset()
	{
		tid = 0;
		mt.address=NULL;
		mt.param=NULL;
	}
	osthread()
	{
		reset();
	}
	//DESTRUCTOR
	~osthread()
	{
		stop();
	}
	virtual void main()
	{
		(mt.address)(mt.param);
	}
	static void main_static( void *obj )
	{
		osthread *pthis = (osthread*)obj;
		if (NULL!=pthis)
			pthis->main();
	}
	virtual void start( mythread_s& m )
	{
		stop();
		mt=m;
		tid = _beginthread( main_static, 0, this );
	}
	virtual void stop()
	{
		::CloseHandle( (HANDLE)tid );
		reset();
	}
}; // class
#endif // _WIN32

#ifdef __linux
struct osthread
{
	pthread_t	_pthread;
	pthread_attr_t	_pthread_attr;
	mythread_s mt;

public:

	osthread() : _pthread(0)
	{
		pthread_attr_init( &_pthread_attr );
	}

	//DESTRUCTOR
	~osthread()
	{
		stop();
	}

	virtual void main()
	{
		(mt.address)(mt.param);
	}

	static void* main_static( void *obj )
	{
		//the param is a Sound object
		osthread *pthis = (osthread*)obj;
		if (pthis)
			pthis->main();
		return NULL;
	}

	virtual int start( mythread_s& m )
	{
		stop();
		mt=m;

		//bring the thread into existance
		int result = pthread_create
		(
			  &_pthread
			, &_pthread_attr
			, main_static
			, this //better be the base class or there will be trouble
		);
		return (result == 0);
	}

	virtual void stop()
	{
		if(0!=_pthread)
		{
			pthread_cancel( _pthread );
			_pthread=0;
		}
	}
}; // struct
#endif // LINUX

osthread the_thread[my_max_threads];

int my_thread_stop( const unsigned int id )
{
	if(id >= my_max_threads)
		return my_bad_index;
	the_thread[id].stop();
	return my_ok;
}
int my_thread_start( const unsigned int id, mythread_s mt )
{
	if(id >= my_max_threads)
		return my_bad_index;
	the_thread[id].start(mt);
	return my_ok;
}

void my_sleep( const int millisec )
{
#ifdef _WIN32
	::Sleep(millisec);
#else
	timespec ts;
	ts.tv_sec = millisec / 1000;
	ts.tv_nsec = (millisec % 1000) *1000*1000;
	::nanosleep( &ts,NULL );
#endif
}

///////////////////////////////////////////
// LOCKS
///////////////////////////////////////////
#ifdef _WIN32
struct mylock_s
{
	CRITICAL_SECTION m_cs;
	mylock_s() { ::InitializeCriticalSection( &m_cs ); }
	~mylock_s() { ::DeleteCriticalSection( &m_cs ); }
	void open() { ::EnterCriticalSection( &m_cs ); }
	void close() { ::LeaveCriticalSection( &m_cs ); }
};
#endif

#ifdef __linux
struct mylock_s
{
	pthread_mutex_t m_cs;
	mylock_s() { m_cs = PTHREAD_MUTEX_INITIALIZER; }
	~mylock_s() {  }
	void open() { ::pthread_mutex_unlock( &m_cs ); }
	void close() { ::pthread_mutex_lock( &m_cs ); }
};
#endif

mylock_s the_lock[my_max_locks];

void my_lock_open( const unsigned int id )
{
	if(my_max_locks <= id)
		return;
	the_lock[id].open();
}
void my_lock_close( const unsigned int id )
{
	if(my_max_locks <= id)
		return;
	the_lock[id].close();
}
