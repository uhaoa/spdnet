#include <stdio.h>
#include <iostream>
#include <spdnet/net/event_service.h>                                           
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
	spdnet::net::EventService service; 
	service.runThread(atoi(argv[3]));

	spdnet::net::AsyncConnector connector(service);
    
	for (int i = 0; i < atoi(argv[5]); i++)
	{
		connector.asyncConnect(argv[1], atoi(argv[2]), [send_data](spdnet::net::TcpSession::Ptr new_conn) {
			//std::cout << "connect success " << std::endl;
			new_conn->setDataCallback([new_conn](const char* data, size_t len)->size_t {
				new_conn->send(data, len);
				return len;
				});
			new_conn->setDisconnectCallback([](spdnet::net::TcpSession::Ptr connection) {
				std::cout << "tcp connection disconnect " << std::endl;
				});

			new_conn->send(send_data->c_str(), send_data->length());
			},

			[]() {
				std::cout << "connect failed " << std::endl;
			});
	}
	getchar();
	return 0; 

} 
