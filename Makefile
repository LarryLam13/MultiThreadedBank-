all: Server Client 
Server: 
	gcc bankingServer.c -lpthread -o bankingServer 
Client: 
	gcc bankingClient.c -lpthread -o bankingClient 
