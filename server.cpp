#include "server.h"

Server::Server(int port, int threadPoolSize) : port(port), isRunning(true) {
    initializeSocket();

    // Create a thread pool
    for (int i = 0; i < threadPoolSize; ++i) {
        threadPool.emplace_back(&Server::threadWorker, this);
    }
}

Server::~Server() {
    isRunning = false;
    condition.notify_all();

    for (auto& thread : threadPool) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    close(serverSocket);
}

void Server::initializeSocket() {
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << port << std::endl;
}

void Server::start() {
    while (isRunning) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);

        if (clientSocket < 0) {
            perror("Accept failed");
            continue;
        }

        std::unique_lock<std::mutex> lock(queueMutex);
        clientQueue.push(clientSocket);
        condition.notify_one();
    }
}

void Server::threadWorker() {
    while (isRunning) {
        int clientSocket;

        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [this]() { return !clientQueue.empty() || !isRunning; });

            if (!isRunning && clientQueue.empty()) {
                return;
            }

            clientSocket = clientQueue.front();
            clientQueue.pop();
        }

        handleClient(clientSocket);
    }
}

void Server::handleClient(int clientSocket) {
    char buffer[2048] = {0};
    read(clientSocket, buffer, sizeof(buffer) - 1);

    std::string request(buffer);
    std::string response = handleRequest(request);

    send(clientSocket, response.c_str(), response.size(), 0);
    close(clientSocket);
}

std::string Server::handleRequest(const std::string& request) {
    std::istringstream requestStream(request);
    std::string method, path, protocol;
    requestStream >> method >> path >> protocol;

    if (method == "GET") {
        if (path == "/") {
            path = "/index.html"; // Default page
        }

        std::ifstream file("static" + path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return generateHttpResponse(buffer.str(), "200 OK");
        } else {
            return generateHttpResponse("<h1>404 Not Found</h1>", "404 Not Found");
        }
    } else {
        return generateHttpResponse("<h1>405 Method Not Allowed</h1>", "405 Method Not Allowed");
    }
}

std::string Server::generateHttpResponse(const std::string& content, const std::string& status) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << content.size() << "\r\n"
             << "Connection: close\r\n\r\n"
             << content;
    return response.str();
}
