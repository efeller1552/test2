server: server.c
	gcc -I . server.c -o echos

clean:
	$(RM) *.o