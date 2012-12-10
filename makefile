header=myftp.h
client=client
server=server
main_server=myftpServer
main_client=myftpClient
all: $(main_server).c $(main_client).c $(client).o $(server).o
	gcc $(server).o $(main_server).c -o $(main_server)
	gcc $(client).o $(main_client).c -o $(main_client)
$(client).o: $(header) $(client).c
	gcc -c $(client).c
$(server).o: $(header) $(server).c
	gcc -c $(server).c
clean:
	rm -f *.o
