# RedisBenchmark
Redis ping-pong latency tester

## Getting Started
These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites
#### Redis
apt-get install redis-server  
sudo systemctl enable redis-server.service  
check proper installation via - "redis-cli ping"  

#### Json Parser  
git clone https://github.com/nlohmann/json 
    cd json 
    cmake . 
    make 
    make install

#### Boost  
   wget -O boost_1_69_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.69.0/boost_1_69_0.tar.gz/download  
   tar xzvf boost_1_69_0.tar.gz  
   cd boost_1_69_0  
   ./bootstrap.sh  
   ./b2  

#### Python3  
apt install python3


### Installing
git clone https://github.com/schlesg/RedisBenchmark.git  
mkdir build  
cd build  
cmake ..  
make  


## Running the tests

* Can run the Initiator and Echoer directly:
  - ./examples/Initiator --msgLength 1000  
  - ./examples/Echoer  
* More complex test are defined within the py files (e.g. IPC-1-5Test.py).
In order to allow remote access to redis-server you should add to /etc/redis/redis.conf "bind 0.0.0.0" after "bind 127.0.0.1", and than restart the server via "sudo service redis-server restart"


## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

