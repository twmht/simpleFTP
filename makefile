my_ftp=myftp
main_server=myftpServer
main_client=myftpClient
all: $(main_server).c $(main_client).c $(my_ftp).o
	gcc $(my_ftp).o $(main_server).c -o $(main_server)
	gcc $(my_ftp).o $(main_client).c -o $(main_client)
$(my_ftp).o: $(my_ftp).h $(my_ftp).c
	gcc -c $(my_ftp).c
clean:
	rm -f $(my_ftp).o $(main_client) $(main_server)
