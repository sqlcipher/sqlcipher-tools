/* 
  gcc recrypt.c -I../sqlcipher -DSQLITE_HAS_CODEC -l crypto -o recrypt
*/

#include <sqlite3.c>
#include <stdio.h>

#define ERROR(X)  {printf("[ERROR] "); printf X;fflush(stdout); exit(1);}

int main(int argc, char **argv) {
  sqlite3 *indb, *edb, *outdb;

  const char* infile = "plain.db";
  const char* efile= "sqlcipher.db";
  const char* outfile= "plain2.db";

  const char* key = "test123";

  int rc;

  if (sqlite3_open(infile, &indb) != SQLITE_OK) {
     ERROR(("sqlite3_open failed for %s: %s\n", infile, sqlite3_errmsg(indb)))
  }

  if (sqlite3_open(efile, &edb) != SQLITE_OK) {
     ERROR(("sqlite3_open failed for %s: %s\n", efile, sqlite3_errmsg(edb)))
  }
  sqlite3_key(edb, key, strlen(key));
  sqlite3_exec(edb, "SELECT count(*) FROM sqlite_master;", NULL, 0, NULL);

  if (sqlite3_open(outfile, &outdb) != SQLITE_OK) {
     ERROR(("sqlite3_open failed for %s: %s\n", outfile, sqlite3_errmsg(outdb)))
  }

  /*
  sqlite3_backup *backup = sqlite3_backup_init(outdb, "main", indb, "main"); 
  if(backup == NULL) {
     ERROR(("sqlite3_backup failed %s\n", sqlite3_errmsg(outdb)))
  }
  if((rc = sqlite3_backup_step(backup, -1)) != SQLITE_DONE) { 
     ERROR(("sqlite3_backup_step from indb to outdb failed %d, %s, %s\n", rc, sqlite3_errmsg(indb), sqlite3_errmsg(outdb)))
  }
  sqlite3_backup_finish(backup);
  */

  /*
  */
  sqlite3_backup *backup = sqlite3_backup_init(edb, "main", indb, "main"); 
  if(backup == NULL) {
     ERROR(("sqlite3_backup failed %s\n", sqlite3_errmsg(indb)))
  }
  if((rc = sqlite3_backup_step(backup, -1)) != SQLITE_DONE) { 
     ERROR(("sqlite3_backup_step from indb to edb failed %d, %s, %s\n", rc, sqlite3_errmsg(indb), sqlite3_errmsg(edb)))
  }
  sqlite3_backup_finish(backup);


  backup = sqlite3_backup_init(outdb, "main", edb, "main"); 
  if(backup == NULL) {
     ERROR(("sqlite3_backup failed %s\n", sqlite3_errmsg(edb)))
  }
  if((rc = sqlite3_backup_step(backup, -1)) != SQLITE_DONE) { 
     ERROR(("sqlite3_backup_step failed from edb to outdb %d, %s, %s\n", rc, sqlite3_errmsg(edb), sqlite3_errmsg(outdb)))
  }
  sqlite3_backup_finish(backup);

  sqlite3_close(indb);
  sqlite3_close(edb);
  sqlite3_close(outdb);
}
