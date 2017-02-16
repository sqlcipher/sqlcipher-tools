/* 
  decrypts a sqlcipher version 1 database at the raw page level, bypassing sqlite internals completely

  gcc decrypt.c -I../openssl-1.0.0a/include -l crypto -o decrypt -g
*/

#include <stdio.h>
#include <unistd.h> 
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

#define ERROR(X)  {printf("[ERROR] iteration %d: ", i); printf X;fflush(stdout);}
#define PAGESIZE 1024
#define PBKDF2_ITER 64000
#define FILE_HEADER_SZ 16

const char *usage =
        "Usage: decrypt [options]\n"
        "\n"
        "Options :\n"
        "\n"
        "\t-f filename input filename (default sqlcipher.db).\n"
        "\t-o filename output filename (default sqlite.db).\n"
        "\t-k key      key (default testkey).\n"
        "\t-h          help (this text)\n"
        "\n"
        "\n"
        ;
const char *optstring = "hi:o:k:";

int parse_args(int argc, char **argv, char **key, char **input, char **output) 
{
        int c;

        while((c = getopt(argc, argv, optstring)) != -1) {
                switch (c) {
                case 'i':
                        *input = optarg;
                        break;
                case 'o':
                        *output = optarg;
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
  char* infile = "sqlcipher.db";
  char* outfile = "sqlite.db";
  char *pass= "testkey";
  int i, csz, tmp_csz, key_sz, iv_sz, block_sz, hmac_sz, reserve_sz;
  FILE *infh, *outfh;
  int read, written;
  unsigned char *inbuffer, *outbuffer, *salt, *out, *key, *iv;
  EVP_CIPHER *evp_cipher;
  EVP_CIPHER_CTX ectx;

  if (!parse_args(argc, argv, &pass, &infile, &outfile)) return 1;

  OpenSSL_add_all_algorithms();

  evp_cipher = (EVP_CIPHER *) EVP_get_cipherbyname("aes-256-cbc");

  key_sz = EVP_CIPHER_key_length(evp_cipher);
  key = malloc(key_sz);

  iv_sz = EVP_CIPHER_iv_length(evp_cipher);
  iv = malloc(iv_sz);
 
  hmac_sz = EVP_MD_size(EVP_sha1());

  block_sz = EVP_CIPHER_block_size(evp_cipher);

  reserve_sz = iv_sz + hmac_sz;
  reserve_sz = ((reserve_sz % block_sz) == 0) ? reserve_sz : ((reserve_sz / block_sz) + 1) * block_sz; 
               
  inbuffer = (unsigned char*) malloc(PAGESIZE);
  outbuffer = (unsigned char*) malloc(PAGESIZE);
  salt = malloc(FILE_HEADER_SZ);

  infh = fopen(infile, "rb");

  if(infh == NULL) {
    printf("unable to open input file %s\n", infile);
    exit(1);
  }

  outfh = fopen(outfile, "wb");
  if(outfh == NULL) {
    printf("unable to open output file %s\n", outfile);
    exit(1);
  }
 
  read = fread(inbuffer, 1, PAGESIZE, infh);  /* read the first page */
  memcpy(salt, inbuffer, FILE_HEADER_SZ); /* first 16 bytes are the random database salt */

  PKCS5_PBKDF2_HMAC_SHA1(pass, strlen(pass), salt, FILE_HEADER_SZ, PBKDF2_ITER, key_sz, key);
 
  memset(outbuffer, 0, PAGESIZE);
  out = outbuffer;

  memcpy(iv, inbuffer + PAGESIZE - reserve_sz, iv_sz); /* last iv_sz bytes are the initialization vector */
 
  EVP_CipherInit(&ectx, evp_cipher, NULL, NULL, 0);
  EVP_CIPHER_CTX_set_padding(&ectx, 0);
  EVP_CipherInit(&ectx, NULL, key, iv, 0);
  EVP_CipherUpdate(&ectx, out, &tmp_csz, inbuffer + FILE_HEADER_SZ, PAGESIZE - reserve_sz - FILE_HEADER_SZ);
  csz = tmp_csz;  
  out += tmp_csz;
  EVP_CipherFinal(&ectx, out, &tmp_csz);
  csz += tmp_csz;
  EVP_CIPHER_CTX_cleanup(&ectx);

  fwrite("SQLite format 3\0", 1, FILE_HEADER_SZ, outfh);
  fwrite(outbuffer, 1, PAGESIZE - FILE_HEADER_SZ, outfh);

  printf("wrote page %d\n", 0);

  for(i = 1; (read = fread(inbuffer, 1, PAGESIZE, infh)) > 0 ;i++) {
    memcpy(iv, inbuffer + PAGESIZE - reserve_sz, iv_sz); /* last iv_sz bytes are the initialization vector */
    memset(outbuffer, 0, PAGESIZE);
    out = outbuffer;
 
    EVP_CipherInit(&ectx, evp_cipher, NULL, NULL, 0);
    EVP_CIPHER_CTX_set_padding(&ectx, 0);
    EVP_CipherInit(&ectx, NULL, key, iv, 0);
    EVP_CipherUpdate(&ectx, out, &tmp_csz, inbuffer, PAGESIZE - reserve_sz);
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

