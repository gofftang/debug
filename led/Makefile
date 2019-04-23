#CROSS_COMPILE := arm-none-linux-gnueabi-
CC = $(CROSS_COMPILE)gcc
STRIP = $(CROSS_COMPILE)strip
CFLAGS = -Wall -g -O -fPIC
LDFLAGS := -L./
LIBS    := -lled -lpthread
INCLUDES := -I./

LIB_LED = libled.so
TEST = test

LIB_OBJS = render.o service.o
LED_OBJS = test.o

all : $(LIB_LED) $(TEST)

$(LIB_LED) : $(LIB_OBJS)
	$(CC) -shared -fPIC -o $(LIB_LED) $(LIB_OBJS)

$(TEST): $(LED_OBJS) $(LIB_LED)
	$(CC) -o $@ $(LED_OBJS) $(INCLUDES) $(LDFLAGS) $(LIBS)
	$(STRIP) -x $(TEST)

$(LIB_OBJS) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(LED_OBJS) : %.o : %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TEST) *.o $(LIB_LED)

install: $(LIB_LED)
	cp $(LIB_LED) ../lib/unione/
	cp service.h ../inc/uni_led.h
