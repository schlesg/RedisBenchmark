#include <string>
#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <string>
#include <redisclient/redisasyncclient.h>
#include <thread>

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
    static std::string publisherUniqueName;
    static uint64_t updateRate;
    static uint64_t payloadSize;
    static size_t roundtripCount;
      static uint64_t subCount;
      

};
std::string Config::pubTopic;
std::string Config::subTopic;
std::string Config::redisIP;
uint64_t Config::redisPort;
std::string Config::publisherUniqueName;
uint64_t Config::updateRate;
uint64_t Config::payloadSize;
size_t Config::roundtripCount;
uint64_t Config::subCount;


//static const std::string channelName = "unique-redis-channel-name-example";
//static const boost::posix_time::seconds timeout(1);

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
        publisher.publish(Config::pubTopic, str); //TBD
    }

    void start()
    {
        msg = str(boost::format("%1%") % Config::publisherUniqueName);
        msg.append(Config::payloadSize, '*');

        connectSubscriber();
        connectPublisher();
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

        if (publisher.state() == RedisAsyncClient::State::Connected)
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

        if (subscriber.state() == RedisAsyncClient::State::Connected ||
            subscriber.state() == RedisAsyncClient::State::Subscribed)
        {
            std::cerr << "disconnectSubscriber\n";
            subscriber.disconnect();
        }

        boost::asio::ip::tcp::endpoint endpoint(address, port);
        subscriber.connect(endpoint,
                           std::bind(&Client::onSubscriberConnected, this, std::placeholders::_1));
    }

    void callLater(boost::asio::deadline_timer &timer,
                   void (Client::*callback)())
    {
        
        if (Config::updateRate == 0){
            timer.expires_from_now(boost::posix_time::seconds(0));
        }
        else{
            timer.expires_from_now(boost::posix_time::millisec(1000 / Config::updateRate));
        }
        
        timer.async_wait([callback, this](const boost::system::error_code &ec) {
            if (!ec)
            {
                (this->*callback)();
            }
        });
    }

    void onPublishTimeout()
    {
        //static size_t counter = 0;
        //std::string msg = str(boost::format("%1%") % Config::publisherUniqueName);
            //std::cout << "pub ";

        if (publisher.state() == RedisAsyncClient::State::Connected)
        {
            //std::cerr << "pub " << msg << "\n";
            publish(msg);
        }

        callLater(publishTimer, &Client::onPublishTimeout);
    }

    void onPublisherConnected(boost::system::error_code ec)
    {
        if (ec)
        {
            std::cerr << "onPublisherConnected: can't connect to redis: " << ec.message() << "\n";
            callLater(connectPublisherTimer, &Client::connectPublisher);
        }
        else
        {
            //std::cerr << "onPublisherConnected ok\n";
            if (Config::publisherUniqueName.compare("Dummy") == 0){ //case dummy, publish all the time
                cout << "Starting dummy..."<<endl;;
                callLater(publishTimer, &Client::onPublishTimeout);
            }
            else{
                std::this_thread::sleep_for(std::chrono::seconds(1));
                cout << "Starting test..."<<endl;;
                executionStartTime = high_resolution_clock::now(); // begin test
                publish(msg);
            }
        }
    }

    void onSubscriberConnected(boost::system::error_code ec)
    {
        if (ec)
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
        if (Config::updateRate == 0 && Config::publisherUniqueName.compare("Dummy") != 0) // ping back only in case received message is not general load related and in case not from 'Dummy'
        {
            counter++;
            sub_count ++;
            if (sub_count % Config::subCount == 0){ //meaning we received messages from all subscribers
                //cout<<sub_count<<endl;
                publish(s);
            }
        }
        
        if (counter > Config::roundtripCount && Config::updateRate == 0) //case ending test    
        {
            std::chrono::time_point<std::chrono::high_resolution_clock> executionEndTime = high_resolution_clock::now();
            auto diff = std::chrono::duration_cast<std::chrono::microseconds>(executionEndTime - executionStartTime);
            cout << "Average one-way latency = " << diff.count() / Config::roundtripCount / 2 << " microseconds with roundtrip count of " << Config::roundtripCount << endl;

            publishTimer.cancel();
            publisher.disconnect();
            subscriber.disconnect();
            this->ioService.stop();
        }
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

    size_t counter = 1;
    size_t sub_count = 0;

    std::string msg;
    std::chrono::time_point<std::chrono::high_resolution_clock> executionStartTime;
};

int main(int argc, char **argv)
{
    po::options_description description("Options");
    description.add_options()("help", "produce help message. Execution example - ")("redisIP", po::value(&Config::redisIP)->default_value("127.0.0.1"), "Redis server IP")("redisPort", po::value(&Config::redisPort)->default_value(6379), "Redis server port")("msgLength", po::value(&Config::payloadSize)->default_value(100), "Message Length (bytes)")("roundtripCount", po::value(&Config::roundtripCount)->default_value(1000), "ping-pong intervals")("pubName", po::value(&Config::publisherUniqueName)->default_value("Initiator"), "publisher name. Name 'Dummy' will cause the echoer to not echo messages")("updateRate", po::value(&Config::updateRate)->default_value(0), "update rate (for general load purposes only)")("subCount", po::value(&Config::subCount)->default_value(1), "number of subscribers to wait for a ping")("PubTopic", po::value(&Config::pubTopic)->default_value("ping"), "Publish to Topic (Ping)")
  ("SubTopic", po::value(&Config::subTopic)->default_value("pong"), "Subscribe to Topic (Pong) ");

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

    boost::asio::io_service ioService(1);

    Client client(ioService, address, port);

    client.start();
    ioService.run();

    std::cerr << "done\n";

    return 0;
}