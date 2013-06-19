/*
 * rm *.db
 * cc attach.c -I../sqlcipher-static/sqlcipher -DSQLITE_HAS_CODEC -DSQLCIPHER_CRYPTO_CC -framework Security -DSQLITE_ENABLE_FTS3=1 -DSQLITE_TEMP_STORE=2 -o attach
 * OR
 * cc attach.c -I../sqlcipher-static/sqlcipher -DSQLITE_HAS_CODEC -DSQLITE_ENABLE_FTS3=1 -DSQLITE_TEMP_STORE=2 -l crypto -o attach
 * // -I../sqlcipher/src 
 * ./attach
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
  const char* attach_sql = "ATTACH DATABASE ? AS rekey KEY ?;";
  int rc;
  sqlite3 *local_db, *remote_db;  
  char *errorPointer;
  sqlite3_stmt *stmt;
  
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
    ERROR(("select from sqlite_master failed: %s\n", errorPointer));
    sqlite3_free(errorPointer);
    exit(1);
  }
  // prepare stmt for attach
  rc = sqlite3_prepare_v2(remote_db, attach_sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    ERROR(("sqlite3_prepare_v2 failed for %s: %s\n", attach_sql, sqlite3_errmsg(remote_db)))
    exit(1);
  }
  // "ATTACH DATABASE ? AS rekey KEY ?;";
  sqlite3_bind_text(stmt, 1, rekeyPath, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, key, -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    ERROR(("rekey attach failed: %s\n", sqlite3_errmsg(remote_db)));
    sqlite3_free(errorPointer);
    sqlite3_finalize(stmt);
    exit(1);
  }
  sqlite3_finalize(stmt);
  sqlite3_exec(remote_db, "PRAGMA rekey.kdf_iter = 64000;", NULL, 0, NULL);
  rc = sqlite3_exec(remote_db, "SELECT sqlcipher_export('rekey');", NULL, 0, NULL);
  if (rc != SQLITE_OK) {
    ERROR(("sqlcipher_export failed: %s\n", sqlite3_errmsg(remote_db)))
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
    ERROR(("select from sqlite_master failed: %s\n", errorPointer));
    sqlite3_free(errorPointer);
    exit(1);
  }
  // prepare stmt for attach on local_db
  // This is the heart of our test: does attaching this rekey'd db succeed or fail?
  rc = sqlite3_prepare_v2(local_db, attach_sql, -1, &stmt, NULL);
  if (rc != SQLITE_OK) {
    ERROR(("sqlite3_prepare_v2 failed for %s: %s\n", attach_sql, sqlite3_errmsg(remote_db)))
    exit(1);
  }
  // "ATTACH DATABASE ? AS rekey KEY ?;";
  sqlite3_bind_text(stmt, 1, rekeyPath, -1, SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt, 2, key, -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    ERROR(("rekey attach to local failed: %s\n", sqlite3_errmsg(local_db)));
    sqlite3_free(errorPointer);
    sqlite3_finalize(stmt);
    exit(1);
  }
  sqlite3_finalize(stmt);
  sqlite3_close(local_db);
    
  return 0;
}