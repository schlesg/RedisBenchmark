#include <string>
#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <redisclient/redisasyncclient.h>

using namespace redisclient;
using namespace std;
using namespace std::chrono;
namespace po = boost::program_options;
class Config
{
public:
  static std::string pubTopic;
  static std::string subTopic;
    static std::string redisIP;
    static uint64_t redisPort;
};

std::string Config::pubTopic;
std::string Config::subTopic;
std::string Config::redisIP;
uint64_t Config::redisPort;


class Client
{
public:
    Client(boost::asio::io_service &ioService,
           const boost::asio::ip::address &address,
           unsigned short port)
        : ioService(ioService), publishTimer(ioService),
          connectSubscriberTimer(ioService), connectPublisherTimer(ioService),
          address(address), port(port),
          publisher(ioService), subscriber(ioService)
    {
        publisher.installErrorHandler(std::bind(&Client::connectPublisher, this));
        subscriber.installErrorHandler(std::bind(&Client::connectSubscriber, this));
    }

    void publish(const std::string &str)
    {
        publisher.publish(Config::pubTopic, str);
    }

    void start()
    {
        connectPublisher();
        connectSubscriber();
        cout<<"Echoer Initialized"<<endl;
    }

protected:
    void errorPubProxy(const std::string &)
    {
        publishTimer.cancel();
        connectPublisher();
    }

    void errorSubProxy(const std::string &)
    {
        connectSubscriber();
    }

    void connectPublisher()
    {
        //std::cerr << "connectPublisher\n";

        if( publisher.state() == RedisAsyncClient::State::Connected )
        {
            std::cerr << "disconnectPublisher\n";

            publisher.disconnect();
            publishTimer.cancel();
        }

        boost::asio::ip::tcp::endpoint endpoint(address, port);
        publisher.connect(endpoint,
                          std::bind(&Client::onPublisherConnected, this, std::placeholders::_1));
    }

    void connectSubscriber()
    {
        //std::cerr << "connectSubscriber\n";

        if( subscriber.state() == RedisAsyncClient::State::Connected ||
                subscriber.state() == RedisAsyncClient::State::Subscribed )
        {
            std::cerr << "disconnectSubscriber\n";
            subscriber.disconnect();
        }

        boost::asio::ip::tcp::endpoint endpoint(address, port);
        subscriber.connect(endpoint,
                           std::bind(&Client::onSubscriberConnected, this, std::placeholders::_1));
    }

    void callLater(boost::asio::deadline_timer &timer,
                   void(Client::*callback)())
    {
        std::cerr << "callLater\n";
        timer.expires_from_now(boost::posix_time::seconds(1000));
        timer.async_wait([callback, this](const boost::system::error_code &ec) {
            if( !ec )
            {
                (this->*callback)();
            }
        });
    }

    void onPublishTimeout()
    {
        static size_t counter = 0;
        std::string msg = str(boost::format("message %1%")  % counter++);

        if( publisher.state() == RedisAsyncClient::State::Connected )
        {
            std::cerr << "pub " << msg << "\n";
            publish(msg);
        }

        callLater(publishTimer, &Client::onPublishTimeout);
    }

    void onPublisherConnected(boost::system::error_code ec)
    {
         if(ec){}
        // {
        //     std::cerr << "onPublisherConnected: can't connect to redis: " << ec.message() << "\n";
        //     callLater(connectPublisherTimer, &Client::connectPublisher);
        // }
        // else
        // {
        //     std::cerr << "onPublisherConnected ok\n";

        //     callLater(publishTimer, &Client::onPublishTimeout);
        // }
    }

    void onSubscriberConnected(boost::system::error_code ec)
    {
        if( ec )
        {
            std::cerr << "onSubscriberConnected: can't connect to redis: " << ec.message() << "\n";
            callLater(connectSubscriberTimer, &Client::connectSubscriber);
        }
        else
        {
            //std::cerr << "onSubscriberConnected ok\n";
            subscriber.subscribe(Config::subTopic,
                                 std::bind(&Client::onMessage, this, std::placeholders::_1));
        }
    }

    void onMessage(const std::vector<char> &buf)
    {
        std::string s(buf.begin(), buf.end());
        if (s.find(std::string("Dummy")) != string::npos) {
            //cout<<"Dummy #" << counter <<endl;
            return;//Avoid pinging back dummy publishers
        }
        //cout << "pong #" << this->counter <<endl;
        publish(s);
        counter ++;
    }

private:
    boost::asio::io_service &ioService;
    boost::asio::deadline_timer publishTimer;
    boost::asio::deadline_timer connectSubscriberTimer;
    boost::asio::deadline_timer connectPublisherTimer;
    const boost::asio::ip::address address;
    const unsigned short port;

    RedisAsyncClient publisher;
    RedisAsyncClient subscriber;
    int counter = 0;
};

int main(int argc, char ** argv)
{
    po::options_description description("Options");
  description.add_options()
  ("help", "produce help message")
  ("redisIP", po::value(&Config::redisIP)->default_value("127.0.0.1"), "Redis server IP")
  ("redisPort", po::value(&Config::redisPort)->default_value(6379), "Redis server port")
  ("PubTopic", po::value(&Config::pubTopic)->default_value("pong"), "Publish to Topic (Pong)")
  ("SubTopic", po::value(&Config::subTopic)->default_value("ping"), "Subscribe to Topic (ping) ");

  po::variables_map vm;

  try
  {
    po::store(po::parse_command_line(argc, argv, description), vm);
    po::notify(vm);
  }
  catch (const po::error &e)
  {
    std::cerr << e.what() << "\n";
    return EXIT_FAILURE;
  }

  if (vm.count("help"))
  {
    std::cout << description << "\n";
    return EXIT_SUCCESS;
  }

    boost::asio::ip::address address = boost::asio::ip::address::from_string(Config::redisIP);
    const unsigned short port = Config::redisPort;

    boost::asio::io_service ioService;

    Client client(ioService, address, port);

    client.start();
    ioService.run();

    std::cerr << "done\n";

    return 0;
}