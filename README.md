Protobuf DEMO 
======================

This project uses protobuf and boost::asio to send files by c++ code. 

It depends on openssl library to calculate file md5sum. You need install openssl before build.

For centos 7:

	yum install openssl-devel openssl -y

compile:
-------------
	protoc -I=. --cpp_out=. package.proto
	mkdir build
	cd build
	cmake ..
	make


How to use
-------------
server side:

	./protoRecv -p 8080

client side:

	./protoSend -s 192.168.100.100 -p 8080 -f file1 file2 file3


