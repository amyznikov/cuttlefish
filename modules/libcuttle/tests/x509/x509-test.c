/*
 * x509-test.c
 *
 *  Created on: Aug 31, 2016
 *      Author: amyznikov
 */

#include "cuttle/ssl-init.h"
#include "cuttle/ssl-pkey.h"
#include "cuttle/ssl-x509.h"
#include <string.h>


int main(int argc, char *argv[])
{
  EVP_PKEY * pk = NULL;
  X509 * x509 = NULL;

  const char * keytype = NULL;
  const char * keyparams = NULL;

  for ( int i = 1; i < argc; ++i ) {

    if ( strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0 ) {

      printf("Usage:\n");
      printf("  x509-test "
          "[keytype=<keytype>] "
          "[keyparams=<keyparams>] "
          " \n");

      printf("keytypes: \n");
      printf("  rsa dstu4145le gost94 gost2001 ...\n");

      return 0;
    }

    if ( strncmp(argv[i], "keytype=", 8) == 0 ) {
      keytype = argv[i] + 8;
    }
    else if ( strncmp(argv[i], "keyparams=", 10) == 0 ) {
      keytype = argv[i] + 10;
    }
    else {
      fprintf(stderr, "Invalid argument %s\n", argv[i]);
      return 1;
    }
  }


  /////////////////

  if ( !cf_ssl_initialize() ) {
    fprintf(stderr, "cf_ssl_initialize() fails\n");
    ERR_print_errors_fp(stderr);
    goto end;
  }





  /////////////////

  if ( !(pk = cf_ssl_pkey_new(keytype, keyparams, NULL)) ) {
    fprintf(stderr, "cf_ssl_pkey_new() fails\n");
    ERR_print_errors_fp(stderr);
    goto end;
  }

  if ( !cf_write_pem_private_key_fp(pk, stdout) ) {
    fprintf(stderr, "cf_write_pem_private_key_fp() fails\n");
    ERR_print_errors_fp(stderr);
    goto end;
  }

  if ( !cf_write_pem_public_key_fp(pk, stdout) ) {
    fprintf(stderr, "cf_write_pem_public_key_fp() fails\n");
    ERR_print_errors_fp(stderr);
    goto end;
  }



  /////////////////

  x509 = cf_ssl_x509_new( &(struct cf_x509_create_args ) {
        .md = NULL,
        .pkey = pk,
        .country = "UA",
        .state = "Kiev distr",
        .city = "Kiev",
        .company = "Special-IS",
        .department = "A.Myznikov",
        .common_name = "amyznikov",
        .email = "andrey.myznikov@gmail.com",
        .serial = 1,
        .days = 365,
      });

  if ( !x509 ) {
    fprintf(stderr, "cf_ssl_x509_new() fails\n");
    ERR_print_errors_fp(stderr);
    goto end;
  }

  if ( !cf_write_pem_x509_fp(x509, stdout) ) {
    fprintf(stderr, "cf_write_pem_x509_fp() fails\n");
    ERR_print_errors_fp(stderr);
    goto end;
  }



end:

  return 0;
}

