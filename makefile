CC = gcc

LIBRARIES = -lm

NAME = tests
NAME_NODEBUG = tests_nodebug

TEMP_DIR = tempdirectory

all: debug nodebug auto
debug:
	$(CC) *.c -lm -o $(NAME)
nodebug:
	mkdir $(TEMP_DIR)
	cp *.c *.h $(TEMP_DIR)
	rm $(TEMP_DIR)/curdebug.h
	mv $(TEMP_DIR)/nodebug.h $(TEMP_DIR)/curdebug.h
	$(CC) $(TEMP_DIR)/*.c $(LIBRARIES) -o $(NAME_NODEBUG)
	rm -rf $(TEMP_DIR)
auto: nodebug
	./$(NAME_NODEBUG)

clean:
	rm -f $(NAME_NODEBUG) $(NAME)
	rm -rf $(TEMP_DIR)