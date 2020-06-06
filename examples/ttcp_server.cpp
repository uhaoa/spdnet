#include <stdio.h>
#include <iostream>
#include <spdnet/net/event_service.h>
#include <atomic>
//#include <gperftools/profiler.h>
#include <cassert>
#include <memory>
#include <string.h>
#include <spdnet/net/acceptor.h>

std::atomic_llong total_recv_size = ATOMIC_VAR_INIT(0);
std::atomic_llong total_client_num = ATOMIC_VAR_INIT(0);
std::atomic_llong total_packet_num = ATOMIC_VAR_INIT(0);
/*
void gprofStartAndStop(int signum) {
    static int isStarted = 0;
    if (signum != SIGUSR1) return;

    //通过isStarted标记未来控制第一次收到信号量开启性能分析，第二次收到关闭性能分析。
    if (!isStarted){
        isStarted = 1;
        ProfilerStart("ttcp_server.prof");
        printf("ProfilerStart success\n");
    }else{
        isStarted = 0  ; 
        ProfilerStop();
        printf("ProfilerStop success\n");
    }
}
*/
struct SessionMessage {
    int number;
    int length;
} __attribute__ ((__packed__));

struct PayloadMessage {
    int length;
    char data[0];
};


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage : <port> <thread num>\n");
        exit(-1);
    }
    //signal(SIGUSR1, gprofStartAndStop);

    spdnet::net::EventService service;
    service.runThread(atoi(argv[2]));
    spdnet::net::TcpAcceptor acceptor(service);

    acceptor.start("0.0.0.0", atoi(argv[1]), [](spdnet::net::TcpSession::Ptr new_conn) {
        total_client_num++;
        bool session_recv = false;
        auto session_ptr = std::make_shared<SessionMessage>();
        int payload_len = 0;
        char *payload_data = nullptr;
        new_conn->setDataCallback([new_conn, session_recv, session_ptr, payload_len, payload_data](const char *data,
                                                                                                   size_t len) mutable -> size_t {
            if (session_recv == false) {
                assert(len <= sizeof(SessionMessage));
                if (len >= sizeof(SessionMessage)) {
                    session_recv = true;
                    *session_ptr = *(reinterpret_cast<const SessionMessage *>(data));
                    session_ptr->number = ntohl(session_ptr->number);
                    session_ptr->length = ntohl(session_ptr->length);
                    new_conn->send((const char *) (&session_ptr->length), (size_t) (sizeof(int)));

                    payload_len = 0;
                    payload_data = new char[session_ptr->length];

                    std::cout << "[[[[number : " << session_ptr->number << " length: " << session_ptr->length << "]]]"
                              << std::endl;

                    total_recv_size += sizeof(SessionMessage);
                    return sizeof(SessionMessage);
                }

            } else {
                int prev_len = len;
                assert(len <= sizeof(int) + session_ptr->length);
                if (payload_len == 0) {
                    payload_len = ntohl(*(int *) data);
                    len -= sizeof(int);
                    data += sizeof(int);
                }
                assert(len <= payload_len);
                memcpy(payload_data + session_ptr->length - payload_len, data, len);
                payload_len -= len;
                if (payload_len == 0) {
                    new_conn->send((const char *) (&session_ptr->length), (size_t) (sizeof(int)));
                }
                total_recv_size += prev_len;
                return prev_len;

            }
            return 0;
        });
        new_conn->setDisconnectCallback([](spdnet::net::TcpSession::Ptr connection) {
            total_client_num--;
        });
    });

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "total recv : " << (total_recv_size / 1024) / 1024 << " M /s, of client num:" << total_client_num
                  << std::endl;
        total_packet_num = 0;
        total_recv_size = 0;
    }

    getchar();
    return 0;

} 
