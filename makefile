CC = gcc

LIBRARIES = -lm

NAME = tests
NAME_NODEBUG = tests_nodebug
NAME_FUSE = fuse

TEMP_DIR = tempdirectory

all: clean debug nodebug auto
debug:
	mkdir $(TEMP_DIR)
	cp *.c *.h $(TEMP_DIR)
	rm $(TEMP_DIR)/fusemain.c
	$(CC) $(TEMP_DIR)/*.c $(LIBRARIES) -o $(NAME_NODEBUG)
	rm -rf $(TEMP_DIR)
nodebug:
	mkdir $(TEMP_DIR)
	cp *.c *.h $(TEMP_DIR)
	rm $(TEMP_DIR)/fusemain.c
	rm $(TEMP_DIR)/curdebug.h
	mv $(TEMP_DIR)/nodebug.h $(TEMP_DIR)/curdebug.h
	$(CC) $(TEMP_DIR)/*.c $(LIBRARIES) -o $(NAME_NODEBUG)
	rm -rf $(TEMP_DIR)
auto: nodebug
	./$(NAME_NODEBUG)
fuse:
	mkdir $(TEMP_DIR)
	cp *.c *.h $(TEMP_DIR)
	rm $(TEMP_DIR)/tests.c
	rm $(TEMP_DIR)/curdebug.h
	mv $(TEMP_DIR)/nodebug.h $(TEMP_DIR)/curdebug.h
	$(CC) $(TEMP_DIR)/*.c $(LIBRARIES) -o $(NAME_FUSE)
	rm -rf $(TEMP_DIR)

clean:
	rm -f $(NAME_NODEBUG) $(NAME) $(NAME_FUSE)
	rm -rf $(TEMP_DIR)