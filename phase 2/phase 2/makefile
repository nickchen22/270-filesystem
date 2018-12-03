CC = gcc

LIBRARIES = -lm

NAME_FUSE = fuse
NAME_MKFS = mkfs

TEMP_DIR = tempdirectory

all: fuse mkfs
fuse: clean
	mkdir $(TEMP_DIR)
	cp *.c *.h $(TEMP_DIR)
	rm $(TEMP_DIR)/tests.c
	rm $(TEMP_DIR)/mkfs.c
	rm $(TEMP_DIR)/nodebug.h
	$(CC) `pkg-config fuse --cflags --libs` $(TEMP_DIR)/*.c $(LIBRARIES) -o $(NAME_FUSE)
	rm -rf $(TEMP_DIR)
	
mkfs: clean
	mkdir $(TEMP_DIR)
	cp *.c *.h $(TEMP_DIR)
	rm $(TEMP_DIR)/tests.c
	rm $(TEMP_DIR)/fusemain.c
	rm $(TEMP_DIR)/nodebug.h
	$(CC) `pkg-config fuse --cflags --libs` $(TEMP_DIR)/*.c $(LIBRARIES) -o $(NAME_MKFS)
	rm -rf $(TEMP_DIR)

clean:
	rm -f $(NAME_MKFS) $(NAME_FUSE)
	rm -rf $(TEMP_DIR)