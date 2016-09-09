/*
 * co-tcp-server.c
 *
 *  Created on: Sep 9, 2016
 *      Author: amyznikov
 */

#include <cuttle/debug.h>
#include <cuttle/interop/co-ssl-server.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include "../ccarray.h"

#define CO_SERVER_LISTENING_THREAD_STACK_SIZE         (256*1024)
#define CO_SERVER_ACCEPTED_THREAD_STACK_SIZE       (2*1024*1024)



typedef
struct co_listening_thread_context {
  struct co_ssl_server * ssrv; // back pointer
  sockaddr_type bind_address;
  void (*on_accepted)(co_ssl_server_context * context);
  SSL_CTX * ssl_ctx;
  int sock_type;
  int transport_proto;
  co_socket * listening_sock;
} co_listening_thread_context;




typedef
struct co_accepted_thread_context {
  struct co_ssl_server * ssrv; // back pointer
  SSL_CTX * ssl_ctx;
  co_socket * accepted_sock;
  void (*on_accepted)(co_ssl_server_context * context);
} co_accepted_thread_context;




struct co_ssl_server {
  ccarray_t listening_threads; // <co_listening_thread_context*>
  SSL_CTX * ssl_ctx;
};





static void co_server_accepted_thread(void * arg)
{
  co_accepted_thread_context * accepted_ctx = arg;
  bool fok = false;

  struct co_ssl_server_context server_ctx = {
    .ssrv = accepted_ctx->ssrv,
    .ssl_sock = NULL,
  };

  if ( !(server_ctx.ssl_sock = co_ssl_socket_accept(accepted_ctx->accepted_sock, accepted_ctx->ssl_ctx)) ) {
    CF_CRITICAL("co_ssl_socket_accept() fails");
    goto end;
  }

  accepted_ctx->on_accepted(&server_ctx);
  fok = true;

end:

  if ( fok ) {
    co_ssl_socket_close(&server_ctx.ssl_sock, false);
  }
  else if ( !server_ctx.ssl_sock ) {
    co_socket_close(&accepted_ctx->accepted_sock, true);
  }
  else {
    co_ssl_socket_close(&server_ctx.ssl_sock, true);
  }

  free(accepted_ctx);
}


static bool start_accepted_thread(co_listening_thread_context * listening_context, co_socket * accepted_sock)
{
  co_accepted_thread_context * accepted_context;
  bool fok = false;

  if ( !(accepted_context = calloc(1, sizeof(*accepted_context))) ) {
    CF_CRITICAL("calloc(accepted_thread_context) fails: %s", strerror(errno));
    goto end;
  }

  accepted_context->accepted_sock = accepted_sock;
  accepted_context->ssrv = listening_context->ssrv;
  accepted_context->ssl_ctx = listening_context->ssl_ctx;
  accepted_context->on_accepted = listening_context->on_accepted;

  if ( !co_schedule(co_server_accepted_thread, accepted_context, CO_SERVER_ACCEPTED_THREAD_STACK_SIZE) ) {
    CF_CRITICAL("co_schedule(accepted_thread) fails: %s", strerror(errno));
    goto end;
  }

  fok = true;

end:

  if ( !fok ) {
    free(accepted_context);
  }

  return fok;
}



static void co_server_listening_thread(void * arg)
{
  co_listening_thread_context * ctx = arg;

  co_socket * listening_sock = ctx->listening_sock;
  co_socket * accepted_sock = NULL;

  CF_DEBUG("Started");


  while ( 42 ) {

    bool fok = false;

    if ( !(accepted_sock = co_socket_accept_new(listening_sock, NULL, 0)) ) {
      CF_CRITICAL("co_ssl_socket_accept() fails");
    }
    else if ( !start_accepted_thread(ctx, accepted_sock) ) {
      CF_CRITICAL("start_accepted_thread() fails");
    }
    else {
      fok = true;
    }

    if ( !fok ) {
      co_socket_close(&accepted_sock, true);
    }
  }

  CF_DEBUG("Finished");
}





co_ssl_server * co_ssl_server_new(const co_ssl_server_opts * opts)
{
  (void)(opts);

  co_ssl_server * ssrv = NULL;
  bool fok = false;

  if ( !(ssrv = calloc(1, sizeof(*ssrv))) ) {
    CF_SSL_ERR(CF_SSL_ERR_MALLOC,"calloc(co_srv) fails: %s", strerror(errno));
    goto end;
  }

  if ( !ccarray_init(&ssrv->listening_threads, 32, sizeof(co_listening_thread_context*)) ) {
    CF_SSL_ERR(CF_SSL_ERR_MALLOC,"ccarray_init(listening_threads) fails: %s", strerror(errno));
    goto end;
  }

  if ( opts && opts->ssl_ctx ) {
    ssrv->ssl_ctx = opts->ssl_ctx;
  }

  fok = true;

end:

  if ( !fok && ssrv ) {
    co_ssl_server_destroy(&ssrv);
  }

  return ssrv;
}

void co_ssl_server_destroy(co_ssl_server ** co_srv)
{
  if ( co_srv && *co_srv ) {
    ccarray_cleanup(&(*co_srv)->listening_threads);
    free(*co_srv);
    *co_srv = NULL;
  }
}


bool co_ssl_server_add_port(co_ssl_server * ssrv, const co_ssl_server_port_opts * opts)
{
  co_listening_thread_context * listening_context = NULL;
  bool fok = false;

  if ( ccarray_size(&ssrv->listening_threads) >= ccarray_capacity(&ssrv->listening_threads) ) {
    errno = ENOBUFS;
    goto end;
  }

  if ( !(listening_context = calloc(1, sizeof(*listening_context))) ) {
    goto end;
  }

  memcpy(&listening_context->bind_address, &opts->bind_address, sizeof(opts->bind_address));
  listening_context->ssrv = ssrv;
  listening_context->on_accepted = opts->on_accepted;
  listening_context->sock_type = opts->sock_type;
  listening_context->transport_proto = opts->proto;

  if ( !(listening_context->ssl_ctx = opts->ssl_ctx) ) {
    listening_context->ssl_ctx = ssrv->ssl_ctx;
  }


  ccarray_ppush_back(&ssrv->listening_threads, listening_context);
  fok = true;

end:

  return fok;
}


bool co_ssl_server_start(co_ssl_server * ssrv)
{
  co_listening_thread_context * listening_context;
  bool fok = true;

  for ( size_t i = 0, n = ccarray_size(&ssrv->listening_threads); i < n; ++i ) {

    listening_context = ccarray_ppeek(&ssrv->listening_threads, i);

    listening_context->listening_sock = co_ssl_listen(&listening_context->bind_address.sa,
        listening_context->sock_type, listening_context->transport_proto);

    if ( !listening_context->listening_sock ) {
      CF_FATAL("co_ssl_listen() fails");
      fok = false;
      break;
    }

    if ( !co_schedule(co_server_listening_thread, listening_context, CO_SERVER_LISTENING_THREAD_STACK_SIZE) ) {
      CF_FATAL("co_schedule(listening_thread) fails: %s", strerror(errno));
      co_socket_close(&listening_context->listening_sock, false);
      fok = false;
      break;
    }
  }

  return fok;
}

bool co_ssl_server_stop(co_ssl_server * ssrv)
{
  (void)(ssrv);
  return false;
}

