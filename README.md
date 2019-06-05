# syspro3

make && ./dropbox_server -p 9002
-p server port

./dropbox_client -d ./input1/ -p 9006 -s 9002 -i strvamv
-d input directory in format ./xxxxx/
-p clients port
-s server port
-i server ip

the linked list holds tuples.
A tuple is either ip(hostname),port or filepath,version.

Haven't completed buffersz, workerthreads, version, some error messages.
Program doesn't send ip address in binary form, instead it sends hostname as string.
To be 100% safe a client must complete its file exchanges in order for another
client to log in.
