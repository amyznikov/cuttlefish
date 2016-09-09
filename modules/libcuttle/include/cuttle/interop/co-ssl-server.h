/*
 * co-ssl-server.h
 *
 *  Created on: Sep 9, 2016
 *      Author: amyznikov
 */

// #pragma once

#ifndef __cuttle_interop_tcp_server_h__
#define __cuttle_interop_tcp_server_h__

#include <cuttle/sockopt.h>
#include <cuttle/cothread/ssl.h>


#ifdef __cplusplus
extern "C" {
#endif


typedef
struct co_ssl_server
  co_ssl_server;

typedef
struct co_ssl_server_context {
  co_ssl_server * ssrv;
  co_ssl_socket * ssl_sock;
} co_ssl_server_context;


typedef
struct co_ssl_server_port_opts {
  sockaddr_type bind_address;
  SSL_CTX * ssl_ctx;
  int sock_type;
  int proto;
  void (*on_accepted)(co_ssl_server_context * context);
  int accepted_stack_size;
} co_ssl_server_port_opts;


typedef
struct co_ssl_server_opts {
  SSL_CTX * ssl_ctx;
} co_ssl_server_opts;


co_ssl_server * co_ssl_server_new(const co_ssl_server_opts * opts);
bool co_ssl_server_add_port(co_ssl_server * ssrv, const co_ssl_server_port_opts * opts);
bool co_ssl_server_start(co_ssl_server * ssrv);
bool co_ssl_server_stop(co_ssl_server * ssrv);
void co_ssl_server_destroy(co_ssl_server ** ssrv);




#ifdef __cplusplus
}
#endif

#endif /* __cuttle_interop_tcp_server_h__ */
