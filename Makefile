FLAGS=-lX11 -lm -lXext
PROG=main
SOURCE=$(PROG).c
CC=gcc

all: $(PROG)

$(PROG): $(SOURCE)
	$(CC) $< -o $@ $(FLAGS)

clean:
	rm -f $(PROG)
