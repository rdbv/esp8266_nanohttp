CC = xtensa-lx106-elf-gcc
AR = xtensa-lx106-elf-ar
LD = xtensa-lx106-elf-ld

CFLAGS = -I. -Os -mlongcalls -DICACHE_FLASH --std=c99 -Wno-implicit-function-declaration

LDFLAGS = -Teagle.app.v6.ld 
LDLIBS = -nostdlib -Wl,--start-group -lmain -lnet80211 -lwpa -llwip -lpp -lphy -lc -Wl,--end-group -lgcc 

user_main:
	clear
	$(CC) $(CFLAGS) -c -o bin/user_main.o user_main.c
	$(CC) $(LDFLAGS) bin/user_main.o bin/nanohttp.o $(LDLIBS) -o bin/user_main 
	./esptool.py elf2image bin/user_main 

nanohttp:
	clear
	$(CC) $(CFLAGS) -c -o bin/nanohttp.o nanohttp.c

flash:
	./esptool.py -b 460800 write_flash 0 bin/user_main-0x00000.bin 0x10000 bin/user_main-0x10000.bin 
