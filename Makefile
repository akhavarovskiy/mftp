all: mftp mftpserve

mftp: include/mftp.h src/mftp.c
	gcc -g -Wall -pedantic src/mftp.c -o mftp -lpthread -std=gnu99

mftpserve: include/mftpserve.h src/mftpserve.c
	gcc -g -Wall -pedantic src/mftpserve.c -o mftpserve -lpthread -std=gnu99

clean:
	rm mftp
	rm mftpserve