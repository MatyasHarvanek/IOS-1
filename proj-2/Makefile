CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror  -pedantic
LFLAGS = -lrt -lpthread 

proj2: proj2.o
	$(CC) $(CFLAGS) $< -o $@ $(LFLAGS)
%.o : %.c
	$(CC) $(CFLAGS) -o $@ -c $<