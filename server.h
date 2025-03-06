#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <netinet/in.h>
#include <fstream>
#include <sstream>
#include <string>
#include <unistd.h>

class Server {
public:
    Server(int port, int threadPoolSize);
    ~Server();
    void start();

private:
    int port;
    int serverSocket;
    std::vector<std::thread> threadPool;
    std::queue<int> clientQueue;
    std::mutex queueMutex;
    std::condition_variable condition;
    bool isRunning;

    void handleClient(int clientSocket);
    void threadWorker();
    void initializeSocket();
    std::string handleRequest(const std::string& request);
    std::string generateHttpResponse(const std::string& content, const std::string& status);
};

#endif
