// 
// main.cpp
// chip8-emulator
// 
// Noah Hitz 2025
// 

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>

const std::string programName = "Chip8-Emulator";

int screenWidth = 640;
int screenHeight= 480;

float screenScale = 1.0;

void error(std::string msg, std::string detail = "") {
    std::cout << "[" << programName << "] " << msg <<  ": " << detail;
}

int main (int argc, char *argv[]) {
    SDL_Window* window = nullptr;
    SDL_Surface* screenSurface = nullptr;
    SDL_Renderer* renderer = nullptr;

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        error("SDL initialization error", SDL_GetError());
        return(1);
    }

    SDL_CreateWindowAndRenderer("Chip8 Emulator", screenWidth, screenHeight, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE, &window, &renderer);
    if(window == nullptr) {
        error("SDL window creation failed", SDL_GetError());
        return(1);
    }

    if(renderer == nullptr) {
        error("SDL renderer creation failed", SDL_GetError());
        return(1);
    }

    TTF_Init();

    // EVENT LOOP
    bool quit = false;
    SDL_Event event;
    SDL_zero(event);

    std::string text = "Hello World, you 51235 33'341";
    TTF_Font* fontSans = TTF_OpenFont("resources/OpenSans-Regular.ttf", 16);
    if(fontSans == nullptr) {
        error("SDL Font creation failed", SDL_GetError());
        return(1);
    }
    SDL_Color white = {255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Blended(fontSans, text.c_str(), text.length(), white);
    if(textSurface == nullptr) {
        error("SDL textSurface creation failed", SDL_GetError());
        return(1);
    }
    SDL_Texture* textTex = SDL_CreateTextureFromSurface(renderer, textSurface);
    if(textTex == nullptr) {
        error("SDL textTex creation failed", SDL_GetError());
        return(1);
    }
    SDL_DestroySurface(textSurface);

    SDL_FRect destRect = {0.0, 0.0, (float) textTex->w, (float) textTex->h};

    int color = 0;
    while(!quit) {
        SDL_GetRenderOutputSize(renderer, &screenWidth, &screenHeight);
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_EVENT_QUIT)
                quit = true;
        }
    
        SDL_SetRenderDrawColor(renderer, 50, 0, 255, 255);
        SDL_RenderClear(renderer);
        
        SDL_RenderTexture(renderer, textTex, nullptr, &destRect);
        
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for(int i = 0; i < 10*10; i++) {
            SDL_RenderPoint(renderer, screenWidth/2-10+(i%10), screenHeight/2-10+(i/10));
        }
        
        SDL_RenderPresent(renderer);

        // SDL_FillSurfaceRect(screenSurface, nullptr, SDL_MapSurfaceRGB(screenSurface, color%255, color%255, color%255));
        // color++; 
    
        // SDL_UpdateWindowSurface(window);
    }






    // SDL_DestroySurface(screenSurface);
    SDL_DestroyWindow(window);
    window = nullptr;
    screenSurface = nullptr;
    SDL_Quit();
    
    return 0;
}
