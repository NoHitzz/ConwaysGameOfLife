// 
// main.cpp
// ConwaysGameOfLife
// 
// Noah Hitz 2025
// 

#include "conwayApp.h"

int main (int argc, char *argv[]) {
    int size = 100;
    if(argc >= 2) {
        size = std::stoi(std::string(argv[1]));
    }

    ConwayApp app = ConwayApp(size);
    app.run(); 

    return 0;
}
