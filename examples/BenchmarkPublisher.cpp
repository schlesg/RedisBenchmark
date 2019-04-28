#include <string>
#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <thread>
#include <redisclient/redisasyncclient.h>
#include "nlohmann/json.hpp"
#include <chrono>
#include <boost/program_options.hpp>
#include <regex>
#include <memory>
#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/program_options.hpp>

using namespace redisclient;
using json = nlohmann::json;
using namespace std;
using namespace std::chrono;

static const std::string channelName = "RedisBenchmarkTopic";

class Config
{
public:
    static std::string redisServerAddress;
    static unsigned short redisServerPort;
    static std::string publisherUniqueName;
    static uint64_t payloadSize;
    static int64_t executionTime;
    static double pubRate;
};

std::string Config::redisServerAddress;
unsigned short Config::redisServerPort;
std::string Config::publisherUniqueName;
uint64_t Config::payloadSize;
int64_t Config::executionTime;
double Config::pubRate;

class Client
{
public:
    Client(boost::asio::io_service &ioService,
           const boost::asio::ip::address &address,
           unsigned short port)
        : ioService(ioService), publishTimer(ioService),
          connectPublisherTimer(ioService),
          address(address), port(port),
          publisher(ioService)

    {

        publisher.installErrorHandler(std::bind(&Client::connectPublisher, this));
        msg["src"] = Config::publisherUniqueName;
        msg["seq_num"] = 0;
        msg["time_stamp"] = "0";
        std::string s(Config::payloadSize, '0');
        msg["payload"] = s.c_str();
        this->executionStartTime = high_resolution_clock::now();
    }

    void publish(const std::string &str)
    {
        publisher.publish(channelName, str);
    }

    void start()
    {
        connectPublisher();
    }

protected:
    void errorPubProxy(const std::string &)
    {
        publishTimer.cancel();
        connectPublisher();
    }

    void connectPublisher()
    {
        std::cerr << "connectPublisher\n";

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

    void callLater(boost::asio::deadline_timer &timer,
                   void (Client::*callback)())
    {
        timer.expires_from_now(boost::posix_time::millisec(u_int64_t(1000 * (1 / Config::pubRate))));
        timer.async_wait([callback, this](const boost::system::error_code &ec) {
            if (!ec)
            {
                (this->*callback)();
            }
        });
    }

    void onPublishTimeout()
    {
        static size_t seq = 0;
        seq++;
        high_resolution_clock::time_point p = high_resolution_clock::now();
        microseconds us = duration_cast<microseconds>(p.time_since_epoch());

        msg["time_stamp"] = to_string(us.count());
        msg["seq_num"] = seq;

        if (publisher.state() == RedisAsyncClient::State::Connected)
        {
            if (seq % 100 == 0)
            {
                std::cerr << "pub " << seq << endl;
            }
            auto msg_str = msg.dump();

            publish(msg_str.c_str());
        }
        //Check if passed provided executionTime
        if ((duration_cast<seconds>(high_resolution_clock::now() - executionStartTime)).count() > Config::executionTime)
        {
            msg["time_stamp"] = "0"; //Notify subscriber on ending the test
            std::cerr << "Finishing test " << endl;
            publish(msg.dump().c_str());
            //std::this_thread::sleep_for(std::chrono::seconds(3));
            //ioService.stop();
            return;
        }
        else
        {
            callLater(publishTimer, &Client::onPublishTimeout);
        }
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
            std::cerr << "onPublisherConnected ok\n";

            callLater(publishTimer, &Client::onPublishTimeout);
        }
    }

private:
    boost::asio::io_service &ioService;
    boost::asio::deadline_timer publishTimer;
    boost::asio::deadline_timer connectPublisherTimer;
    const boost::asio::ip::address address;
    const unsigned short port;
    json msg;
    std::chrono::time_point<std::chrono::high_resolution_clock> executionStartTime;
    RedisAsyncClient publisher;
};

int main(int argc, char **argv)
{
    namespace po = boost::program_options;
    po::options_description description("Options");
    description.add_options()("help", "produce help message")("address", po::value(&Config::redisServerAddress)->default_value("127.0.0.1"), "redis server ip address")("port", po::value(&Config::redisServerPort)->default_value(6379), "redis server port")("pubUniqueName", po::value(&Config::publisherUniqueName)->default_value("pub1"), "Publisher Unique Name")("msgLength", po::value(&Config::payloadSize)->default_value(100), "Message Length (bytes)")("executionTime", po::value(&Config::executionTime)->default_value(100), "Benchmark Execution Time (sec)")("pubRate", po::value(&Config::pubRate)->default_value(100), "Publishing rate");

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

    boost::asio::io_service ioService;

    Client client(ioService, boost::asio::ip::address::from_string(Config::redisServerAddress), Config::redisServerPort);

    client.start();
    ioService.run();

    std::cerr << "done\n";

    return 0;
}
