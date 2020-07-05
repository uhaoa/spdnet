#include <stdio.h>
#include <iostream>
#include <spdnet/net/connector.h>
#include <spdnet/net/event_service.h>
#include <atomic>
//#include <gperftools/profiler.h>

struct SessionMessage {
    int number;
    int length;
}__attribute__((__packed__));

struct PayloadMessage {
    int length;
    char data[0];
};

/*
void gprofStartAndStop(int signum) {
    static int isStarted = 0;
    if (signum != SIGUSR1) return;

    //通过isStarted标记未来控制第一次收到信号量开启性能分析，第二次收到关闭性能分析。
    if (!isStarted){
        isStarted = 1;
        ProfilerStart("ttcp_client.prof");
        printf("ProfilerStart success\n");
    }else{
        ProfilerStop();
        isStarted = 0  ; 
        printf("ProfilerStop success\n");
    }
}

*/

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "usage: <host> <port> <thread num> <session num> <number> <length>\n");
        exit(-1);
    }
    // signal(SIGUSR1, gprofStartAndStop);

    std::atomic_int cur_client_num = ATOMIC_VAR_INIT(0);

    spdnet::net::EventService service;
    service.runThread(atoi(argv[3]));
    spdnet::net::AsyncConnector connector(service);

    auto session_msg = std::make_shared<SessionMessage>();
    int number = atoi(argv[5]);
    int length = atoi(argv[6]);
    session_msg->number = htonl(number);
    session_msg->length = htonl(length);

    const int msg_len = sizeof(int) + length;
    PayloadMessage *msg = static_cast<PayloadMessage *>(::malloc(msg_len));
    msg->length = htonl(length);
    for (int i = 0; i < length; i++)
        msg->data[i] = "0123456789ABCDEF"[i % 16];
    cur_client_num = atoi(argv[4]);

    for (int i = 0; i < cur_client_num; i++) {
        std::shared_ptr<int> number_ptr = std::make_shared<int>(number);
        connector.asyncConnect(spdnet::net::EndPoint::ipv4(argv[1], atoi(argv[2])),
                               [&service, session_msg, msg, number_ptr, length, &cur_client_num](
                                       std::shared_ptr<spdnet::net::TcpSession> new_conn) {
                                   new_conn->setDataCallback(
                                           [new_conn, msg, number_ptr, length, &cur_client_num](const char *data,
                                                                                                size_t len) mutable -> size_t {
                                               if (len >= static_cast<size_t>(sizeof(int))) {
                                                   if (--*number_ptr > 0)
                                                       new_conn->send((char *) msg, length + sizeof(int));
                                                   else {
                                                       new_conn->postShutDown();
                                                       --cur_client_num;
                                                   }
                                                   return sizeof(int);
                                               }
                                               return 0;
                                           });
                                   new_conn->setDisconnectCallback([](std::shared_ptr<spdnet::net::TcpSession> connection) {
                                       std::cout << "tcp connection disconnect " << std::endl;

                                   });
                                   new_conn->send((char *) &*session_msg, sizeof(SessionMessage));
                               },
                // failed cb
                               []() {
                                   std::cout << "connect failed " << std::endl;
                               });
    }


    std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (cur_client_num == 0) {
            break;
        }
    }

    std::chrono::steady_clock::time_point end_time = std::chrono::steady_clock::now();
    int cost_second = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time).count();

    long long total_len = (atoi(argv[4]) * (long long) number * (long long) length) / 1024 / 1024;

    std::cout << cost_second << " seconds" << std::endl;
    if (cost_second > 0)
        std::cout << total_len / cost_second << " Mib/s" << std::endl;
    free(msg);
    return 0;

} 
