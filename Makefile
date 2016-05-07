all:
	gcc mcast_client.c -o client
	gcc mcast_server.c -o server
clean:
	rm client
	rm server