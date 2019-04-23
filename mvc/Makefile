# CROSS_COMPILE := arm-none-linux-gnueabi-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
CFLAGS = -Wall -g -O -fPIC
LDFLAGS := -L./
LIBS    := -lobserver
INCLUDES := -I./

LIB_TAG = libobserver.so
APP_TAG = test

LIB_OBJS = observer.o subject.o
LED_OBJS = test.o

all : $(LIB_TAG) $(APP_TAG)

$(LIB_TAG) : $(LIB_OBJS)
	$(CC) -shared -fPIC -o $(LIB_TAG) $(LIB_OBJS)

$(APP_TAG): $(LED_OBJS) $(LIB_TAG)
	$(CC) -o $@ $(LED_OBJS) $(INCLUDES) $(LDFLAGS) $(LIBS)
	$(STRIP) -x $(APP_TAG)

$(LIB_OBJS) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(LED_OBJS) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(APP_TAG) *.o $(LIB_TAG)
