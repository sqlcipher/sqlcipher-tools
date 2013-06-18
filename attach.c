/*
 * cc attach.c -I../sqlcipher-static/sqlcipher -DSQLITE_HAS_CODEC -DSQLCIPHER_CRYPTO_CC -framework Security -DSQLITE_ENABLE_FTS3=1 -DSQLITE_TEMP_STORE=2 -o attach
 *
 */

#include <sqlite3.c>
#include <stdio.h>

#define ERROR(X)  {printf("[ERROR]: "); printf X;fflush(stdout);}
 
int main( int argc, const char* argv[])
{  
  const char* key = "123";
  const char* create = "CREATE TABLE t1( id TEXT PRIMARY KEY, a TEXT, b TEXT);";
  const char* insert = "INSERT INTO t1 (id,a,b) VALUES (1,'a','b');";
  const char* localPath = "./attach-local.db";
  const char* remotePath = "./attach-remote.db";
  const char* rekeyPath = "./attach-rekey.db";
  int rc;

  sqlite3 *local_db, *remote_db;  
  char *errorPointer;
  // create the remote db with legacy settings
  if (sqlite3_open(remotePath, &remote_db) != SQLITE_OK) {
    ERROR(("sqlite3_open failed for %s: %s\n", remotePath, sqlite3_errmsg(remote_db)))
    exit(1);
  }
  sqlite3_key(remote_db, key, strlen(key));
  sqlite3_exec(remote_db, "PRAGMA kdf_iter = 4000;", NULL, 0, NULL);
  sqlite3_exec(remote_db, create, NULL, 0, NULL);
  sqlite3_exec(remote_db, insert, NULL, 0, NULL);
  // close it to write it out to storage...
  sqlite3_close(remote_db);
  
  // create the local db with current settings
  if (sqlite3_open(localPath, &local_db) != SQLITE_OK) {
    ERROR(("sqlite3_open failed for %s: %s\n", localPath, sqlite3_errmsg(local_db)))
    exit(1);
  }
  sqlite3_key(local_db, key, strlen(key));
  sqlite3_exec(local_db, "PRAGMA kdf_iter = 64000;", NULL, 0, NULL);
  sqlite3_exec(local_db, create, NULL, 0, NULL);
  sqlite3_exec(local_db, insert, NULL, 0, NULL);
  // close it to write it out to storage...
  sqlite3_close(local_db);
  
  // upgrade the remote db via rekey
  if (sqlite3_open(remotePath, &remote_db) != SQLITE_OK) {
    ERROR(("sqlite3_open failed for %s: %s\n", remotePath, sqlite3_errmsg(remote_db)))
    exit(1);
  }
  sqlite3_key(remote_db, key, strlen(key));
  sqlite3_exec(remote_db, "PRAGMA kdf_iter = 4000;", NULL, 0, NULL);
  if (sqlite3_exec(remote_db, "SELECT * FROM sqlite_master;", NULL, 0, &errorPointer) != SQLITE_OK) {
    ERROR(("select from sqlite_master failed: %s", errorPointer));
    sqlite3_free(errorPointer);
    exit(1);
  }
  // FIXME: use variables ...
  if (sqlite3_exec(remote_db, "ATTACH DATABASE './attach-rekey.db' AS rekey KEY '123';", NULL, 0, &errorPointer) != SQLITE_OK) {
    ERROR(("rekey attach failed: %s", errorPointer));
    sqlite3_free(errorPointer);
    exit(1);
  }
  sqlite3_exec(remote_db, "PRAGMA rekey.kdf_iter = 64000;", NULL, 0, NULL);
  rc = sqlite3_exec(remote_db, "SELECT sqlcipher_export('rekey');", NULL, 0, NULL);
  if (rc != SQLITE_OK) {
    ERROR(("sqlcipher_export failed: %s", sqlite3_errmsg(remote_db)))
    exit(1);
  }
  sqlite3_close(remote_db);
  // re-open local_db and attach rekeyed database
  if (sqlite3_open(localPath, &local_db) != SQLITE_OK) {
    ERROR(("sqlite3_open failed for %s: %s\n", localPath, sqlite3_errmsg(local_db)))
    exit(1);
  }
  sqlite3_key(local_db, key, strlen(key));
  sqlite3_exec(local_db, "PRAGMA kdf_iter = 64000;", NULL, 0, NULL);
  if (sqlite3_exec(local_db, "SELECT * FROM sqlite_master;", NULL, 0, &errorPointer) != SQLITE_OK) {
    ERROR(("select from sqlite_master failed: %s", errorPointer));
    sqlite3_free(errorPointer);
    exit(1);
  }
  // This is the heart of our test: does attaching this rekey'd db succeed or fail?
  if (sqlite3_exec(local_db, "ATTACH DATABASE './attach-rekey.db' AS rekey KEY '123';", NULL, 0, &errorPointer) != SQLITE_OK) {
    ERROR(("attach rekey to local failed: %s", errorPointer));
    sqlite3_free(errorPointer);
    exit(1);
  }
  sqlite3_close(local_db);
    
  return 0;
}