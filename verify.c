/* 
  verifies per-page HMAC on a SQLCipher 3 database at the page level, bypassing sqlite internals 

  SQLCipher 3:
  clang verify.c -DPAGESIZE=1024 -DPBKDF2_ITER=64000 -DHMAC_SHA1  -I /usr/local/opt/openssl/include -L /usr/local/opt/openssl/lib -l crypto -o verify

  SQLCipher 4:
  clang verify.c -DPAGESIZE=4096 -DPBKDF2_ITER=256000 -DHMAC_SHA512  -I /usr/local/opt/openssl/include -L /usr/local/opt/openssl/lib -l crypto -o verify
*/

#include <stdio.h>
#include <unistd.h> 
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

#define FILE_HEADER_SZ 16

#ifndef PAGESIZE
#define PAGESIZE 1024
#endif

#ifndef PBKDF2_ITER
#define PBKDF2_ITER 64000
#endif

#ifndef HMAC_SALT_MAS
#define HMAC_SALT_MASK 0x3a
#endif

#ifndef FAST_PBKDF2_ITER
#define FAST_PBKDF2_ITER 2
#endif

#if defined(HMAC_SHA512)
#define HMAC_FUNC EVP_sha512()
#elif defined(HMAC_SHA256)
#define HMAC_FUNC EVP_sha256()
#else
#define HMAC_FUNC EVP_sha1()
#endif

#if (defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
static HMAC_CTX *HMAC_CTX_new(void)
{
  HMAC_CTX *ctx = OPENSSL_malloc(sizeof(*ctx));
  if (ctx != NULL) {
    HMAC_CTX_init(ctx);
  }
  return ctx;
}

static void HMAC_CTX_free(HMAC_CTX *ctx)
{
  if (ctx != NULL) {
    HMAC_CTX_cleanup(ctx);
    OPENSSL_free(ctx);
  }
}
#endif

const char *usage =
        "Usage: verify [options]\n"
        "\n"
        "Options :\n"
        "\n"
        "\t-f filename input filename (default sqlcipher.db).\n"
        "\t-k key      key (default testkey).\n"
        "\t-h          help (this text)\n"
        "\n"
        "\n"
        ;
const char *optstring = "hf:k:";

int parse_args(int argc, char **argv, char **key, char **file) 
{
        int c;

        while((c = getopt(argc, argv, optstring)) != -1) {
                switch (c) {
                case 'f':
                        *file = optarg;
                        break;
                case 'k':
                        *key = optarg;
                        break;
                case 'h':
                default:
                        fputs(usage, stderr);
                        return 0;
                }
        }

        return 1;
}


int main(int argc, char **argv) {
  char* file = "sqlcipher.db";
  char *pass= "testkey";
  int i, rc, key_sz, iv_sz, block_sz, hmac_sz, reserve_sz, read, problems = 0;
  unsigned int outlen;
  FILE *infh;
  unsigned char *buffer, *salt, *hmac_salt, *out, *key, *hmac_key;
  EVP_CIPHER *evp_cipher;
  HMAC_CTX* hctx = NULL;

  if (!parse_args(argc, argv, &pass, &file)) return 1;

#if (defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER < 0x10100000L) || (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER < 0x20700000L)
  OpenSSL_add_all_algorithms();
#endif

  evp_cipher = (EVP_CIPHER *) EVP_get_cipherbyname("aes-256-cbc");

  key_sz = EVP_CIPHER_key_length(evp_cipher);
  key = malloc(key_sz);
  hmac_key = malloc(key_sz);

  iv_sz = EVP_CIPHER_iv_length(evp_cipher);
  hmac_sz = EVP_MD_size(HMAC_FUNC);
  block_sz = EVP_CIPHER_block_size(evp_cipher);
  reserve_sz = iv_sz + hmac_sz;
  reserve_sz = ((reserve_sz % block_sz) == 0) ? reserve_sz : ((reserve_sz / block_sz) + 1) * block_sz; 
               
  buffer = (unsigned char*) malloc(PAGESIZE);
  out = (unsigned char*) malloc(hmac_sz);
  salt = (unsigned char*) malloc(FILE_HEADER_SZ);
  hmac_salt = (unsigned char*) malloc(FILE_HEADER_SZ);

  infh = fopen(file, "rb");

  if(infh == NULL) {
    printf("unable to open input file %s\n", file);
    goto error;
  }

  read = fread(buffer, 1, PAGESIZE, infh);  
  memcpy(salt, buffer, FILE_HEADER_SZ); 
  memcpy(hmac_salt, buffer, FILE_HEADER_SZ); 
  
  for(i = 0; i < FILE_HEADER_SZ; i++) {
    hmac_salt[i] ^= HMAC_SALT_MASK;
  }

  if(!PKCS5_PBKDF2_HMAC((const char *)pass, strlen(pass), salt, FILE_HEADER_SZ, PBKDF2_ITER, HMAC_FUNC, key_sz, key)) goto error;
  if(!PKCS5_PBKDF2_HMAC((const char *)key, key_sz, hmac_salt, FILE_HEADER_SZ, FAST_PBKDF2_ITER, HMAC_FUNC, key_sz, hmac_key)) goto error;

  i = 1;
  if((hctx = HMAC_CTX_new()) == NULL) goto error;
  if(!HMAC_Init_ex(hctx, hmac_key, key_sz, HMAC_FUNC, NULL)) goto error;
  if(!HMAC_Update(hctx, buffer + FILE_HEADER_SZ, PAGESIZE - FILE_HEADER_SZ - reserve_sz + iv_sz)) goto error;
  if(!HMAC_Update(hctx, (const unsigned char *) &i, sizeof(int))) goto error;
  if(!HMAC_Final(hctx, out, &outlen)) goto error;
  HMAC_CTX_free(hctx);

  if(memcmp(out, buffer + PAGESIZE - reserve_sz + iv_sz, outlen) != 0) {
    problems++;
    printf("page %d is invalid\n", i);
  } 

  for(i = 2; (read = fread(buffer, 1, PAGESIZE, infh)) > 0; i++) {
    if((hctx = HMAC_CTX_new()) == NULL) goto error;
    if(!HMAC_Init_ex(hctx, hmac_key, key_sz, HMAC_FUNC, NULL)) goto error;
    if(!HMAC_Update(hctx, buffer, PAGESIZE - reserve_sz + iv_sz)) goto error;
    if(!HMAC_Update(hctx, (const unsigned char *) &i, sizeof(int))) goto error;
    if(!HMAC_Final(hctx, out, &outlen)) goto error;
    HMAC_CTX_free(hctx);

    if(memcmp(out, buffer + PAGESIZE - reserve_sz + iv_sz, outlen) != 0) {
      problems++;
      printf("page %d is invalid\n", i);
    } 
  } 

rc = 0;

if(problems > 0) {
  printf("scanned %d pages and found %d invalid, database is corrupt\n", i-1, problems);
} else {
  printf("scanned %d pages and 0 are invalid, database is intact\n", i-1);
}
goto close;

error:
  printf("an error occurred\n");
  rc = 1;  

close:
  if(infh != NULL) fclose(infh);
  if(buffer != NULL) free(buffer);
  if(key != NULL) free(key);
  if(hmac_key != NULL) free(hmac_key);
  if(salt != NULL) free(salt);
  if(hmac_salt != NULL) free(hmac_salt);
  
  return rc;
}

