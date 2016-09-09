/*
 * cuttle/cothread/co-ssl.h
 *
 *  Created on: Sep 8, 2016
 *      Author: amyznikov
 *
 *  SSL BIO for cothread
 */

// #pragma once

#ifndef __cuttle_co_ssl_h__
#define __cuttle_co_ssl_h__

#include <cuttle/ssl/ssl-context.h>
#include <cuttle/cothread/scheduler.h>


#ifdef __cplusplus
extern "C" {
#endif

BIO_METHOD * BIO_co_socket(void);
BIO * BIO_co_socket_new(co_socket * cc);

SSL * co_ssl_new(SSL_CTX * ssl_ctx, co_socket * cc);
void co_ssl_free(SSL ** ssl);



typedef
struct co_ssl_socket
  co_ssl_socket;

/* takes ownership of so */
co_ssl_socket * co_ssl_socket_new(co_socket * cc, SSL_CTX * ssl_ctx);
void co_ssl_socket_close(co_ssl_socket ** ssl_sock, bool abort_conn);

void co_ssl_socket_set_send_timeout(co_ssl_socket * ssl_sock, int msec);
void co_ssl_socket_set_recv_timeout(co_ssl_socket * ssl_sock, int msec);

ssize_t co_ssl_socket_send(co_ssl_socket * ssl_sock, const void * buf, size_t size);
ssize_t co_ssl_socket_recv(co_ssl_socket * ssl_sock, void * buf, size_t size);


/** TODO: add additional options for new SSL object */
co_ssl_socket * co_ssl_socket_accept(co_socket * cc, SSL_CTX * ssl_ctx);
co_socket * co_ssl_tcp_listen(const struct sockaddr * addrs);
co_ssl_socket * co_ssl_tcp_connect(const struct sockaddr * address, int tmo_ms, SSL_CTX * ssl_ctx);
co_ssl_socket * co_ssl_tcp_server_connect(const char * server, uint16_t port, int tmo_ms, SSL_CTX * ssl_ctx);




#ifdef __cplusplus
}
#endif

#endif /* __cuttle_co_ssl_h__ */
