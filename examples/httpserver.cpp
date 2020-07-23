#include <cstdio>
#include <iostream>
#include <atomic>
#include <spdnet/net/event_service.h>
#include <spdnet/net/http/http_server.h>

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
    server.start(spdnet::net::end_point::ipv4("127.0.0.1", atoi(argv[1])), [](std::shared_ptr<http_session> session){
		session->set_http_request_callback([](const http_request& request, std::shared_ptr<http_session> session) {
			http_response response;
			response.set_body("<html>hello http ..</html>");
			session->send_response(response);
		}); 
    });


    getchar();
    return 0;

}