// 
// main.cpp
// chip8-emulator
// 
// Noah Hitz 2025
// 

#ifndef MAIN_CPP
#define MAIN_CPP

#include <SDL3/SDL.h>
#include <string>
#include <iostream>

class SDLApp {
    private:
        std::string programName;

        SDL_Window* window = nullptr;
        SDL_Surface* screenSurface = nullptr;
        SDL_Renderer* renderer = nullptr;

    public:
        SDLApp(std::string& name, int initWidth, int initHeight) {
            if(!SDL_Init(SDL_INIT_VIDEO)) 
                error("SDL initialization error", SDL_GetError());

            SDL_CreateWindowAndRenderer(programName.c_str(), 
                    initWidth, initHeight, 
                    SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE, 
                    &window, &renderer);
            if(window == nullptr || renderer == nullptr)
                error("SDL window/renderer creation failed", SDL_GetError());
        }

        ~SDLApp() {
            SDL_DestroyWindow(window);
            SDL_DestroyRenderer(renderer);
        }

    private:
        void error(std::string msg, std::string detail = "") {
            std::cerr << "[" << programName << "] " << msg; 
            if(!detail.empty())
                std::cerr << ": " << detail;
            std::cerr << "\n";
        }
};


#endif /* MAIN_CPP */
