/*
 * x509.c
 *
 *  Created on: Aug 31, 2016
 *      Author: amyznikov
 *
 *
 *  See openssl/demos/x509/mkcert.c
 */


#include "cuttle/ssl-x509.h"
#include "cuttle/ssl-error.h"
#include <string.h>


static bool cf_x509_add_txt_entry(X509_NAME * name, const char * field, const char * value)
{
  return value ? X509_NAME_add_entry_by_txt(name, field, MBSTRING_ASC, (const uint8_t*) value, -1, -1, 0) : true;
}

X509 * cf_x509_new(const cf_x509_create_args * args)
{
  X509 * x = NULL;
  X509_NAME * name = NULL, * caname = NULL;
  EVP_PKEY * pkey = NULL, * cakey = NULL;
  const EVP_MD * md = NULL;

  bool fok = false;

  if ( !(pkey = args->pkey) ) {
    CF_SSL_ERR(CF_SSL_ERR_INVALID_ARG, "pkey not specified");
    goto end;
  }

  if ( (args->ca && !args->cakey) ) {
    CF_SSL_ERR(CF_SSL_ERR_INVALID_ARG, "cakey not specified");
    goto end;
  }

  if ( args->cakey && !args->ca ) {
    CF_SSL_ERR(CF_SSL_ERR_INVALID_ARG, "ca not specified");
    goto end;
  }

  if ( !(x = X509_new()) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "X509_new() fails");
    goto end;
  }

  X509_set_version(x, 2);
  ASN1_INTEGER_set(X509_get_serialNumber(x), args->serial);
  X509_gmtime_adj(X509_get_notBefore(x), 0);
  X509_gmtime_adj(X509_get_notAfter(x), args->days > 0 ? (long) 60 * 60 * 24 * args->days : (long) 365 * 60 * 60 * 24);
  X509_set_pubkey(x, pkey);

  if ( !(name = X509_get_subject_name(x)) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "X509_get_subject_name() fails");
    goto end;
  }
  if ( !cf_x509_add_txt_entry(name, "C", args->country) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "cf_x509_add_txt_entry('C') fails");
    goto end;
  }
  if ( !cf_x509_add_txt_entry(name, "ST", args->state) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "cf_x509_add_txt_entry('ST') fails");
    goto end;
  }
  if ( !cf_x509_add_txt_entry(name, "L", args->city) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "cf_x509_add_txt_entry('L') fails");
    goto end;
  }
  if ( !cf_x509_add_txt_entry(name, "O", args->company) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "cf_x509_add_txt_entry('O') fails");
    goto end;
  }
  if ( !cf_x509_add_txt_entry(name, "OU", args->department) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "cf_x509_add_txt_entry('OU') fails");
    goto end;
  }
  if ( !cf_x509_add_txt_entry(name, "CN", args->common_name) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "cf_x509_add_txt_entry('CN') fails");
    goto end;
  }

  if ( !args->ca ) {
    /* If self signed set the issuer name to be the same as the subject. */
    if ( !X509_set_issuer_name(x, name) ) {
      CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "X509_set_issuer_name() fails");
      goto end;
    }
  }
  else if ( !(caname = X509_get_subject_name(args->ca) ) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "X509_get_subject_name(ca) fails");
    goto end;
  }
  else if ( !X509_set_issuer_name(x, caname) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "X509_set_issuer_name(caname) fails");
    goto end;
  }
  else if ( !X509_set_subject_name(x, name) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "X509_set_subject_name(name) fails");
    goto end;
  }

  if ( !(md = args->md) ) {
    md = EVP_sha256();
  }

  if ( !(cakey = args->cakey) ) {
    cakey = pkey;    /* do self-sign */
  }

  if ( !X509_sign(x, cakey, md) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "X509_sign() fails");
    goto end;
  }

  fok = true;

end:

  if ( !fok ) {
    cf_x509_free(&x);
  }

  return x;
}


void cf_x509_free(X509 ** x)
{
  if ( x && *x ) {
    X509_free(*x);
    *x = NULL;
  }
}


bool cf_write_pem_x509_fp(X509 * x, FILE * fp)
{
  return PEM_write_X509(fp, x) == 1;
}

X509 * cf_read_pem_x509_fp(FILE * fp)
{
  X509 * x = NULL;
  PEM_read_X509(fp, &x, NULL, NULL);
  return x;
}

bool cf_write_pem_x509(X509 * x, const char * fname)
{
  FILE * fp;
  bool fok = false;

  if ( strcasecmp(fname, "stdout") == 0 ) {
    fp = stdout;
  }
  else if ( strcasecmp(fname, "stderr") == 0 ) {
    fp = stderr;
  }
  else if ( !(fp = fopen(fname, "w")) ) {
    CF_SSL_ERR(CF_SSL_ERR_STDIO, "fopen(%s) fails: %s", fname, strerror(errno));
    goto end;
  }

  fok = cf_write_pem_x509_fp(x, fp);

end:
  if ( fp && fp != stdout && fp != stderr ) {
    fclose(fp);
  }

  return fok;
}

X509 * cf_read_pem_x509(const char * fname)
{
  FILE * fp;
  X509 * x = NULL;

  if ( strcasecmp(fname, "stdin") == 0 ) {
    fp = stdin;
  }
  else if ( !(fp = fopen(fname, "r")) ) {
    CF_SSL_ERR(CF_SSL_ERR_STDIO, "fopen(%s) fails: %s", fname, strerror(errno));
    goto end;
  }

  x = cf_read_pem_x509_fp(fp);

end:
  if ( fp && fp != stdout && fp != stderr ) {
    fclose(fp);
  }

  return x;
}


char * cf_write_pem_x509_str(X509 * x)
{
  char * outbuf = NULL;
  BIO * bio = NULL;
  BUF_MEM * mem = NULL;

  if ( !(bio = BIO_new(BIO_s_mem())) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "BIO_new(BIO_s_mem()) fails");
    goto end;
  }

  if ( PEM_write_bio_X509(bio, x) <= 0 ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "PEM_write_bio_X509() fails");
    goto end;
  }

  BIO_get_mem_ptr(bio, &mem);
  if ( !mem ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "BIO_get_mem_ptr() fails");
    goto end;
  }

  if ( !(outbuf = OPENSSL_malloc(mem->length+1)) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "OPENSSL_malloc(%zu) fails", mem->length + 1);
    goto end;
  }

  memcpy(outbuf, mem->data, mem->length);
  outbuf[mem->length] = 0;

end:

  if ( bio ) {
    BIO_free(bio);
  }

  return outbuf;
}

X509 * cf_read_pem_x509_str(const char * s)
{
  X509 * x = NULL;
  BIO * bio = NULL;

  bool fOk = false;

  if ( !s || !*s ) {
    CF_SSL_ERR(CF_SSL_ERR_INVALID_ARG, "No string provided");
    goto end;
  }

  if ( !(bio = BIO_new_mem_buf(s, -1)) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "BIO_new_mem_buf() fails");
    goto end;
  }

  if ( !PEM_read_bio_X509(bio, &x, NULL, NULL) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "PEM_read_bio_X509() fails");
    goto end;
  }

  fOk = true;

end:

  if ( bio ) {
    BIO_free(bio);
  }

  if ( x && !fOk ) {
    cf_x509_free(&x);
  }

  return x;
}

