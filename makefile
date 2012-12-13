header=myftp.h
client=client
server=server
checksum=checksum
main_server=myftpServer
main_client=myftpClient
all: $(main_server).c $(main_client).c $(client).o $(server).o $(checksum).o
	mkdir -p server
	mkdir -p client
	gcc $(server).o $(checksum).o $(main_server).c -o server/$(main_server)
	gcc $(checksum).o $(client).o  $(main_client).c -o client/$(main_client)
$(client).o: $(header) $(client).c
	gcc -c $(client).c
$(server).o: $(header) $(server).c
	gcc -c $(server).c
$(checksum).o: $(header) $(checksum).c
	gcc -c $(checksum).c
clean:
	rm -f *.o
