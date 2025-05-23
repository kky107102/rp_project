CC = aarch64-linux-gnu-gcc
CFLAGS = -fPIC -I /usr/aarch64-linux-gnu/include
LDFLAGS = -L /usr/aarch64-linux-gnu/lib -lwiringPi -lpthread

all: libshared.so webserver scp

libshared.so: device_control.c
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

webserver: webserver.c web_response.c
	$(CC) -o $@ $^ -ldl $(CFLAGS) $(LDFLAGS) -lcrypt

scp:
	~/scp.sh libshared.so
	~/scp.sh webserver

clean:
	rm -f libshared.so webserver
