#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <bits/stdc++.h>
#include <iostream>
#include <libgen.h>
#include <openssl/md5.h>
#include <boost/asio.hpp>

#include "package.pb.h"
  
using namespace std;

const uint32_t HEADERMAGIC =  0x123456;

struct Header{
    uint32_t      magic;
    uint64_t      length;
}__attribute__((__packed__));


boost::asio::ip::tcp::socket connectToSrv(boost::asio::io_service *io_service, char* srvIp, int srvPort){
    boost::system::error_code error;
    boost::asio::ip::tcp::socket sock(*io_service);
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address_v4::from_string(srvIp), srvPort);
    sock.connect(endpoint, error);
    return sock;
}

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

void printHelp(){
    cout << "-p   server port, default port is 5000" << endl;
    cout << "-s   server ip address" << endl;
    cout << "-f   file names will be sent" << endl;
    exit(0);
}


int main(int argc, char** argv){
    int srvPort = 0;
    char* filePath[100];
    char* srvIp = 0;
    int count = 0;
    int i = 0, len = 0, fileNum;

    struct Header *header = (struct Header*)malloc(sizeof(struct Header));
    header->magic = HEADERMAGIC;

    for (count = 1; count < argc; ++count) {
        if (strcmp("-h", argv[count])==0 || strcmp("--help", argv[count])==0){
            printHelp();
        }
        if (strcmp("-p", argv[count])==0){
            if((count+1)<argc){
                srvPort = atoi(argv[count+1]);
            }else{
                cout<<"cmd param error"<<endl;
            }
        }
        
        if (strcmp("-s", argv[count])==0){
            if((count+1)<argc){
                if(strlen(argv[count+1]) > 15 || strlen(argv[count+1])<7){
                    cout << "Server ip error!!" << endl;
                }else{
                    srvIp = argv[count+1];
                }
            }else{
                    cout<<"cmd param error"<<endl;
            }
        }
        
        if (strcmp("-f", argv[count])==0){
            while((count+1)<argc){
                filePath[i] = argv[count+1];
                i++;
                count++;
            }
            fileNum = i;
        }
    }
    
    if(srvPort<=0){
        srvPort = 5000;
        cout <<"using 5000 port!!!" << endl;
    }
    
    if(srvIp == 0){
        cout << "Error!!!Server ip is empty, using -s server ip!!!" << endl;
        cout << "" <<endl;
        cout << "" <<endl;
        printHelp();
        exit(-1);
    }
   
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::socket srvSock =  connectToSrv(&io_service, srvIp, srvPort); 
    
    ProtoMessage::Package pack;

    cout << "fileNum:" << fileNum << endl;   
 
    for(i=0;i<fileNum;i++){
        FILE *inFile = fopen (filePath[i], "rb");
        if (inFile == NULL) {
            printf( "%s can't be opened.\n", filePath[i]);
            continue;
        }
        fseek(inFile, 0L, SEEK_END);
        header->length = ftell(inFile);
        fclose(inFile);
        
        char* strMd5 = fileMd5(filePath[i]);
        if(strMd5==NULL){
            cout << "Get file md5 error!!!" << endl;
            continue;
        }

        std::ifstream ifile(filePath[i],std::ios_base::binary);
        if (!ifile.is_open())
        {
            std::cout << "open file "<< filePath[i] << " error!!" << std::endl;
        }
        pack.set_id(i);
        pack.set_filename(basename(filePath[i]));
        pack.set_md5(strMd5);
        free(strMd5);
        
        std::stringstream  streambuffer;
        streambuffer<<ifile.rdbuf();
        std::string sdata(streambuffer.str());
        pack.set_content(sdata);
        ifile.close();

        boost::asio::streambuf sbuf;
        ostream serialized(&sbuf);
        pack.SerializeToOstream(&serialized);    
        uint32_t message_length = sbuf.size();
        header->length = message_length;
        try{
            boost::asio::write(srvSock, boost::asio::buffer((void*)header, sizeof(struct Header)));
            boost::asio::write(srvSock, sbuf);
        }catch (boost::system::system_error& e){
            cout<<"Send Header error"<<endl;
            std::cout << e.code() << " : " << e.what() << std::endl;
            exit(-1);
        }
    }
            
    return 0;
}

