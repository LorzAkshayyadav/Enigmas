
# Enigmas
Buid c++ server code using this command:
```bash
g++ web.cpp -o server $(pkg-config --cflags --libs jsoncpp) -lwebsockets -pthread
if json lib is not installed.
./server
```
else 
```bash
g++ web.cpp -o server -lwebsockets -ljsoncpp -pthread
./server
``` 


# Enigmas