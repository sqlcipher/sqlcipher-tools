/* 
  compare the difference on whether open is expensive

  gcc open-load.c -I../sqlcipher -DSQLITE_HAS_CODEC -l crypto -o open-load
*/

#include <sqlite3.c>
#include <stdio.h>

#define ERROR(X)  {printf("[ERROR] iteration %d: ", i); printf X;fflush(stdout);}

int main(int argc, char **argv) {
  sqlite3 *db;
  const char* key = "x'2DD29CA851E7B56E4697B0E1F08507293D761A05CE4D1B628663F411A8086D99'";
  const char* file = "sqlcipher.db";
  int i, rc;
  int loops = 100000;
  int insert_rows = 10;
  int print_every = 10000;
  int openonce = 0;
  srand(0);

  if(openonce) {
    printf("opening database %s\n", file);
    if ((rc = sqlite3_open(file, &db)) != SQLITE_OK) {
      ERROR(("open failed %d %s\n", rc, sqlite3_errmsg(db)))
      exit(1);
    }
    if(sqlite3_key(db, key, strlen(key)) != SQLITE_OK) {
      ERROR(("error setting key %s\n", sqlite3_errmsg(db)))
      exit(1);
    }
  }

  for(i = 0; i < loops; i++) {
    int row, master_rows;
    sqlite3_stmt *stmt;

    if(!openonce) {
      if ((rc = sqlite3_open(file, &db)) != SQLITE_OK) {
        ERROR(("open failed %d %s\n", rc, sqlite3_errmsg(db)))
        exit(1);
      }
      if(sqlite3_key(db, key, strlen(key)) != SQLITE_OK) {
        ERROR(("error setting key %s\n", sqlite3_errmsg(db)))
        exit(1);
      }
    }

    if( i % print_every == 0 ) printf("iteration %d - %d openonce=%d...\n", i, i + print_every - 1, openonce); 
    /* read schema. If no rows, create table and stuff
       if error - close it up!*/
    if(sqlite3_prepare_v2(db, "SELECT count(*) FROM sqlite_master;", -1, &stmt, NULL) == SQLITE_OK) {
      if (sqlite3_step(stmt) == SQLITE_ROW) {
        master_rows = sqlite3_column_int(stmt, 0);
      } else {
        ERROR(("error authenticating database %s\n", sqlite3_errmsg(db)))
        exit(1);
      }
    } else {
      ERROR(("error preparing sqlite_master query %s\n", sqlite3_errmsg(db)))
      exit(1);
    }
    sqlite3_finalize(stmt);
  
    if(!openonce) {
      sqlite3_close(db);
    }
  }

  if(openonce) {
    printf("closing database %s\n", file);
    sqlite3_close(db);
  }
}
