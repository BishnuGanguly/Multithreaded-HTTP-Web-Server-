#include "server.h"

int main() {
    int port = 8080;             // Port to listen on
    int threadPoolSize = 4;      // Number of threads in the thread pool

    Server server(port, threadPoolSize);
    server.start();

    return 0;
}
