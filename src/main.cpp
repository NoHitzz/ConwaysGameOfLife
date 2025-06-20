// 
// main.cpp
// chip8-emulator
// 
// Noah Hitz 2025
// 

#include "testApp.h"
#include "ConwayApp.h"


int main (int argc, char *argv[]) {
    int size = 16;
    if(argc >= 2) {
        std::cout << "Size set to: " + std::string(argv[1]) << "\n";
        size = std::stoi(std::string(argv[1]));
        // if(size%8 != 0) {
            // std::cout << "Size invalid, needs to be multiple of 8" << "\n";
            // size = 16;
        // }
    }

    ConwayApp app = ConwayApp(size);
    app.run(); 

    return 0;
}
