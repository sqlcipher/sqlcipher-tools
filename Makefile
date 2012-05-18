SQLITEDIR = ../sqlcipher

INCS = -I$(SQLITEDIR) 
CFLAGS = -g -DSQLITE_TEMP_STORE=1  -DSQLITE_HAS_CODEC -DSQLITE_OS_UNIX=1 -DSQLITE_THREADSAFE=1
LDFLAGS = -lcrypto
CC = gcc 

SRC = load.c 
OBJS = ${SRC:.c=.o}
TARGET = load

all: $(TARGET)
	
$(OBJS): $(SRC)
	$(CC) $(INCS) $(DEFS) $(LIBS) $(LDFLAGS) $(CFLAGS) -o $@ $(SRC)

$(TARGET): $(OBJS) 
	#$(CC) $(INCS) $(DEFS) $(LIBS) $(CFLAGS) $(LDFLAGS) $(OBJS) -o load


clean:
	rm -rf $(OBJS) *.dSYM *.db load
