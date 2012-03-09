# Makefile for the DKProtocol project
# By Lee Davis and Garrett Koller

CC=gcc
CFLAGS=-c -Wall
LDFLAGS=

SRC_DIR=../

DKP_HDR=$(SRC_DIR)dkp.h

DKP_SRC=$(SRC_DIR)dkp.c
SERVER_SRC=$(SRC_DIR)server.c
CLIENT_SRC=$(SRC_DIR)client.c

DKP_OBJ=dkp.o
SERVER_OBJ=server.o
CLIENT_OBJ=client.o
OBJS=$(DKP_OBJ) $(SERVER_OBJ) $(CLIENT_OBJ)

SERVER_EXE=server
CLIENT_EXE=client
BASIC_SERVER_EXE=basic_server
BASIC_CLIENT_EXE=basic_client
EXES=$(SERVER_EXE) $(CLIENT_EXE) $(BASIC_SERVER_EXE) $(BASIC_CLIENT_EXE)

RM := rm

# All Target
all: server client

# Tool invocations
#.c.o:
#	$(CC) $(CFLAGS) $< -o $@
	
$(DKP_OBJ): $(DKP_SRC) $(DKP_HDR)
	$(CC) $(CFLAGS) $(DKP_SRC) -o $(DKP_OBJ)

$(SERVER_OBJ): $(SERVER_SRC) $(DKP_HDR)
	$(CC) $(CFLAGS) $(SERVER_SRC) -o $(SERVER_OBJ)

$(CLIENT_OBJ): $(CLIENT_SRC) $(DKP_HDR)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_OBJ)

server: $(DKP_OBJ) $(SERVER_OBJ)
	@echo 'Building target: $@'
	$(CC)  -o $(SERVER_EXE) $(SERVER_OBJ) $(DKP_OBJ)
	@echo 'Finished building target: $@'
	@echo ' '

client: $(DKP_OBJ) $(CLIENT_OBJ)
	@echo 'Building target: $@'
	$(CC)  -o $(CLIENT_EXE) $(CLIENT_OBJ) $(DKP_OBJ)
	@echo 'Finished building target: $@'
	@echo ' '
	
basic_server:
	@echo 'Building target: $@'
	$(CC)  -o $(BASIC_SERVER_EXE) ../$(@).c
	@echo 'Finished building target: $@'
	@echo ' '

basic_client:
	@echo 'Building target: $@'
	$(CC)  -o $(BASIC_CLIENT_EXE) ../$(@).c
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	$(RM) -f $(OBJS) $(EXES)
	@echo ' '
