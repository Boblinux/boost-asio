#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/socket_base.hpp>
#include <deque>
#include <thread>
#include "upload.h"

using boost::asio::ip::tcp;

class server
{
public:
  server(boost::asio::io_context& io_context,
        const tcp::endpoint& endpoint1,
        const tcp::endpoint& endpoint2,
        const tcp::endpoint& endpoint3,
        short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
			io_context_(io_context),
      socket1_(io_context_),
      socket2_(io_context_),
      socket3_(io_context_)
  {
    socket1_.connect(endpoint1);
    socket2_.connect(endpoint2);
    socket3_.connect(endpoint3);
    start();
  }

  void close()
  {
    boost::asio::post(io_context_, [this]() { socket1_.close(); });
    boost::asio::post(io_context_, [this]() { socket2_.close(); });
    boost::asio::post(io_context_, [this]() { socket3_.close(); });
  }

private:
  void start()
  {
    tcp::socket socket(io_context_);
    acceptor_.accept(socket);
    // std::cout << "connect success\n";
    std::thread th([this, &socket]()
                      {
                        std::make_shared<upload>(socket,
                                                socket1_,
                                                socket2_,
                                                socket3_)->start();
                      });
    th.detach();

    start();
  }

  tcp::acceptor acceptor_;
  boost::asio::io_context& io_context_;
  tcp::socket socket1_;
  tcp::socket socket2_;
  tcp::socket socket3_;
};

int main(int argc, char* argv[])
{
  try
  {
    boost::asio::io_context io_context;
    tcp::resolver resolver(io_context);

    tcp::endpoint endpoint1(boost::asio::ip::address::from_string("127.0.0.1"), 9980);
    tcp::endpoint endpoint2(boost::asio::ip::address::from_string("127.0.0.1"), 9982);
    tcp::endpoint endpoint3(boost::asio::ip::address::from_string("127.0.0.1"), 9983);
    
    server s(io_context,
             endpoint1, 
             endpoint2,
             endpoint3,
             9981);

    std::thread t([&io_context](){ io_context.run(); });
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}