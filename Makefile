server: server.c
	gcc -I . client.c -o echos

clean:
	$(RM) *.o