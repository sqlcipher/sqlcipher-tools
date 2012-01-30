/* 
  generates a large amount of load for stress testing via repeted opens, inserts and deletes

  gcc load.c -I../sqlcipher -DSQLITE_HAS_CODEC -l crypto -o load
*/

#include <sqlite3.c>
#include <stdio.h>

#define ERROR(X)  {printf("[ERROR] iteration %d: ", i); printf X;fflush(stdout);}

int main(int argc, char **argv) {
  sqlite3 *db;
  const char* key = "test123";
  const char* file = "sqlcipher.db";
  int i;
  int loops = 100000;
  int insert_rows = 100;
  int print_every = 100;

  unsigned long inserts, deletes, updates;
  inserts = deletes = updates = 0;

  srand(0);

  for(i = 0; i < loops; i++) {
    if( i % print_every == 0 ) printf("iteration %d - %d...\n", i, i + print_every - 1); 

    if (sqlite3_open(file, &db) == SQLITE_OK) {
      int row, rc, master_rows;
      sqlite3_stmt *stmt;
      
      if(db == NULL) {
        ERROR(("sqlite3_open reported OK, but db is null, retrying open %s\n", sqlite3_errmsg(db)))
        continue;
      }
 
      if(sqlite3_key(db, key, strlen(key)) != SQLITE_OK) {
	ERROR(("error setting key %s\n", sqlite3_errmsg(db)))
        exit(1);
      }

      if(sqlite3_rekey(db, key, strlen(key)) != SQLITE_OK) {
	ERROR(("error setting rekey %s\n", sqlite3_errmsg(db)))
        exit(1);
      }
    
    
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
  
      if(master_rows == 0) { /* no schema yet */
        if(sqlite3_exec(db, "CREATE TABLE t1(a,b);", NULL, NULL, NULL) != SQLITE_OK) {
	  ERROR(("error preparing sqlite_master query %s\n", sqlite3_errmsg(db)))
        }
        if(sqlite3_exec(db, "CREATE INDEX t1_a_idx ON t1(a);", NULL, NULL, NULL) != SQLITE_OK) {
          ERROR(("error creating index %s\n", sqlite3_errmsg(db)))
        }
      }
  
      if(sqlite3_exec(db, "BEGIN;", NULL, NULL, NULL) != SQLITE_OK) {
        ERROR(("error starting transaction %s\n", sqlite3_errmsg(db)))
      }
     
      if(sqlite3_prepare_v2(db, "INSERT INTO t1(a,b) VALUES (?, ?);", -1, &stmt, NULL) == SQLITE_OK) {
        for(row = 0; row < insert_rows; row++) {
          sqlite3_bind_int(stmt, 1, rand());
          sqlite3_bind_int(stmt, 2, rand());
          if (sqlite3_step(stmt) != SQLITE_DONE) {
            ERROR(("error inserting row %s\n", sqlite3_errmsg(db)))
            exit(1);
          }
	  inserts++;
          sqlite3_reset(stmt);
        }
      } else {
        ERROR(("error preparing insert %s\n", sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_finalize(stmt);

      if(sqlite3_prepare_v2(db, "UPDATE t1 SET b = ? WHERE a < ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, rand());
        sqlite3_bind_int(stmt, 2, rand());
        if (sqlite3_step(stmt) != SQLITE_DONE) {
          ERROR(("error updating rows %s\n", sqlite3_errmsg(db)))
        }
	updates += sqlite3_changes(db);
      } else {
        ERROR(("error preparing delete statement rows %s\n", sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_finalize(stmt);

      if(sqlite3_prepare_v2(db, "DELETE FROM t1 WHERE a < ?;", -1, &stmt, NULL) == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, rand());
        if (sqlite3_step(stmt) != SQLITE_DONE) {
          ERROR(("error deleting rows %s\n", sqlite3_errmsg(db)))
        }
	deletes += sqlite3_changes(db);
      } else {
        ERROR(("error preparing delete statement rows %s\n", sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_finalize(stmt);

      if(sqlite3_exec(db, "COMMIT;", NULL, NULL, NULL) != SQLITE_OK) {
        ERROR(("error committing transaction %s\n", sqlite3_errmsg(db)))
      }
     
      sqlite3_close(db);
  
    } else {
      ERROR(("error opening database %s\n", sqlite3_errmsg(db)))
      exit(1);
    }	
  }

  printf("completed test run with %d open/begin/commit/closes, %lu row inserts, %lu row updates, and %lu row deletes\n", i, inserts, updates, deletes); 
}
