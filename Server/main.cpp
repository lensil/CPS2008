#include "Server.h"
#include "Canvas.h"

Canvas canvas;

int main() {
    Server server(PORT);
    server.run();
    return 0;
}