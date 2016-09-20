/*
 * ssl-init.c
 *
 *  Created on: Aug 31, 2016
 *      Author: amyznikov
 *
 *  https://wiki.openssl.org/index.php/Libcrypto_API
 */
#define _GNU_SOURCE

#include "cuttle/ssl-init.h"
#include "cuttle/ssl-error.h"
#include "cuttle/debug.h"
#include <openssl/crypto.h>
#include <openssl/engine.h>
#include <openssl/conf_api.h>
#include <string.h>
#include <pthread.h>


#define UNUSED(x) (void)(x)


/* defined in ssl-error.c */
extern void cf_init_ssl_error_strings(void);


/*********************************************************************************************************************
 * OpenSSL verstion ssl-int compiled with
 */

int cf_get_opennssl_version_number(void)
{
 return OPENSSL_VERSION_NUMBER;
}

const char * cf_get_opennssl_version_string(void)
{
  return OPENSSL_VERSION_TEXT;
}

const char * cf_get_runtime_opennssl_version_string(void)
{
  return SSLeay_version(SSLEAY_VERSION);
}


/*********************************************************************************************************************
 * OpenSSL in threaded environment
 *  @see http://www.openssl.org/support/faq.html#PROG1
 *  @see http://www.openssl.org/docs/crypto/threads.html
 */
static pthread_rwlock_t * cf_ssl_locks;

static void cf_ssl_thread_id_callback(CRYPTO_THREADID * p)
{
  p->ptr = NULL;
  p->val = pthread_self();
}

static void cf_ssl_pthreads_locking_callback(int mode, int type, const char *file, int line)
{
  UNUSED(file), UNUSED(line);

  if ( !(mode & CRYPTO_LOCK) ) {
    // fprintf(stderr, "unlock %3d from %s:%d", type, file, line);
    pthread_rwlock_unlock(&cf_ssl_locks[type]);
  }
  else if ( mode & CRYPTO_WRITE ) {
    // fprintf(stderr, "wrlock %3d from %s:%d", type, file, line);
    pthread_rwlock_wrlock(&cf_ssl_locks[type]);
  }
  else if ( mode & CRYPTO_READ ) {
    // fprintf(stderr, "rdlock %3d from %s:%d", type, file, line);
    pthread_rwlock_rdlock(&cf_ssl_locks[type]);
  }
  else {
    //errmsg("invalid lock mode 0x%X type=%d from %s:%d", mode, type, file, line);
  }
}

static void cf_ssl_thread_setup(void)
{
  int i, n;

  n = CRYPTO_num_locks();
  cf_ssl_locks = OPENSSL_malloc(n * sizeof(*cf_ssl_locks));
  for ( i = 0; i < n; i++ ) {
    pthread_rwlock_init(&cf_ssl_locks[i], NULL);
  }

  CRYPTO_THREADID_set_callback(cf_ssl_thread_id_callback);
  CRYPTO_set_locking_callback(cf_ssl_pthreads_locking_callback);
}


/*********************************************************************************************************************
 * OpenSSL Engines
 */
/*
static struct {
  const char * name;
  const char * dir;
  const char * sConf;
  ENGINE * engine;
} engines[] = {
    {
        .name = "gost",
        .dir = "../engines/ccgost",
        .sConf = ""
            "openssl_conf = openssl_def\n"
            "\n"
            "[openssl_def]\n"
            "engines = engine_section\n"
            "\n"
            "[engine_section]\n"
            "gost = gost_section\n"
            "\n"
            "[gost_section]\n"
            "default_algorithms = ALL\n"
            "\n"
    },
    {
        .name = "dstu",
        .dir = "../engines/uadstu",
        .sConf = ""
            "openssl_conf = openssl_def\n"
            "\n"
            "[openssl_def]\n"
            "engines = engine_section\n"
            "\n"
            "[engine_section]\n"
            "dstu = dstu_section\n"
            "\n"
            "[dstu_section]\n"
            "default_algorithms = ALL\n"
            "\n"
    }
};

#define GOST_Engine (engines[0].engine)
#define DSTU_Engine (engines[1].engine)

static void cf_ssl_load_engines(void)
{
  for ( size_t i = 0, n = sizeof(engines) / sizeof(engines[0]); i < n; ++i ) {

    CONF * pConfig = NULL;
    BIO * bpConf = NULL;

    char sConf[strlen(engines[i].sConf) + 1];
    long lErrLine;

    bool fok = false;

    strcpy(sConf, engines[i].sConf);

    if ( !(pConfig = NCONF_new(NULL)) ) {
      //CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "NCONF_new() fails");
      goto end;
    }

    if ( !(bpConf = BIO_new_mem_buf(sConf, -1)) ) {
      //CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "BIO_new_mem_buf() fails");
      goto end;
    }

    if ( !NCONF_load_bio(pConfig, bpConf, &lErrLine) ) {
      //CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "NCONF_load_bio() fails");
      goto end;
    }

    if ( !CONF_modules_load(pConfig, NULL, 0) ) {
      //CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "CONF_modules_load() fails");
      goto end;
    }

    if ( !(engines[i].engine = ENGINE_by_id(engines[i].name)) ) {
      //CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "ENGINE_by_id('%s') fails", engines[i].name);
      goto end;
    }

    fok = true;

end : ;

    if ( bpConf ) {
      BIO_free(bpConf);
    }

    if ( pConfig ) {
      NCONF_free(pConfig);
    }

    if ( !fok ) {
//      ERR_print_errors_fp(stderr);
    }
  }

  ERR_clear_error();
}

*/

