all: myftpServer.c myftpClient.c myftp.c myftp.h
	gcc -o myftpServer myftpServer.c myftp.c
	gcc -o myftpClient myftpClient.c myftp.c

clean:
	rm -f myftpServer myftpClient
