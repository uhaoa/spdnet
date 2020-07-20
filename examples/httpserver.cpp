#include <cstdio>
#include <iostream>
#include <atomic>
#include <spdnet/net/event_service.h>
#include <spdnet/net/http/http_server.h>

std::atomic_llong total_recv_size = ATOMIC_VAR_INIT(0);
std::atomic_llong total_client_num = ATOMIC_VAR_INIT(0);
std::atomic_llong total_packet_num = ATOMIC_VAR_INIT(0);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage : <port> <thread num>\n");
        exit(-1);
    }
    using namespace spdnet::net ;
    using namespace spdnet::net::http;

    event_service service;
    service.run_thread(atoi(argv[2]));

    http_server server(service) ;
    server.start(spdnet::net::end_point::ipv4("0.0.0.0", atoi(argv[1])), [](const http_request& request, std::shared_ptr<http_session> session){
        std::cout << "hello http" << std::endl;
    });


    getchar();
    return 0;

}