# RedisBenchmark
TBC
## Getting Started
These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

### Prerequisites

OpenDDS
Boost 1.69
Python3

### Installing



## Running the tests

python3 runFullScaleTest.py \n
In order to allow remote access to redis-server you should add to /etc/redis/redis.conf "bind 0.0.0.0" after "bind 127.0.0.1", and than restart the server via "sudo service redis-server restart"


## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

