/* 
  test utility for sqlite based backup encrytion to demostrate brokenness

  gcc backuptest.c -I../sqlcipher -DSQLITE_HAS_CODEC -l crypto -o backuptest
*/

#include <sqlite3.c>
#include <stdio.h>

#define ERROR(X)  {printf("[ERROR] "); printf X;fflush(stdout); exit(1);}

const char *usage =
        "Usage: backuptest [options]\n"
        "\n"
        "Options :\n"
        "\n"
        "\t-i filename input filename or :memory: (default sqlcipher.db).\n"
        "\t-I input key (default no key).\n"
        "\t-o filename output filename or :memory: (default backup.db).\n"
        "\t-O output key (default no key).\n"
        "\t-h help (this text)\n"
        "\n"
        "\n"
        ;
const char *optstring = "hi:o:I:O:";

int parse_args(int argc, char **argv, char **input, char **output, char **inkey, char **outkey)
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
                case 'I':
                        *inkey = optarg;
                        break;
                case 'O':
                        *outkey = optarg;
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
  sqlite3 *indb, *outdb;
  sqlite3_stmt *stmt;
  char* infile = "sqlcipher.db";
  char* outfile= "backup.db";
  char* inkey = NULL;
  char* outkey = NULL;
  int count;
  int rc;

  if (!parse_args(argc, argv, &infile, &outfile, &inkey, &outkey)) return 1;

  if (sqlite3_open(infile, &indb) != SQLITE_OK) {
     ERROR(("sqlite3_open failed for %s: %s\n", infile, sqlite3_errmsg(indb)))
  }

  if(inkey != NULL) sqlite3_key(indb, inkey, strlen(inkey)); 

  if(strcmp(infile, ":memory:") == 0) {
    sqlite3_exec(indb, "CREATE TABLE t1(a,b);", NULL, 0, NULL);
    sqlite3_exec(indb, "BEGIN;", NULL, 0, NULL);
    for(int i = 0; i < 524288; i++) {
      sqlite3_exec(indb, "INSERT INTO t1(a,b) VALUES ('0123456789abcdefghijklmnopqrstuvwxyz', ')!@#$%^&*(ABCDEFGHIJKLMNOPQRSTUVWXYZ');", NULL, 0, NULL);
    } 
    sqlite3_exec(indb, "COMMIT;", NULL, 0, NULL);
  }
 
  if (sqlite3_open(outfile, &outdb) != SQLITE_OK) {
     ERROR(("sqlite3_open failed for %s: %s\n", outfile, sqlite3_errmsg(outdb)))
  }

  if(outkey != NULL) {
    sqlite3_key(outdb, outkey, strlen(outkey)); 
  }
  sqlite3_exec(outdb, "SELECT count(*) FROM sqlite_master;", NULL, 0, NULL);

  sqlite3_backup *backup = sqlite3_backup_init(outdb, "main", indb, "main"); 
  if(backup == NULL) {
     ERROR(("sqlite3_backup failed %s\n", sqlite3_errmsg(outdb)))
  }
  if((rc = sqlite3_backup_step(backup, -1)) != SQLITE_DONE) { 
     ERROR(("sqlite3_backup_step from indb to outdb failed %d, %s, %s\n", rc, sqlite3_errmsg(indb), sqlite3_errmsg(outdb)))
  }
  sqlite3_backup_finish(backup);

  if((rc = sqlite3_prepare_v2(outdb, "PRAGMA integrity_check;", -1, &stmt, NULL)) == SQLITE_OK) {
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      printf("%s\n", sqlite3_column_text(stmt, 0));
    } else {
      ERROR(("error stepping integrity statement %d: %s\n", rc, sqlite3_errmsg(outdb)))
    }
  } else {
    ERROR(("error preparing integrity statement %d: %s\n", rc, sqlite3_errmsg(outdb)))
  }
  sqlite3_finalize(stmt);

  if((rc = sqlite3_prepare_v2(outdb, "PRAGMA page_size;", -1, &stmt, NULL)) == SQLITE_OK) {
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      printf("%d\n", sqlite3_column_int(stmt, 0));
    } else {
      ERROR(("error stepping page statement %d: %s\n", rc, sqlite3_errmsg(outdb)))
    }
  } else {
    ERROR(("error preparing page statement %d: %s\n", rc, sqlite3_errmsg(outdb)))
  }
  sqlite3_finalize(stmt);

  if((rc = sqlite3_prepare_v2(outdb, "SELECT count(*) FROM t1;", -1, &stmt, NULL)) == SQLITE_OK) {
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      printf("%d\n", sqlite3_column_int(stmt, 0));
    } else {
      ERROR(("error stepping count statement %d: %s\n", rc, sqlite3_errmsg(outdb)))
    }
  } else {
    ERROR(("error preparing count statement %d: %s\n", rc, sqlite3_errmsg(outdb)))
  }
  sqlite3_finalize(stmt);

  if((rc = sqlite3_prepare_v2(outdb, "select max(length(a)), min(length(a)), min(length(b)), min(length(b)) from t1;", -1, &stmt, NULL)) == SQLITE_OK) {
    if ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
      printf("%d %d %d %d\n", sqlite3_column_int(stmt, 0), sqlite3_column_int(stmt, 1), sqlite3_column_int(stmt, 2), sqlite3_column_int(stmt, 3));
    } else {
      ERROR(("error stepping max statement %d: %s\n", rc, sqlite3_errmsg(outdb)))
    }
  } else {
    ERROR(("error preparing max statement %d: %s\n", rc, sqlite3_errmsg(outdb)))
  }
  sqlite3_finalize(stmt);


  sqlite3_close(indb);
  sqlite3_close(outdb);
}
