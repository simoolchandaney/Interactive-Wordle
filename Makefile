# Configuration

CC			= gcc
CFLAGS		= -g -std=gnu99 -Wall -Iinclude -fPIC -pthread 
TARGET		= client/mpwordle server/mpwordleserver

# Rules
make: $(TARGET)

$(TARGET): client/client.c server/server.c
	$(CC) $(CFLAGS) -o client/mpwordle cJSON.c client/client.c
	$(CC) $(CFLAGS) -o server/mpwordleserver cJSON.c server/server.c 

clean:
	@echo "Removing  executables"
	@rm -f client/mpwordle server/mpwordleserver