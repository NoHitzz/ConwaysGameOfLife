// 
// main.cpp
// chip8-emulator
// 
// Noah Hitz 2025
// 

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <time.h>
#include <stdlib.h>
#include <unordered_map>

#include "Texture.h"
#include "Timer.h"

const std::string programName = "Chip8-Emulator";

int screenWidth = 640;
int screenHeight= 480;

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

TTF_Font* debugFont = nullptr;
struct DebugRect {
    SDL_Color color;
    float xOffset;
};


std::unordered_map<std::string, DebugRect> debugRects{}; 


void error(std::string msg, std::string detail = "") {
    std::cerr << "[" << programName << "] " << msg; 
    if(!detail.empty())
        std::cerr << ": " << detail;
    std::cerr << "\n";
}

double hue2rgb(double p, double q, double t) {
  if (t < 0) 
    t += 1;
  if (t > 1) 
    t -= 1;
  if (t < 1./6) 
    return p + (q - p) * 6 * t;
  if (t < 1./2) 
    return q;
  if (t < 2./3)   
    return p + (q - p) * (2./3 - t) * 6;
    
  return p;
}

SDL_Color hslToRgb(double h, double s, double l) {
    if(s == 0)
        return SDL_Color{(Uint8) (l*255), (Uint8) (l*255), (Uint8) (l*255)};

    float q = (l < 0.5 ? l * (1 + s) : l + s - l * s);
    float p = 2 * l - q;
    float r = hue2rgb(p, q, h + 1./3) * 255;
    float g = hue2rgb(p, q, h) * 255;
    float b = hue2rgb(p, q, h - 1./3) * 255;
    return SDL_Color{(Uint8) r, (Uint8) g, (Uint8) b};
}

bool init() {
    srand(time(nullptr));

    if(!SDL_Init(SDL_INIT_VIDEO)) {
        error("SDL initialization error", SDL_GetError());
        return false;
    }

    SDL_CreateWindowAndRenderer("Chip8 Emulator", screenWidth, screenHeight, SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE, &window, &renderer);
    if(window == nullptr) {
        error("SDL window creation failed", SDL_GetError());
        return false;
    }

    if(renderer == nullptr) {
        error("SDL renderer creation failed", SDL_GetError());
        return false;
    }

    if(SDL_SetRenderVSync(renderer, 1) == false) {
        error("SDL activating VSYNC failed", SDL_GetError());
    }

    return true;
}

void close() {
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);
    SDL_Quit();
}

SDL_Texture* createText(std::string text, TTF_Font* font, SDL_Color color) {
    SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
    if(textSurface == nullptr) 
        error("TTF_RenderText_Blended failed", SDL_GetError());
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    if(textTexture == nullptr) 
        error("SDL_CreateTextureFromSurface failed", SDL_GetError());

    SDL_DestroySurface(textSurface);
    return textTexture;
}

SDL_Texture* loadImg(std::string path) {
    SDL_Texture* img = IMG_LoadTexture(renderer, path.c_str());
    if(img == nullptr) 
        error("IMG_LoadTexture failed", SDL_GetError());

    return img;
}

void renderDebugRect(std::string name, int x, int y, int w, int h) {
    if(!debugRects.contains(name)) {
        SDL_Color color = hslToRgb((rand()%255)/255.0, 0.8, 0.8);
        float offset = rand()%100; 
        debugRects.insert(std::pair<std::string, DebugRect>(name, {color, offset}));
    }

    DebugRect r = debugRects.find(name)->second;
    SDL_Color& c = r.color;
    Texture text = Texture(renderer);
    text.loadText(name, debugFont, c);
    SDL_FRect renderQuad = {(float) x, (float) y, (float) w, (float) h};
    SDL_FRect textQuad = {(float) x + (w - text.getWidth())/100.0f*r.xOffset, (float) y, (float) text.getWidth(), (float) text.getHeight()};
    SDL_SetRenderDrawColor(renderer, c.r, c.g, c.b, 100);
    SDL_RenderRect(renderer, &renderQuad);
    SDL_SetRenderDrawColor(renderer, 25, 25, 25, 225);
    SDL_RenderRect(renderer, &textQuad);
    text.render((float) x + (w - text.getWidth())/100.0f*r.xOffset, (float) y);
}

