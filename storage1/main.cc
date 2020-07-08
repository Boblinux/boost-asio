#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <thread>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/socket_base.hpp>
#include <thread>
#include "upLoad.h"
#include "downLoad.h"

using boost::asio::ip::tcp;

class server
{
public:
  server(boost::asio::io_context& io_context, short port)
    : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
			io_context_(io_context)
  {
    start();
  }

  void start()
  {
    tcp::socket socket(io_context_);
    acceptor_.accept(socket);
    boost::system::error_code ec_;
    std::vector<char> data_(8);
    boost::asio::read(socket, boost::asio::buffer(&*data_.begin(), 8), ec_); 
    if(ec_)
    {
      std::cout << "start error" << '\n';
      start();
    }
    //std::cout << std::string(&*data_.begin(), 8).data() << '\n';
    if(std::string(&*data_.begin(), 8) == "upppload")
    {
      std::thread th([&socket]()
                        {
                          std::make_shared<upLoad>(socket)->start();
                        });
      th.detach();
    }
    else if(std::string(&*data_.begin(), 8) == "download")
    {
      std::thread th([&socket]()
                        {
                          std::make_shared<downLoad>(socket)->download();
                        });
      th.detach();
    }
    start();
  }



private:
  tcp::acceptor acceptor_;
  boost::asio::io_context& io_context_;
};

int main(int argc, char* argv[])
{
  try
  {
    boost::asio::io_context io_context;
    server s(io_context, 9980);
    std::thread t([&io_context](){ io_context.run(); });
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
