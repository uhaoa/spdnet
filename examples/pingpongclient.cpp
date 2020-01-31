#include <stdio.h>
#include <iostream>
#include <spdnet/net/tcp_service.h>                                           
#include <spdnet/net/connector.h>
#include <spdnet/net/socket.h>

int main(int argc , char* argv[])
{
	if(argc != 6)
    {
        fprintf(stderr , "usage: <host> <port> <thread num> <data size> <client num>\n");
        exit(-1); 
    }
    auto send_data = std::make_shared<std::string>(atoi(argv[4]) , 'a');
    spdnet::net::TcpService::Ptr service = spdnet::net::TcpService::Create();	
	service->LaunchWorkerThread(atoi(argv[3])) ;
    auto connector = spdnet::net::AsyncConnector::Create() ; 
	connector->StartWorkerThread();
    
    for(int i = 0 ; i < atoi(argv[5]) ; i++)
    {
	    connector->AsyncConnect({spdnet::net::AsyncConnector::ConnectOptions::WithAddr(argv[1] , atoi(argv[2])) , 
							spdnet::net::AsyncConnector::ConnectOptions::WithSuccessCallback([service , send_data](spdnet::net::TcpSocket::Ptr socket){
									//std::cout << "connect success " << std::endl;
									service->AddTcpConnection(std::move(socket) , [send_data](spdnet::net::TcpConnection::Ptr new_conn){
										new_conn->SetDataCallback([new_conn](const char* data , size_t len)->size_t{
											new_conn->Send(data , len) ; 
											//std::string recv(data , len);
											//std::cout << "recv: " << recv << std::endl;
											return len ; 
										}) ; 
										new_conn->SetDisconnectCallback([](spdnet::net::TcpConnection::Ptr connection){
											std::cout << "tcp connection disconnect " << std::endl;

										}); 
										new_conn->Send(send_data->c_str() , send_data->length()) ; 

									}) ; 
							}) ,spdnet::net::AsyncConnector::ConnectOptions::WithFailedCallback([](){
									std::cout << "connect failed " << std::endl;
							}) }) ; 

        
    }

	getchar();
	return 0; 

} 
