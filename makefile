CC=gcc
CFLAGS=-Wall
LDFLAGS=-lrt -lGL -lGLU -lglut  # Link with real-time and OpenGL libraries

# Default target
all: main test workers plane cleaner
# Compile main.c into an executable named main
main: main.c
	$(CC) $(CFLAGS) -o main main.c $(LDFLAGS) -lpthread -lm

# Compile test.c into an executable named test
test: test.c
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS)

# Compile workers.c into an executable named workers
workers: workers.c
	$(CC) $(CFLAGS) -o workers workers.c $(LDFLAGS) -lpthread

plane: plane.c
	$(CC) $(CFLAGS) -o plane plane.c $(LDFLAGS) -lpthread

cleaner: clean.c
	$(CC) $(CFLAGS) -o cleaner clean.c $(LDFLAGS) -lpthread

# Clean up the compiled files and any other temporary files
clean:
	rm -f main test workers plane cleaner

.PHONY: all clean

