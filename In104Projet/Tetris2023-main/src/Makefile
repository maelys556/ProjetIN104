CC=gcc
CFLGAS=-Wall -Werror

all: main.x

%.x: %.o
	$(CC) $(CFLGAS) -o $@ $<

%.o : %.c
	$(CC) $(CFLGAS) -c $<

clean:
	rm -f *~ *.o

realclean: clean
	rm -f *.x
