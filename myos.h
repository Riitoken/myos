#ifndef _myos_h_
#define _myos_h_

///////////////////////////////////////////
// ERRORS
///////////////////////////////////////////
enum my_ret
{
	 my_ok              =  0 // all is well
	,my_error           = -1 // general error
	,my_invalid_address = -2 // the socket address was invalid
	,my_socket_shutdown = -3 // the socket was gracefully closed
	,my_bad_index       = -4 // the ID was out of bounds
};

int my_get_last_error(); // more detail for general errors

///////////////////////////////////////////
// SOCKETS
///////////////////////////////////////////
struct mysockadr_s
{
	unsigned char a[48];
};

int my_socket_okay();

int my_make_udp_socket( int& s );
int my_make_udp_socket6( int& s );

int my_make_address( mysockadr_s& sa, const char* ipaddr, const char* sport );
int my_make_address6( mysockadr_s& sa, const char* ipaddr, const char* sport );

int my_sockadr_to_text( const mysockadr_s& sa, char sip[16], char sport[6] );
int my_sockadr_to_text6( const mysockadr_s& sa, char sip[48], char sport[6] );

int my_bind_socket( int& s, const mysockadr_s& msa );

int my_make_udp_bound_socket( int& s, const char* ipaddr, const char* sport );
int my_make_udp_bound_socket6( int& s, const char* ipaddr, const char* sport );

int my_setsockopt_broadcast( int s, const bool yn );

int my_setsockopt_send_timeout( int s, const int nms );

int my_setsockopt_recv_timeout( int s, const int nms );

int my_sendto( int s, const char* buf, const int nbytes, const mysockadr_s& to );

int my_recvfrom( int s, char* buf, const int nbytes, mysockadr_s& from);

int my_close_socket( int s );

///////////////////////////////////////////
// THREADS
///////////////////////////////////////////
#define my_max_threads (10) // [0,9]

struct mythread_s
{
	void (*address)(void*);
	void *param;
	mythread_s():address(0),param(0){}
	mythread_s( void (*a)(void*), void* p):address(a),param(p){}
};

int my_thread_start( const unsigned int id, mythread_s mt );

int my_thread_stop( const unsigned int id );

void my_sleep( const int millisec );

///////////////////////////////////////////
// LOCKS
///////////////////////////////////////////
#define my_max_locks (10) // [0,9]

void my_lock_open( const unsigned int id );

void my_lock_close( const unsigned int id );

#endif
