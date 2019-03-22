/* 
  gcc loadtest.c -I../sqlcipher-internal -DSQLITE_HAS_CODEC -DSQLCIPHER_CRYPTO_CC -o loadtest -framework Security -framework Foundation
*/

#include <sqlite3.c>
#include <stdio.h>
#include <sys/time.h>

#define ERROR(X)  {printf("[ERROR] iteration %d: ", i); printf X;fflush(stdout);}

#define LOOPS 1500
int main(int argc, char **argv) {
  sqlite3 *db;
  const char *file= "loadtest.db";
  struct timeval stop, start;
  int insert_rows = 10000;
  int row, rc;
  sqlite3_stmt *stmt;
  int i;

  const char *key = "test"; 
  char *buffer = malloc(LOOPS);
  sqlite3_int64 current, highwater;

  for(i = 0; i < LOOPS; i++) {
    *(buffer+i) = 'a';
  }

  if (sqlite3_open(file, &db) != SQLITE_OK) {
    ERROR(("error opening database %s\n", sqlite3_errmsg(db)))
    exit(1);
  }
     
  if(sqlite3_key(db, key, strlen(key)) != SQLITE_OK) {
    ERROR(("error setting key %s\n", sqlite3_errmsg(db)))
    exit(1);
  }

  if(sqlite3_exec(db, "CREATE TABLE IF NOT EXISTS t1(data);", NULL, NULL, NULL) != SQLITE_OK) {
    ERROR(("error creating table %s\n", sqlite3_errmsg(db)))
    exit(1);
  }

/*
  if(sqlite3_exec(db, "PRAGMA cipher_memory_security = OFF;", NULL, NULL, NULL) != SQLITE_OK) {
    exit(1);
  }
*/

  for(i = 0; i < LOOPS; i++) {

/*
    if(sqlite3_exec(db, "DELETE FROM t1;", NULL, NULL, NULL) != SQLITE_OK) {
      exit(1);
    }

    if(sqlite3_exec(db, "VACUUM;", NULL, NULL, NULL) != SQLITE_OK) {
      exit(1);
    }
*/

    gettimeofday(&start, NULL);

    if(sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL) != SQLITE_OK) {
      ERROR(("error starting transaction %s\n", sqlite3_errmsg(db)))
      exit(1);
    }
     
    if(sqlite3_prepare_v2(db, "INSERT INTO t1(data) VALUES (?);", -1, &stmt, NULL) != SQLITE_OK) {
      ERROR(("error preparing insert %s\n", sqlite3_errmsg(db)))
      exit(1);
    }

    for(row = 0; row < insert_rows; row++) {
      //sqlite3_bind_text(stmt, 1, buffer, i, SQLITE_TRANSIENT);
      sqlite3_bind_text(stmt, 1, buffer, i, SQLITE_STATIC);
      if (sqlite3_step(stmt) != SQLITE_DONE) {
        ERROR(("error inserting row %s\n", sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);

    sqlite3_status64(SQLITE_STATUS_MEMORY_USED, &current, &highwater, 0);
    printf("SQLITE_STATUS_MEMORY_USED current=%lld, highwater=%lld\n", current, highwater);
    sqlite3_status64(SQLITE_STATUS_MALLOC_SIZE, &current, &highwater, 0);
    printf("SQLITE_STATUS_MALLOC_SIZE current=%lld, highwater=%lld\n", current, highwater);
    sqlite3_status64(SQLITE_STATUS_MALLOC_COUNT, &current, &highwater, 0);
    printf("SQLITE_STATUS_MALLOC_COUNT current=%lld, highwater=%lld\n", current, highwater);
    sqlite3_status64(SQLITE_STATUS_PAGECACHE_USED, &current, &highwater, 0);
    printf("SQLITE_STATUS_PAGECACHE_USED current=%lld, highwater=%lld\n", current, highwater);
    sqlite3_status64(SQLITE_STATUS_PAGECACHE_OVERFLOW, &current, &highwater, 0);
    printf("SQLITE_STATUS_PAGECACHE_OVERFLOW current=%lld, highwater=%lld\n", current, highwater);
    sqlite3_status64(SQLITE_STATUS_PAGECACHE_SIZE, &current, &highwater, 0);
    printf("SQLITE_STATUS_PAGECACHE_SIZE current=%lld, highwater=%lld\n", current, highwater);

    if(sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL) != SQLITE_OK) {
      ERROR(("error committing transaction %s\n", sqlite3_errmsg(db)))
      exit(1);
    }
    gettimeofday(&stop, NULL);
    
    //printf("Inserted %d records with each containing %d chars in %lu ms\n", insert_rows, i, ((stop.tv_sec - start.tv_sec) * 1000) + ((stop.tv_usec - start.tv_usec) / 1000) );
    printf("%d\t%lu\n", i, ((stop.tv_sec - start.tv_sec) * 1000) + ((stop.tv_usec - start.tv_usec) / 1000) );
  }

  sqlite3_close(db);
  free((void *)buffer);
}
