all:shell.c
	gcc -std=c99 -Wall -o shell -g shell.c

clean:
	$(RM) shell
