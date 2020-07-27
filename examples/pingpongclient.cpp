#include <stdio.h>
#include <iostream>
#include <spdnet/net/event_service.h>
#include <spdnet/net/connector.h>

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "usage: <host> <port> <thread num> <data size> <client num>\n");
        exit(-1);
    }
    auto send_data = std::make_shared<std::string>(atoi(argv[4]), 'a');
    spdnet::net::event_service service;
    service.run_thread(atoi(argv[3]));

    spdnet::net::async_connector connector(service);

    for (int i = 0; i < atoi(argv[5]); i++) {
        connector.async_connect(spdnet::net::end_point::ipv4(argv[1], atoi(argv[2])),
                                [send_data](std::shared_ptr<spdnet::net::tcp_session> session) {
                                    //std::cout << "connect success " << std::endl;
                                    session->set_data_callback([session](const char *data, size_t len) -> size_t {
                                        session->send(data, len);
                                        return len;
                                    });
                                    session->set_disconnect_callback(
                                            [](std::shared_ptr<spdnet::net::tcp_session> session) {
                                                std::cout << "tcp connection disconnect " << std::endl;
                                            });

                                    session->send(send_data->c_str(), send_data->length());
                                },

                                []() {
                                    std::cout << "connect failed " << std::endl;
                                });
    }
    getchar();
    return 0;

} 