//static void cf_ssl_thread_cleanup(void)
//{
//  int i, n;
//  CRYPTO_set_locking_callback(NULL);
//  for ( i = 0, n = CRYPTO_num_locks(); i < n; i++ ) {
//    pthread_rwlock_destroy(&cf_ssl_locks[i]);
//  }
//  OPENSSL_free(cf_ssl_locks);
//}


static bool cf_ssl_set_rand_method(void)
{
/*
To make this working, add something like this at the end of openssl.cnf:

[dstu_section]
engine_id = dstu
dynamic_path = /usr/local/lib/engines/libdstu.so
default_algorithms = ALL

[cuttlessl]

# engine_id, that specifies default engine for random number generation.
# NOTE: If this variable is used, all default_algorithms of this engine will
# be available in application even if it is not included into [engine_section].
def_rand_engine = ${dstu_section::engine_id}
*/

  CONF * defConfig = NULL;
  ENGINE * randEngine = NULL;
  long err = 0;
  bool changed = false;
  char * randEngineID;


  if ( !(defConfig = NCONF_new(NULL)) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "NCONF_new() fails");
    goto end;
  }

  if (!NCONF_load(defConfig, CONF_get1_default_config_file(), &err))	{
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "NCONF_load(CONF_get1_default_config_file()) fails");
    goto end;
  }


  if( !(randEngineID = _CONF_get_string(defConfig, "cuttlessl", "def_rand_engine")))  {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "Cannot find $cuttlessl::defRandEngine_engine value in default config.");
    goto end;
  }

  randEngine = ENGINE_by_id(randEngineID);
  if(!randEngine) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "ENGINE_by_id() fails");
    goto end;
  }

  if(!ENGINE_init(randEngine)) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "ENGINE_init() fails");
    goto end;
  }

  if( !ENGINE_get_RAND(randEngine) ) {
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "ENGINE_get_RAND() fails, engine_id = %s", randEngineID);
    goto end;
  }

  if(! ENGINE_register_RAND(randEngine)){
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "ENGINE_register_RAND() fails, engine_id = ", randEngineID);
    goto end;
  }

  if(! ENGINE_set_default(randEngine, ENGINE_METHOD_RAND)){
    CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "ENGINE_set_default(ENGINE_METHOD_RAND) fails, engine_id = ", randEngineID);
    goto end;
  }

  changed = true;
  //CF_INFO("Engine '%s' used as default for random number generation.", defRandEngine);

end:
  if(!changed) {
    CF_SSL_ERR(CF_SSL_ERR_CUTTLE, "Cannot set %s engine as default for random number generation. Method was not changed.", randEngineID);
  }
  ENGINE_free(randEngine);
  _CONF_free_data(defConfig);
  return changed;
}

bool cf_ssl_initialize(void)
{
/*
To load extra engines add the folloving lines (as an example) to openssl.cnf:

#The last line in default section (just before first [...]):

openssl_conf = openssl_def

#at the end of file:

#####################################################################
[openssl_def]
engines = engine_section

#####################################################################
[engine_section]
gost = gost_section
dstu = dstu_section

[gost_section]
engine_id = gost
dynamic_path = /usr/local/lib/engines/libgost.so
default_algorithms = ALL
CRYPT_PARAMS = id-Gost28147-89-CryptoPro-A-ParamSet

[dstu_section]
engine_id = dstu
dynamic_path = /usr/local/lib/engines/libdstu.so
default_algorithms = ALL

*/

  static bool is_initialized = false;
  if ( !is_initialized ) {

    cf_ssl_thread_setup();

    OPENSSL_config(NULL);

    ERR_load_crypto_strings();
    cf_init_ssl_error_strings();

    OPENSSL_load_builtin_modules();
    ENGINE_load_builtin_engines();

    if ( !cf_ssl_set_rand_method() ) {
      CF_SSL_ERR(CF_SSL_ERR_OPENSSL, "cf_ssl_set_rand_method() fails");
      goto end;
    }

    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();
    OpenSSL_add_all_algorithms();

    is_initialized = true;
  }

end:

  return is_initialized;
}

