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


// self-signed:
// $ x509-test keytype=rsa pkeyout=ca.pem certout=ca.cer CN=rootCA OU="Special-IS CA"

// issued:
// $ x509-test keytype=rsa pkeyout=domain.pem certout=domain.cer ca=ca.cer cakey=ca.pem

int main(int argc, char *argv[])
{
  EVP_PKEY * pk = NULL;
  X509 * x509 = NULL;

  X509 * ca = NULL;
  EVP_PKEY * cakey = NULL;

  const char * keytype = NULL;
  const char * keyparams = NULL;

  const char * pkeyout = "stdout";
  const char * pubkeyout = "stdout";
  const char * certout = "stdout";

  const char * cafile = NULL;
  const char * cakeyfile = NULL;


  const char * subj_C = "UA";
  const char * subj_ST = "Kiev district";
  const char * subj_L = "Kiev";
  const char * subj_O = "Special-IS";
  const char * subj_OU = "Home";
  const char * subj_CN = "amyznikov";

  for ( int i = 1; i < argc; ++i ) {

    if ( strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0 ) {

      printf("Usage:\n");
      printf("  x509-test "
          "[keytype=<keytype>] "
          "[keyparams=<keyparams>] "
          "[pkeyout=<output-private-key.pem>] "
          "[pubkeyout=<output-public-key.pem>] "
          "[certout=<output-cert.cer>] "
          "[ca=<ca.cer>] "
          "[cakey=<cakey.pem>] "
          "[C=<Country-Code>] "
          "[ST=<State-Or-Province>] "
          "[L=<Location-Or-City>] "
          "[O=<Organization>] "
          "[OU=<Organization-Unit>] "
          "[CN=<Common-Name>] "
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
    else if ( strncmp(argv[i], "pkeyout=", 8) == 0 ) {
      pkeyout = argv[i] + 8;
    }
    else if ( strncmp(argv[i], "pubkeyout=", 10) == 0 ) {
      pubkeyout = argv[i] + 10;
    }
    else if ( strncmp(argv[i], "certout=", 8) == 0 ) {
      certout = argv[i] + 8;
    }
    else if ( strncmp(argv[i], "ca=", 3) == 0 ) {
      cafile = argv[i] + 3;
    }
    else if ( strncmp(argv[i], "cakey=", 6) == 0 ) {
      cakeyfile = argv[i] + 6;
    }
    else if ( strncmp(argv[i], "C=", 2) == 0 ) {
      subj_C = argv[i] + 2;
    }
    else if ( strncmp(argv[i], "ST=", 3) == 0 ) {
      subj_ST = argv[i] + 3;
    }
    else if ( strncmp(argv[i], "L=", 2) == 0 ) {
      subj_L = argv[i] + 2;
    }
    else if ( strncmp(argv[i], "O=", 2) == 0 ) {
      subj_O = argv[i] + 2;
    }
    else if ( strncmp(argv[i], "OU=", 3) == 0 ) {
      subj_OU = argv[i] + 3;
    }
    else if ( strncmp(argv[i], "CN=", 3) == 0 ) {
      subj_CN = argv[i] + 3;
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


  if ( cafile && !(ca = cf_read_pem_x509(cafile)) ) {
    fprintf(stderr, "cf_read_pem_x509(%s) fails\n", cafile);
    ERR_print_errors_fp(stderr);
    goto end;
  }


  if ( cakeyfile && !(cakey = cf_read_pem_private_key(cakeyfile)) ) {
    fprintf(stderr, "cf_read_pem_private_key(%s) fails\n", cakeyfile);
    ERR_print_errors_fp(stderr);
    goto end;
  }


  /////////////////

  if ( !(pk = cf_pkey_new(keytype, keyparams, NULL)) ) {
    fprintf(stderr, "cf_ssl_pkey_new() fails\n");
    ERR_print_errors_fp(stderr);
    goto end;
  }

  if ( !cf_write_pem_private_key(pk, pkeyout) ) {
    fprintf(stderr, "cf_write_pem_private_key(%s) fails\n", pkeyout);
    ERR_print_errors_fp(stderr);
    goto end;
  }

  if ( !cf_write_pem_public_key(pk, pubkeyout) ) {
    fprintf(stderr, "cf_write_pem_public_key(%s) fails\n", pubkeyout);
    ERR_print_errors_fp(stderr);
    goto end;
  }



  /////////////////

  x509 = cf_x509_new( &(struct cf_x509_create_args ) {
        .md = NULL,
        .pkey = pk,
        .ca = ca,
        .cakey = cakey,
        .country = subj_C,
        .state = subj_ST,
        .city = subj_L,
        .company = subj_O,
        .department = subj_OU,
        .common_name = subj_CN,
        .email = "andrey.myznikov@gmail.com",
        .serial = 1,
        .days = 365,
      });

  if ( !x509 ) {
    fprintf(stderr, "cf_ssl_x509_new() fails\n");
    ERR_print_errors_fp(stderr);
    goto end;
  }

  if ( !cf_write_pem_x509(x509, certout) ) {
    fprintf(stderr, "cf_write_pem_x509(%s) fails\n", certout);
    ERR_print_errors_fp(stderr);
    goto end;
  }



end:

  return 0;
}

