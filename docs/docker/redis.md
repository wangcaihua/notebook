docker pull redis:latest

docker run -itd --name redis-test -p 6379:6379 redis

docker exec -it redis-test /bin/bash

docker pull redis:latest