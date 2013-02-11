all: server client

server:
	gcc notify.c -o irssi-notify
client:
	gcc listen.c -o irssi-listen

clean:
	rm irssi-notify irssi-listen
