#include <stdio.h>
#include <iostream>
#include <spdnet/net/tcp_service.h>    
#include <atomic>
#include <gperftools/profiler.h>

std::atomic_llong TotalRecvSize = ATOMIC_VAR_INIT(0) ; 
std::atomic_llong total_client_num = ATOMIC_VAR_INIT(0);
std::atomic_llong total_packet_num = ATOMIC_VAR_INIT(0);
void gprofStartAndStop(int signum) {
    static int isStarted = 0;
    if (signum != SIGUSR1) return;

    //通过isStarted标记未来控制第一次收到信号量开启性能分析，第二次收到关闭性能分析。
    if (!isStarted){
        isStarted = 1;
        ProfilerStart("pingpongserver.prof");
        printf("ProfilerStart success\n");
    }else{
        ProfilerStop();
        printf("ProfilerStop success\n");
    }
}

int main(int argc , char* argv[])
{
    if(argc != 3)
    {
        fprintf(stderr , "usage : <port> <thread num>\n");
        exit(-1); 
    }
	signal(SIGUSR1, gprofStartAndStop);
	spdnet::net::TcpService::Ptr service = spdnet::net::TcpService::Create();	
	service->LaunchWorkerThread(atoi(argv[2])) ;
	spdnet::net::TcpService::ServiceParamBuilder builder; 
	builder.WithNoDelay().WithTcpEnterCallback([](spdnet::net::TcpConnection::Ptr new_conn) {
		total_client_num++;
		new_conn->SetDataCallback([new_conn](const char* data, size_t len)->size_t {
			new_conn->Send(data, len);
			TotalRecvSize += len;
			total_packet_num++;
			return len;
		});
		new_conn->SetDisconnectCallback([](spdnet::net::TcpConnection::Ptr connection) {
			total_client_num--;
		});

	});
	service->StartServer( "0.0.0.0" , atoi(argv[1])  , builder.GetParams());
					

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1)) ; 
        std::cout << "total recv : " << (TotalRecvSize / 1024) / 1024 << " M /s, of client num:" << total_client_num
 << std::endl;
        std::cout << "packet num:" << total_packet_num << std::endl;
        total_packet_num = 0;
        TotalRecvSize = 0;
    }

	getchar();
	return 0; 

} 
