/* 
  gcc decrypt.c -I../openssl-1.0.0a/include -l crypto -o decrypt -g
*/

#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

#define ERROR(X)  {printf("[ERROR] iteration %d: ", i); printf X;fflush(stdout);}
#define PAGESIZE 1024
#define PBKDF2_ITER 4000
#define FILE_HEADER_SZ 16

int main(int argc, char **argv) {
  const char* infile = "sqlcipher.db";
  const char* outfile = "sqlite.db";
  char *pass= "test123";
  int i, csz, tmp_csz, key_sz, iv_sz;
  FILE *infh, *outfh;
  int read, written;
  unsigned char *inbuffer, *outbuffer, *salt, *out, *key, *iv;
  EVP_CIPHER *evp_cipher;
  EVP_CIPHER_CTX ectx;

  OpenSSL_add_all_algorithms();

  evp_cipher = (EVP_CIPHER *) EVP_get_cipherbyname("aes-256-cbc");

  key_sz = EVP_CIPHER_key_length(evp_cipher);
  key = malloc(key_sz);

  iv_sz = EVP_CIPHER_iv_length(evp_cipher);
  iv = malloc(iv_sz);

  inbuffer = (unsigned char*) malloc(PAGESIZE);
  outbuffer = (unsigned char*) malloc(PAGESIZE);
  salt = malloc(FILE_HEADER_SZ);

  infh = fopen(infile, "r");
  outfh = fopen(outfile, "w");
  
  read = fread(inbuffer, 1, PAGESIZE, infh);  /* read the first page */
  memcpy(salt, inbuffer, FILE_HEADER_SZ); /* first 16 bytes are the random database salt */

  PKCS5_PBKDF2_HMAC_SHA1(pass, strlen(pass), salt, FILE_HEADER_SZ, PBKDF2_ITER, key_sz, key);
 
  memset(outbuffer, 0, PAGESIZE);
  out = outbuffer;

  memcpy(iv, inbuffer + PAGESIZE - iv_sz, iv_sz); /* last iv_sz bytes are the initialization vector */
 
  EVP_CipherInit(&ectx, evp_cipher, NULL, NULL, 0);
  EVP_CIPHER_CTX_set_padding(&ectx, 0);
  EVP_CipherInit(&ectx, NULL, key, iv, 0);
  EVP_CipherUpdate(&ectx, out, &tmp_csz, inbuffer + FILE_HEADER_SZ, PAGESIZE - iv_sz - FILE_HEADER_SZ);
  csz = tmp_csz;  
  out += tmp_csz;
  EVP_CipherFinal(&ectx, out, &tmp_csz);
  csz += tmp_csz;
  EVP_CIPHER_CTX_cleanup(&ectx);

  fwrite("SQLite format 3\0", 1, FILE_HEADER_SZ, outfh);
  fwrite(outbuffer, 1, PAGESIZE - FILE_HEADER_SZ, outfh);

  printf("wrote page %d\n", 0);

  for(i = 1; (read = fread(inbuffer, 1, PAGESIZE, infh)) > 0 ;i++) {
    memcpy(iv, inbuffer + PAGESIZE - iv_sz, iv_sz); /* last iv_sz bytes are the initialization vector */
    memset(outbuffer, 0, PAGESIZE);
    out = outbuffer;
 
    EVP_CipherInit(&ectx, evp_cipher, NULL, NULL, 0);
    EVP_CIPHER_CTX_set_padding(&ectx, 0);
    EVP_CipherInit(&ectx, NULL, key, iv, 0);
    EVP_CipherUpdate(&ectx, out, &tmp_csz, inbuffer, PAGESIZE - iv_sz);
    csz = tmp_csz;  
    out += tmp_csz;
    EVP_CipherFinal(&ectx, out, &tmp_csz);
    csz += tmp_csz;
    EVP_CIPHER_CTX_cleanup(&ectx);

    fwrite(outbuffer, 1, PAGESIZE, outfh);
    printf("wrote page %d\n", i);
  }
  
  fclose(infh);
  fclose(outfh);
 
  free(inbuffer);
  free(outbuffer);
  free(key);
  free(salt);
  free(iv);
  
  return 0;
}

