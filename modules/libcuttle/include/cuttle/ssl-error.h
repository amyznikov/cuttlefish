/*
 * ssl-error.h
 *
 *  Created on: Aug 31, 2016
 *      Author: amyznikov
 */

// #pragma once

#ifndef __cf_ssl_error_h__
#define __cf_ssl_error_h__

#include <openssl/conf.h>
#include <openssl/err.h>


#ifdef __cplusplus
extern "C" {
#endif


#ifdef OPENSSL_NO_ERR

  #define CF_SSL_ERR(r,...)

#else /* OPENSSL_NO_ERR */


#define CF_SSL_ERR_OPENSSL       100
#define CF_SSL_ERR_INVALID_ARG   101
#define CF_SSL_ERR_STDIO         102
#define CF_SSL_ERR_MALLOC        103
#define CF_SSL_ERR_PTHREAD       104
#define CF_SSL_ERR_EPOLL         105
#define CF_SSL_ERR_CUTTLE        106


#define CF_SSL_ERR(r,...)  \
  cf_ssl_error(__func__, r, __FILE__, __LINE__, __VA_ARGS__)


void cf_init_ssl_error_strings(void);

void cf_ssl_error(const char * func, int reason, char * file, int line, char * msg, ...)
  __attribute__ ((__format__ (__printf__, 5, 6)));


#endif /* OPENSSL_NO_ERR */


#ifdef __cplusplus
}
#endif

#endif /* __cf_ssl_error_h__ */
