#include <iostream>
#include <fstream>
#include <string>
#include <boost/asio.hpp>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <openssl/md5.h>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind.hpp>


#include "package.pb.h"

using namespace std;
using namespace boost::asio;

const uint32_t HEADERMAGIC =  0x123456;

struct Header{
    uint32_t      magic;
    uint64_t      length;
}__attribute__((__packed__));



typedef boost::shared_ptr<ip::tcp::socket> socket_ptr;


char* fileMd5(const char* filePath){
    MD5_CTX mdContext;
    unsigned char achbyBuff[17] = {0};
    unsigned char *pbyBuff = achbyBuff;
    MD5_Init (&mdContext);
    FILE *inFile = fopen (filePath, "rb");
    if (inFile == NULL) {
        printf( "%s can't be opened.\n", filePath);
        return NULL;
    }

    int bytes;
    unsigned char data[1024];

    while ((bytes = fread (data, 1, 1024, inFile)) != 0){
        MD5_Update (&mdContext, data, bytes);
    }
    
    MD5_Final (pbyBuff,&mdContext);
    char* strMd5 = (char*)malloc(40);
    for(int i = 0; i < MD5_DIGEST_LENGTH; i++){
        sprintf(&strMd5[i*2], "%02x", pbyBuff[i]);
    }
    fclose(inFile);
    return strMd5;
}


int ParsePackage(const ProtoMessage::Package& pack) {
    cout << " ID: "  << pack.id() << endl;
    cout << " FILENAME: " << pack.filename().c_str() << endl;
    cout << " MD5: " << pack.md5() << endl;

    if(pack.content().length() == 0){
        cout << " File content is empty!!!" << endl;
        return 0;
    }
    if(pack.md5().length() == 0){
        cout << " File MD5 is empty!!!" << endl;
        return 0;
    }
    fstream destFile(pack.filename(), ios::binary|ios::out);
    if (!destFile) {
        cout << "Error opening destination file." << endl;
        return -1;
    }
    destFile << pack.content() ;
    destFile.close();    
    
    char* szBuf = fileMd5(pack.filename().c_str());
    if(szBuf == NULL) {
        cout << "Get file md5 error!!!" << endl;
        return -1;
    }
    
    char strMd5[40]={0};
    sprintf(strMd5, pack.md5().c_str());
    if(strcmp(strMd5, szBuf)==0){
        cout << " *********************************************************************** " << endl;
        cout << " Receive file " << pack.filename() << " successful!!! " << endl;    
        cout << " " << endl;
        cout << " " << endl;
        free(szBuf);
        return 0;
    }else{
        cout << " Receive file "<< pack.filename() << " failed!!!" << " File content is error!!!" << endl;
        cout << " " << endl;
        cout << " " << endl;
        free(szBuf);
        return -1;
    }
}

void client_session(socket_ptr sock) 
{
    try{
        ProtoMessage::Package pack;
        int ret = 0;
        uint32_t len = 0;

        uint32_t message_length = 0;

        struct Header* header = (struct Header*)malloc(sizeof(struct Header));
    
        while(1){
            memset(header, 0, sizeof(struct Header));
            len = sock->receive(boost::asio::buffer(header, sizeof(struct Header)));
            if ( len <=0 ){
                cout << "socket error!!" << endl;
                sock->close();
                free(header);
                return;
            }

            if(header->magic != HEADERMAGIC){
                cout << "recv header error!!!" << endl;
                free(header);
                sock->close();
                return;
            }

            message_length = header->length;
            char* buf = (char*)malloc(message_length);
            if(buf==0){
                cout << "Malloc buffer error!!!" << endl;
                free(header);
                sock->close();
                return;
            }
            len = 0;
            while(len < message_length) {
                ret = sock->receive(boost::asio::buffer(buf + len, message_length - len));
                if(ret < 0 ){
                    cout << "socket error, " << strerror(errno) << endl;
                    free(header);
                    free(buf);
                    sock->close();
                    return;
                }
                len += ret;
            }
            if (len == message_length) {
                if(pack.ParseFromArray(buf, message_length) != 1) {    
                    std::cout << "Failed to parse package." << std::endl;
                    free(header);
                    free(buf);
                    sock->close();
                    return;
                }else{
                    ret = ParsePackage(pack);
                    free(buf);
                }
            }else{
                cout << "recv file content error!!!" << strerror(errno) << endl;
                free(header);
                free(buf);
                sock->close();
                return;
            }
        }
    } catch (exception& e) {
        cerr << e.what() << endl;
    }
}

int main(int argc, char* argv[]) {
    GOOGLE_PROTOBUF_VERIFY_VERSION;
    int port=0;
    int count = 0;
    if (argc == 2) {
        cout << " -p  port  " << endl;
        exit(0);
    }
    for (count = 1; count < argc; ++count) {
        if (strcmp("-p", argv[count])==0){
            if((count+1)<argc){
                port = atoi(argv[count+1]);
            }else{
                cout<<"cmd param error"<<endl;
            }
        }
    }    
    if(port<=0){
        port = 5000;
    }
    cout <<"using " << port  << " port!!!" << endl;

    typedef boost::shared_ptr<ip::tcp::socket> socket_ptr;
    io_service service;
    ip::tcp::endpoint ep(boost::asio::ip::address_v4::from_string("0.0.0.0"), port); 
    ip::tcp::acceptor acc(service, ep);
    while ( true) {
        socket_ptr sock(new ip::tcp::socket(service));
        acc.accept(*sock);
        boost::thread(boost::bind(client_session, sock));
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}

