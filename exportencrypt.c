/* 
  encrypt a plaintext database to an encrypted database using the sqlcipher_export function in sqlcipher 2

  Build:
    gcc exportencrypt.c -I../sqlcipher -DSQLITE_HAS_CODEC -DSQLITE_ENABLE_FTS3=1 -DSQLITE_TEMP_STORE=2 -l crypto -o exportencrypt
  Usage:
    ./exportencrypt -i <input file> -o <output file> -k <key> [-c cache_size] [-p page_size]
  
    ./exportencrypt -i ~/Downloads/input.sqlite -o enc.db -k test123 -c 2048 -p 4096
*/

#include <sqlite3.c>
#include <stdio.h>
#include <unistd.h>

#define ERROR(X)  {printf("[ERROR] "); printf X;fflush(stdout);}
#define INFO(X)  {printf("[INFO] "); printf X;fflush(stdout);}

#define BUF_SZ 1024

int main(int argc, char **argv) {
  sqlite3 *db;
  int rc;
  int cache_sz = -1;
  int page_sz = -1;
  char flag;
  char *inf, *outf, *key;
  char *sql;
  
  while ((flag = getopt(argc, argv, "i:o:k:c:p:")) != -1) {
    switch(flag) {
      case 'i':
        if((inf = (char *)calloc((size_t) strlen(optarg) + 1, sizeof(char))) == NULL) {
           ERROR(("no memory"));
           exit(1);
        }
        strcpy(inf, optarg);
        break;
      case 'o':
        if((outf = (char *)calloc((size_t) strlen(optarg) + 1, sizeof(char))) == NULL) {
           ERROR(("no memory"));
           exit(1);
        }
        strcpy(outf, optarg);
        break;
      case 'k':
        if((key = (char *) calloc((size_t) strlen(optarg) + 1, sizeof(char))) == NULL) {
           ERROR(("no memory"));
           exit(1);
        }
        strcpy(key, optarg);
        break;
      case 'c':
        cache_sz = atoi(optarg);
        break;
      case 'p':
        page_sz = atoi(optarg);
        break;
      case '?':
      default:
        break; 
    }
  }
  argc -= optind;
  argv += optind;

  if(inf == NULL || outf == NULL || key == NULL) {
    ERROR(("usage: exportencrypt -i <input file> -o <output file> -k <key> [-c cache_size] [-p page_size]\n"));
    exit(1);
  }

  if ((rc = sqlite3_open(inf, &db)) != SQLITE_OK) {
     ERROR(("sqlite3_open failed for %s: %d, %s\n", inf, rc, sqlite3_errmsg(db)))
  }

  if(cache_sz != -1) {
    sql = sqlite3_mprintf("PRAGMA cache_size = %d;", cache_sz); 
    rc = (sql == NULL) ? SQLITE_NOMEM : sqlite3_exec(db, sql, NULL, 0, NULL); 
    INFO(("%s\n", sql));
    if( rc!=SQLITE_OK ) goto end_of_export;
    sqlite3_free(sql);
  }

  sql = sqlite3_mprintf("ATTACH DATABASE %Q AS enc KEY %Q;", outf, key); 
  rc = (sql == NULL) ? SQLITE_NOMEM : sqlite3_exec(db, sql, NULL, 0, NULL); 
  INFO(("%s\n", sql));
  if( rc!=SQLITE_OK ) goto end_of_export;
  sqlite3_free(sql);

  if(page_sz != -1) {
    sql = sqlite3_mprintf("PRAGMA enc.cipher_page_size = %d;", page_sz); 
    rc = (sql == NULL) ? SQLITE_NOMEM : sqlite3_exec(db, sql, NULL, 0, NULL); 
    INFO(("%s\n", sql));
    if( rc!=SQLITE_OK ) goto end_of_export;
    sqlite3_free(sql);
  }
  
  sql = "SELECT sqlcipher_export('enc');";
  rc = sqlite3_exec(db, sql, NULL, 0, NULL); 
  INFO(("%s\n", sql));
  if( rc!=SQLITE_OK ) goto end_of_export;

  sql = "DETACH DATABASE enc;";
  rc = sqlite3_exec(db, sql, NULL, 0, NULL); 
  INFO(("%s\n", sql));
  if( rc!=SQLITE_OK ) goto end_of_export;

  sql = NULL;

end_of_export:
  sqlite3_free(sql);

  if(rc != SQLITE_OK) {
    ERROR(("error %d: %s\n", rc, sqlite3_errmsg(db)))
  }
  sqlite3_close(db);
  exit(0);
}
