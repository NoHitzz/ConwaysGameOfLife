// 
// main.cpp
// chip8-emulator
// 
// Noah Hitz 2025
// 

// #include "testApp.h"
#include "ConwayApp.h"


int main (int argc, char *argv[]) {
    int size = 100;
    if(argc >= 2) {
        std::cout << "Size set to: " + std::string(argv[1]) << "\n";
        size = std::stoi(std::string(argv[1]));
    }

    ConwayApp app = ConwayApp(size);
    app.run(); 

    return 0;
}
