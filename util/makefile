CC     = gcc
#LFLAGS = -L /usr/lib -L /lib -l/lib/libc.a 
#LFLAGS = -L ./lib -L /usr/lib -L /lib -lc -lgcc -lsocket -lnsl

#### Debug version flags
#### Linux:
CFLAGS = -g -Wall -DLINUX
LFLAGS = 
#### BSD:
#CFLAGS = -g -Wall -Wno-unused
#### Solaris: (daydream)
#CFLAGS = -g -Wall -DSOLARIS -I ../../include
#LFLAGS = -L ../../lib -lsocket -lnsl

# Final version flags - stripped symbols so impossible to debug
# CFLAGS = -s -O2 -Wall

all: nocr_all nocr delete_password

nocr_all: nocr_all.c
	$(CC) $(CFLAGS) -o nocr_all nocr_all.c

nocr: nocr.c
	$(CC) $(CFLAGS) -o nocr nocr.c

delete_password: delete_password.c
	$(CC) $(CFLAGS) -o delete_password delete_password.c
