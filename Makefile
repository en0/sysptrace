include config.mk

BIN=sysptrace
OBJ=$(patsubst %.c, %.c.o, $(wildcard *.c)) \
	$(patsubst %.a, %.a.o, $(wildcard *.a))

$(BIN) : $(OBJ)
	$(CC) -o $@ $^

%.c.o : %.c
	$(CC) -c -o $@ $<

%.a.o : %.a
	$(AS) -o $@ $<

.PHONY : clean clobber

clean : 
	$(RM) $(OBJ)

clobber :
	$(RM) $(OBJ) $(BIN)

.PHONY : install uninstall

install : $(BIN)
	install -t $(PREFIX) $(BIN)

uninstall :
	$(RM) $(PREFIX)/$(BIN)
