# Configuration

CC			= gcc
CFLAGS		= -g -std=gnu99 -Wall -Iinclude -fPIC -pthread 
TARGET		= client/wordle server/wordleserver

# Rules
make: $(TARGET)

$(TARGET): client/client.c server/server.c
	$(CC) $(CFLAGS) -o client/wordle client/client.c
	$(CC) $(CFLAGS) -o server/wordleserver server/server.c 

clean:
	@echo "Removing  executables"
	@rm -f client/wordle server/wordleserver