#include <cstdio>
#include <iostream>
#include <atomic>
#include <spdnet/net/event_service.h>
#include <spdnet/net/http/http_connector.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "usage : <port> <thread num>\n");
        exit(-1);
    }
    using namespace spdnet::net ;
    using namespace spdnet::net::http;

    event_service service;
    service.run_thread(atoi(argv[2]));

    http_connector connector(service) ;
    // http
	connector.async_connect(spdnet::net::end_point::ipv4("127.0.0.1", atoi(argv[1])), [](std::shared_ptr<http_session> session){
		session->set_http_response_callback([](const http_response& response, std::shared_ptr<http_session> session) {
			session->shutdown();
		});

		http_request req; 
        req.set_url("/xu/hao/"); 
        //req.add_header("Host", "www.xuhao.com");
        req.add_query_param("key1", "val1");
        req.add_query_param("key2", "val2");
        req.add_query_param("key3", "val3");
        req.add_query_param("key4", "val4");
        //req.set_body("hello http server");
        //req.set_version(http_version(1, 1)); 
        req.set_method(HTTP_GET); 
		session->send_request(req);
		}, []() {
			std::cout << "connect failed" << std::endl; 
		});

    // websocket
    connector.async_connect(spdnet::net::end_point::ipv4("127.0.0.1", atoi(argv[1])), [](std::shared_ptr<http_session> session){
        session->set_ws_handshake_success_callback([session](){
            std::cout << "handshake success ..." << std::endl;
            websocket_frame frame1("hi websocket server." , ws_opcode::op_text_frame , true , true);
            session->send_ws_frame(frame1);
            websocket_frame frame2("hello world" , ws_opcode::op_text_frame , true , false);
            session->send_ws_frame(frame2);
        });

        session->set_ws_frame_enter_callback([](const websocket_frame& frame_data, std::shared_ptr<http_session> session){
            std::cout << "recv ws frame . opcode:" << (uint32_t)frame_data.get_opcode()  << "payload content:" << frame_data.get_payload()<< std::endl;
        });

        session->start_ws_handshake();
    }, []() {
        std::cout << "connect failed" << std::endl;
    });
    getchar();
    return 0;

}