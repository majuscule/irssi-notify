all: irssi-listen irssi-notify

irssi-notify:
	gcc notify.c -o irssi-notify
irssi-listen:
	gcc -o irssi-listen `pkg-config --cflags --libs libnotify` listen.c

clean:
	rm irssi-notify irssi-listen
