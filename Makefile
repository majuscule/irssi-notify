all: server client

server:
	gcc server.c -o irssi-notify
client:
	gcc client.c -o irssi-listen

clean:
	rm irssi-notify irssi-listen
