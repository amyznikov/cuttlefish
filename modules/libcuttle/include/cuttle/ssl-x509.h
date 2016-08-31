/*
 * x509.h
 *
 *  Created on: Aug 31, 2016
 *      Author: amyznikov
 */

// #pragma once

#ifndef __cuttle_x509_h__
#define __cuttle_x509_h__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509v3.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef
struct cf_x509_create_args {
  const EVP_MD * md;
  EVP_PKEY * pkey;
  const char * country;
  const char * state;
  const char * city;
  const char * company;
  const char * department;
  const char * common_name;
  const char * email;
  int serial;
  int days;
} cf_x509_create_args;

X509 * cf_ssl_x509_new(const cf_x509_create_args * args);
void cf_ssl_x509_free(X509 ** x);


bool cf_write_pem_x509_fp(X509 * x, FILE * fp);
X509 * cf_read_pem_x509_fp(FILE * fp);

bool cf_write_pem_x509(X509 * x, const char * fname);
X509 * cf_read_pem_x509(const char * fname);



#ifdef __cplusplus
}
#endif

#endif /* __cuttle_x509_h__ */
