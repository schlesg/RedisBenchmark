#include <string>
#include <iostream>
#include <boost/asio/ip/address.hpp>
#include <boost/format.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <thread>
#include <redisclient/redisasyncclient.h>
#include <chrono>
#include "nlohmann/json.hpp"
#include <fstream>
#include <vector>
#include "BenchmarkLogger.hpp"

using namespace redisclient;
using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

static const std::string channelName = "RedisBenchmarkTopic";

// class BenchmarkLogger
// {
// public:
//     static std::vector<std::tuple<std::string, uint64_t, uint32_t>> latencyResults; // src + seq# + latency
//     static void DumpResultsToFile()
//     {
//         stringstream ss;
//         ss << std::this_thread::get_id();
//         string PID = ss.str();
//         std::ofstream outfile("BenchmarkResults_" + PID + ".csv");
//         outfile << "src,seq#,latency (microsec)" << endl;
//         for (auto &result : latencyResults)
//         {
//             outfile << std::get<0>(result) << "," << std::get<1>(result) << "," << std::get<2>(result) << endl;
//         }
//         outfile.clear();
//     };
// };

// std::vector<std::tuple<std::string, uint64_t, uint32_t>> BenchmarkLogger::latencyResults;

class Client
{
public:
    Client(boost::asio::io_service &ioService,
           const boost::asio::ip::address &address,
           unsigned short port)
        : ioService(ioService),
          connectSubscriberTimer(ioService),
          address(address), port(port),
          subscriber(ioService)
    {
        //publisher.installErrorHandler(std::bind(&Client::connectPublisher, this));
        subscriber.installErrorHandler(std::bind(&Client::connectSubscriber, this));
    }

    void start()
    {
        connectSubscriber();
    }

protected:
    void errorSubProxy(const std::string &)
    {
        connectSubscriber();
    }

    void connectSubscriber()
    {
        std::cerr << "connectSubscriber\n";

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
        timer.expires_from_now(boost::posix_time::millisec(100));
        timer.async_wait([callback, this](const boost::system::error_code &ec) {
            if (!ec)
            {
                (this->*callback)();
            }
        });
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
            std::cerr << "onSubscriberConnected ok\n";
            subscriber.subscribe(channelName,
                                 std::bind(&Client::onMessage, this, std::placeholders::_1));
            // subscriber.psubscribe("*",
            //                      std::bind(&Client::onMessage, this, std::placeholders::_1));
        }
    }

    void onMessage(const std::vector<char> &buf)
    {
        high_resolution_clock::time_point p = high_resolution_clock::now();
        microseconds nowTime = duration_cast<microseconds>(p.time_since_epoch());
        std::string msg(buf.begin(), buf.end());
        json json_msg = json::parse(msg);
        auto srcTimestamp = std::string(json_msg["time_stamp"]);
        if (srcTimestamp.compare("0") == 0)
        {
            cout << "END OF TEST" << endl;
            BenchmarkLogger::DumpResultsToFile();
            ioService.stop();
            return;
        }
        else
        {
            auto microsecLatency = nowTime.count() - std::stoll(srcTimestamp);
            //cout << "One way latency (millisec) = " << static_cast<double>(microsecLatency) / 1000 << endl;
            BenchmarkLogger::latencyResults.emplace_back(
                json_msg["src"],
                json_msg["seq_num"],
                microsecLatency);
        }

        // std::string s(buf.begin(), buf.end());
        // std::cout << "onMessage: " << s << "\n";
    }

private:
    boost::asio::io_service &ioService;
    boost::asio::deadline_timer connectSubscriberTimer;
    const boost::asio::ip::address address;
    const unsigned short port;

    RedisAsyncClient subscriber;
};

int main(int, char **)
{
    boost::asio::ip::address address = boost::asio::ip::address::from_string("127.0.0.1");
    const unsigned short port = 6379;

    boost::asio::io_service ioService;

    Client client(ioService, address, port);

    client.start();
    ioService.run();

    std::cerr << "done\n";

    return 0;
}