int main (int argc, char *argv[]) {
    if(!init())   
        return 1;

    TTF_Init();
    debugFont = TTF_OpenFont("resources/RobotoMono-Regular.ttf", 14);

    Texture dafoe(renderer);
    dafoe.loadImg("resources/dafoe.jpg");

    // EVENT LOOP
    bool quit = false;
    SDL_Event event;
    SDL_zero(event);

    std::string fpsText = "Fps:";
    int fpsFontSize = 16;
    TTF_Font* fontSans = TTF_OpenFont("resources/OpenSans-Regular.ttf", fpsFontSize);
    TTF_Font* fontMono = TTF_OpenFont("resources/RobotoMono-Regular.ttf", fpsFontSize);
    if(fontSans == nullptr || fontMono == nullptr) {
        error("SDL Font creation failed", SDL_GetError());
        return(1);
    }

    SDL_Color renderColor = {0,0,0};

    Timer timer = Timer();

    SDL_FRect dafoeclip = {0.0, 0.0, (float)dafoe.getWidth(), (float)dafoe.getHeight()};

    int color = 0;
    while(!quit) {
        float fps = 1000.0/timer.getMs();
        timer.start();
        
        SDL_GetRenderOutputSize(renderer, &screenWidth, &screenHeight);

        std::stringstream fpsStream;
        fpsStream << std::fixed << std::setprecision(2) << fps;
        std::string fpsStr = fpsStream.str();
        int fpsPadding = std::max((int) (5-fpsStr.find(".")), 1);

        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_EVENT_QUIT)
                quit = true;
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                switch(event.key.key) {
                    case SDLK_UP:
                        dafoeclip = {0.0, 0.0, (float)dafoe.getWidth(), (float)dafoe.getHeight()/2.0f};
                        break;

                    case SDLK_DOWN:
                        dafoeclip = {0.0, dafoe.getHeight()/2.0f, (float)dafoe.getWidth(), (float)dafoe.getHeight()/2.0f};
                        break;

                    case SDLK_LEFT:
                        dafoeclip = {0.0, 0.0, dafoe.getWidth()/2.0f, (float)dafoe.getHeight()};
                        break;

                    case SDLK_RIGHT:
                        dafoeclip = {dafoe.getWidth()/2.0f, 0.0, (float)dafoe.getWidth()/2.0f, (float)dafoe.getHeight()};
                        break;

                    default:
                        dafoeclip = {0.0, 0.0, (float)dafoe.getWidth(), (float)dafoe.getHeight()};
                        break;
                }
            } else if (event.type == SDL_EVENT_KEY_UP) {
                switch(event.key.key) {
                    default:
                        break;
                }
            }
        }

        
        Texture fpsTex = Texture(renderer);
        fpsTex.loadText(fpsText + std::string(fpsPadding, ' ') + fpsStr, fontMono, {200,50,50});
    
        SDL_SetRenderDrawColor(renderer, renderColor.r, renderColor.g, renderColor.b, 255);
        SDL_RenderClear(renderer);
        
        float imgRatio = (float) dafoe.getHeight() / dafoe.getWidth();
        float imgOffset = 20.0;
        float imgWidth = (screenWidth-imgOffset*2)/4;
        float imgHeight = imgWidth * imgRatio;  
        float imgScale = (float)imgWidth/dafoe.getWidth();

        SDL_FPoint mid = {imgWidth/2.0f-dafoeclip.x*imgScale, imgHeight/2.0f-dafoeclip.y*imgScale};
        dafoe.setRotation(color%360, &mid);

        fpsTex.render(10.0, 10.0);
        dafoe.render((screenWidth-imgWidth)/2 + dafoeclip.x*imgScale,
                (screenHeight-imgHeight)/2 + dafoeclip.y*imgScale, 
                dafoeclip.w*imgScale, dafoeclip.h*imgScale,
                &dafoeclip);

        renderDebugRect("sh", 0, (screenHeight-imgHeight)/2, 
                (float)screenWidth, imgHeight);
        renderDebugRect("sw", 0, 0, 
                (float)screenWidth, (screenHeight-imgHeight)/2+ dafoeclip.x);
        renderDebugRect("sw/2", 0, 0, 
                (float)screenWidth, (screenHeight-imgHeight)/2);
        renderDebugRect("sw/2+", 0, 0, 
                (float)screenWidth, 
                (screenHeight-imgHeight)/2+ dafoeclip.x*imgScale);
        
        renderDebugRect("mid", (screenWidth)/2.0, 20, screenWidth/2.0-40, screenHeight-40); 
        renderDebugRect("mid", 20, screenHeight/2.0, screenWidth-40, screenHeight/2.0-40); 
        
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        for(int i = 0; i < 10*10; i++) {
            SDL_RenderPoint(renderer, screenWidth/2-5+(i%10), screenHeight/2-5+(i/10));
        }

        color++;
        
        SDL_RenderPresent(renderer);
        timer.stop();
    }

    TTF_CloseFont(fontSans);
    TTF_CloseFont(debugFont);
    TTF_CloseFont(fontMono);
    TTF_Quit();
    close();
        
    return 0;
}
