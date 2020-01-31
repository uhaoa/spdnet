#ifndef SPDNET_NET_CONNECTOR_H_
#define SPDNET_NET_CONNECTOR_H_
#include <memory>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <spdnet/base/noncopyable.h>
#include <spdnet/net/socket.h>

namespace spdnet
{
namespace net
{
    class AsyncConnectAddr ; 
    class ConnectorContext;

    class AsyncConnector : public base::NonCopyable , public std::enable_shared_from_this<AsyncConnector>
    {
     public:
        using Ptr = std::shared_ptr<AsyncConnector> ; 
        using AsyncTask = std::function<void()> ; 
        using SuccessCallback = std::function<void(TcpSocket::Ptr)> ; 
        using FailedCallback  = std::function<void()> ; 
        
        class ConnectOptions
        {
            public:
                class Options ; 
                using ConnectOptionsFunc = std::function<void(Options& option)> ; 

                static ConnectOptionsFunc WithAddr(const std::string& ip , int port);
                static ConnectOptionsFunc WithSuccessCallback(SuccessCallback&&);
                static ConnectOptionsFunc WithFailedCallback(FailedCallback&&);
                
        } ;

        void AsyncConnect(const std::vector<ConnectOptions::ConnectOptionsFunc>& options); 
        void StartWorkerThread();    
        void ProcessAyncTask();
        static Ptr Create();

     private:

        AsyncConnector();
        virtual ~AsyncConnector();

        std::shared_ptr<bool> is_run_ ; 
        std::shared_ptr<std::thread> thread_ ; 

        std::mutex              async_tasks_guard_; 
        std::vector<AsyncTask>  async_tasks_ ; 

        std::shared_ptr<ConnectorContext>  context_ ;
    };


}
}


#endif  // SPDNET_NET_CONNECTOR_H_