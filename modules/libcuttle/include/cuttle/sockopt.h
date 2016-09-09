/*
 * cuttle/sockopt.h
 *
 *  Created on: Oct 6, 2015
 *      Author: amyznikov
 */


#ifndef __cuttle_sockopt_h__
#define __cuttle_sockopt_h__

#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>


#ifdef __cplusplus
extern "C" {
#endif


int so_get_error(int so);

socklen_t so_get_addrslen(int af);

bool so_set_send_bufsize(int so, int size);
int so_get_send_bufsize(int so, int * size);

bool so_set_recv_bufsize(int so, int size);
int so_get_recv_bufsize(int so, int * size);

bool so_set_send_timeout(int so, int sec);
int so_get_send_timeout(int so, int * sec);

bool so_set_recv_timeout(int so, int sec);
int so_get_recv_timeout(int so, int * sec);

bool so_set_nodelay(int so, int optval);
int so_get_nodelay(int so, int * optval);

bool so_set_reuse_addrs(int so, int optval);
int so_get_reuse_addrs(int so, int * optval);

bool so_is_listening(int so);

bool so_set_non_blocking(int so, int optval);

bool so_set_keepalive(int so, int keepalive, int keepidle, int keepintvl, int keepcnt);
bool so_get_keepalive(int so, int * keepalive, int * keepidle, int * keepintvl, int * keepcnt);

int so_get_outq_size(int so);

bool so_close(int so, bool abort_conn);



#ifdef __cplusplus
}
#endif

#endif /* __cuttle_sockopt_h__ */
