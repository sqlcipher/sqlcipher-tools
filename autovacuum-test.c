/* 
  generates a large amount of load for stress testing via repeted opens, inserts and deletes

  gcc test.c -I../sqlcipher -DSQLITE_HAS_CODEC -l crypto -o test
*/

#include <sqlite3.c>
#include <stdio.h>

#define ERROR(X)  {printf("[ERROR]: "); printf X;fflush(stdout);}

int main(int argc, char **argv) {
  sqlite3 *db;
  const char *file= "karin.db";
  int row, rc, master_rows;

    if (sqlite3_open(file, &db) == SQLITE_OK) {
      sqlite3_stmt *stmt;
      
      if(db == NULL) {
        ERROR(("sqlite3_open reported OK, but db is null, retrying open %s\n", sqlite3_errmsg(db)))
      }
 
      if((rc = sqlite3_prepare_v2(db, "PRAGMA key = 'xsiJ4XmIo10In7w/ggqF0A==';", -1, &stmt, NULL)) == SQLITE_OK) { 
        if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
	  ERROR(("error authenticating database %d, %s\n", rc, sqlite3_errmsg(db)))
          exit(1);
        }
      } else {
	ERROR(("error preparing sqlite_master query %d %s\n", rc, sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_finalize(stmt);
      printf("set key\n");

      if((rc = sqlite3_prepare_v2(db, "BEGIN EXCLUSIVE;", -1, &stmt, NULL)) == SQLITE_OK) {  
        if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
	  ERROR(("error starting transaction %d, %s\n", rc, sqlite3_errmsg(db)))
          exit(1);
        }
      } else {
	ERROR(("error parsing begin exclusive query %d %s\n", rc, sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_finalize(stmt);
      printf("started transaction\n");

      if((rc = sqlite3_prepare_v2(db, "create table Message (_id integer primary key autoincrement, syncServerId text, syncServerTimeStamp integer, displayName text, timeStamp integer, subject text, flagRead integer, flagLoaded integer, flagFavorite integer, flagAttachment integer, flags integer, clientId integer, messageId text, mailboxKey integer, accountKey integer, fromList text, toList text, ccList text, bccList text, replyToList text, meetingInfo text);", -1, &stmt, NULL)) == SQLITE_OK) {   
        if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
	  ERROR(("error creating table 1 %d, %s\n", rc, sqlite3_errmsg(db)))
          exit(1);
        }
      } else {
	ERROR(("error parsing create table 1 %d %s\n", rc, sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_finalize(stmt);
      printf("created table 1\n");
  
      if((rc = sqlite3_prepare_v2(db, "create table Message_Updates (_id integer unique, syncServerId text, syncServerTimeStamp integer, displayName text, timeStamp integer, subject text, flagRead integer, flagLoaded integer, flagFavorite integer, flagAttachment integer, flags integer, clientId integer, messageId text, mailboxKey integer, accountKey integer, fromList text, toList text, ccList text, bccList text, replyToList text, meetingInfo text);", -1, &stmt, NULL)) == SQLITE_OK) { 
        if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
	  ERROR(("error creating table 2 %d, %s\n", rc, sqlite3_errmsg(db)))
          exit(1);
        }
      } else {
	ERROR(("error parsing create table 2 %d %s\n", rc, sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_finalize(stmt);
      printf("created table 2\n");
  
      if((rc = sqlite3_prepare_v2(db, "create table Message_Deletes (_id integer unique, syncServerId text, syncServerTimeStamp integer, displayName text, timeStamp integer, subject text, flagRead integer, flagLoaded integer, flagFavorite integer, flagAttachment integer, flags integer, clientId integer, messageId text, mailboxKey integer, accountKey integer, fromList text, toList text, ccList text, bccList text, replyToList text, meetingInfo text);", -1, &stmt, NULL)) == SQLITE_OK) {
        if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
	  ERROR(("error creating table 3 %d, %s\n", rc, sqlite3_errmsg(db)))
          exit(1);
        }
      } else {
	ERROR(("error parsing create table 3 %d %s\n", rc, sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_finalize(stmt);
      printf("created table 3\n");
  
      if((rc = sqlite3_prepare_v2(db, "COMMIT;", -1, &stmt, NULL)) == SQLITE_OK) {  
        if ((rc = sqlite3_step(stmt)) != SQLITE_DONE) {
	  ERROR(("error committing transaction %d, %s\n", rc, sqlite3_errmsg(db)))
          exit(1);
        }
      } else {
	ERROR(("error parsing commit query %d %s\n", rc, sqlite3_errmsg(db)))
        exit(1);
      }
      sqlite3_finalize(stmt);
      printf("commit transaction\n");

      sqlite3_close(db);
  
    } else {
      ERROR(("error opening database %s\n", sqlite3_errmsg(db)))
      exit(1);
    }	
}
