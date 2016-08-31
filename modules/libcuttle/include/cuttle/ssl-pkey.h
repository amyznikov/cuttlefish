/*
 * ssl-pkey.h
 *
 *  Created on: Aug 31, 2016
 *      Author: amyznikov
 */

//#pragma once

#ifndef __cuttle_ssl_pkey_h__
#define __cuttle_ssl_pkey_h__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Public/Private key pair generation
 *  ctype examples: rsa dstu4145le gost94 gost2001 etc
 */
EVP_PKEY * cf_ssl_pkey_new(const char * ctype, const char * params, EVP_PKEY * pubkey /*=NULL*/);

/** Calls EVP_PKEY_free() */
void cf_ssl_pkey_free(EVP_PKEY ** key);


/* EVP_PKEY Read/Write utily */

bool cf_write_pem_public_key(EVP_PKEY * pkey, const char * filename);
EVP_PKEY * cf_read_pem_public_key(const char * filename);

bool cf_write_pem_public_key_fp(EVP_PKEY * pkey, FILE * fp);
EVP_PKEY * cf_read_pem_public_key_fp(FILE * fp);

bool cf_write_pem_private_key(EVP_PKEY * pkey, const char * filename);
EVP_PKEY * cf_read_pem_private_key(const char * filename);

bool cf_write_pem_private_key_fp(EVP_PKEY * pkey, FILE * fp);
EVP_PKEY * cf_read_pem_private_key_fp(FILE * fp);

bool cf_write_pem_private_key_enc(EVP_PKEY * pkey, const char * filename, const char * enc, const char * psw);
EVP_PKEY * cf_read_pem_private_key_enc(const char * filename, const char * psw);

bool cf_write_pem_private_key_enc_fp(EVP_PKEY * pkey, FILE * fp, const char * enc, const char * psw);
EVP_PKEY * cf_read_pem_private_key_enc_fp(FILE * fp, const char * psw);

char * cf_write_pem_public_key_str(EVP_PKEY * pkey);
EVP_PKEY * cf_read_pem_public_key_str(const char * pem_public_key);

char * cf_write_pem_private_key_str(EVP_PKEY * pkey);
EVP_PKEY * cf_read_pem_private_key_str(const char * pem_private_key);

size_t cf_write_public_key_bits(EVP_PKEY * pkey, /*out*/ uint8_t ** buf);
EVP_PKEY * cf_read_public_key_bits(const uint8_t bits[], size_t cbbits);

size_t cf_write_private_key_bits(EVP_PKEY * pkey, /*out*/ uint8_t ** buf);
EVP_PKEY * cf_read_private_key_bits(const uint8_t bits[], size_t cbbits);

char * cf_write_public_key_hex_str(EVP_PKEY * pkey);
EVP_PKEY * cf_read_public_key_hex_str(const char * hex);

char * cf_write_private_key_hex_str(EVP_PKEY * pkey);
EVP_PKEY * cf_read_private_key_hex_str(const char * hex);

int cf_write_public_key_hex_fp(EVP_PKEY * pkey, FILE * fp);
EVP_PKEY * cf_read_public_key_hex_fp(FILE * fp);

int cf_write_private_key_hex_fp(EVP_PKEY * pkey, FILE * fp);
EVP_PKEY * cf_read_private_key_hex_fp(FILE * fp );

int cf_write_public_key_hex(EVP_PKEY * pkey, const char * filename);
EVP_PKEY * cf_read_public_key_hex(const char * filename);

int cf_write_private_key_hex(EVP_PKEY * pkey, const char * filename);
EVP_PKEY * cf_read_private_key_hex(const char * filename);


#ifdef __cplusplus
}
#endif

#endif /* __cuttle_ssl_pkey_h__ */
