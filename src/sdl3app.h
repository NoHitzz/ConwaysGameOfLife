// 
// sdlapp.h
// chip8-emulator
// 
// Noah Hitz 2025
// 

#ifndef SDLAPP_H
#define SDLAPP_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

#include "Timer.h"
#include "Texture.h"

struct DebugRect {
            SDL_Color color;
            Texture* text;
            int offset;
};

inline SDL_Color hslToRgb(double h, double s, double l);

class SDLApp {
    protected:
        const std::string programName;
        int screenWidth, screenHeight;

        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;

        SDL_Event event;
        bool quit = false;
        Timer timer = Timer();
        long lastFrameTimeMs = 0;
        SDL_Color background = {30, 30, 30};
        
        int debugFontSize = 14;
        TTF_Font* debugFont = nullptr;

    private:
        const std::string fpsText = "Fps:";
        const int fpsFontSize = 16;
        TTF_Font* fpsFont = nullptr;
        std::unordered_map<std::string, DebugRect> debugRects{}; 

    public:
        SDLApp(std::string name, int initWidth, int initHeight) {
            srand(time(nullptr));
            screenWidth = initWidth;
            screenHeight = initHeight;

            if(!SDL_Init(SDL_INIT_VIDEO)) 
                error("SDL initialization error", SDL_GetError());

            SDL_CreateWindowAndRenderer(programName.c_str(), 
                    screenWidth, screenHeight, 
                    SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_RESIZABLE, 
                    &window, &renderer);
            if(window == nullptr || renderer == nullptr)
                error("SDL window/renderer creation failed", SDL_GetError());

            if(SDL_SetRenderVSync(renderer, 1) == false) 
                error("SDL activating VSYNC failed", SDL_GetError());

            if(!TTF_Init()) 
                error("SDL failed to initialize TTF", SDL_GetError());
    
            SDL_zero(event);

            fpsFont = TTF_OpenFont("resources/RobotoMono-Regular.ttf", fpsFontSize);
            debugFont = TTF_OpenFont("resources/RobotoMono-Regular.ttf", debugFontSize);

            if(debugFont == nullptr || fpsFont == nullptr) 
                error("SDL font creation failed", SDL_GetError());

        }

        ~SDLApp() {
            for(auto r : debugRects)
                delete r.second.text;
            TTF_CloseFont(debugFont);
            TTF_CloseFont(fpsFont);
            SDL_DestroyWindow(window);
            SDL_DestroyRenderer(renderer);
            TTF_Quit();
            SDL_Quit();
        }

        void eventHandler() {
            while(SDL_PollEvent(&event)) {
                if(event.type == SDL_EVENT_QUIT)
                    quit = true;
                else if (event.type == SDL_EVENT_KEY_DOWN) 
                    keyDownEventHandler(event);
                else if (event.type == SDL_EVENT_KEY_UP)
                    keyUpEventHandler(event);
                else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
                    mouseDownEventHandler(event);
                else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP)
                    mouseUpEventHandler(event);
                else if (event.type == SDL_EVENT_MOUSE_MOTION)
                    mouseMoveEventHandler(event);
            }
        }

        void run() {
            while(!quit) {
                timer.start();
                SDL_GetRenderOutputSize(renderer, &screenWidth, &screenHeight);
                SDL_SetRenderDrawColor(renderer, 
                    background.r, background.g, background.b, 255);
                SDL_RenderClear(renderer);

                
                eventHandler();
                render();
                
                renderFps();

                SDL_RenderPresent(renderer);
                timer.stop();
                lastFrameTimeMs = timer.getMs();
            }
        }

    protected:
        virtual void render() { }
        
        virtual void keyDownEventHandler(SDL_Event& event) { }
        
        virtual void keyUpEventHandler(SDL_Event& event) { }
       
        virtual void mouseUpEventHandler(SDL_Event& event) { }
        
        virtual void mouseDownEventHandler(SDL_Event& event) { }
       
        virtual void mouseMoveEventHandler(SDL_Event& event) { }

        void renderDebugRect(std::string name, int x, int y, int w, int h) {
            if(!debugRects.contains(name)) {
                SDL_Color color = hslToRgb((rand()%255)/255.0, 0.9, 0.7);
            
                Texture* text = new Texture(renderer);
                text->loadText(name, debugFont, {255, 255, 255});

                debugRects.insert(std::pair<std::string, DebugRect> (name, 
                            {color, text, (rand()%100)/100}));
            }

            int xOffset = 10;
            int textOffsetX = 5;
            int textOffsetTop = 2;
            int textOffsetBottom = 4;
            DebugRect r = debugRects.find(name)->second;
            SDL_FRect renderQuad = {
                (float) x, (float) y, 
                (float) w, (float) h};
            SDL_FRect textQuad = {
                (float) x + xOffset + (w - r.text->getWidth())/100.0f*r.offset, 
                (float) y, 
                (float) r.text->getWidth() + textOffsetX*2, 
                (float) r.text->getHeight() + textOffsetTop + textOffsetBottom};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, r.color.r, r.color.g, r.color.b, 100);
            SDL_RenderRect(renderer, &renderQuad);
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, 200);
            SDL_RenderFillRect(renderer, &textQuad);
            SDL_SetRenderDrawColor(renderer, r.color.r, r.color.g, r.color.b, 100);
            SDL_RenderRect(renderer, &textQuad);
            r.text->render(textQuad.x + textOffsetX, textQuad.y + textOffsetTop);
        }

        void renderFps() {
            float fps = 1000.0/lastFrameTimeMs;

            float offset = 10;
            float textOffsetX = 8;
            float textOffsetTop = 3;
            float textOffsetBottom = 5;
            std::stringstream fpsStream;
            fpsStream << std::fixed << std::setprecision(2) << fps;
            std::string fpsStr = fpsStream.str();
            int fpsPadding = std::max((int) (5-fpsStr.find(".")), 1);
            Texture fpsTex = Texture(renderer);
            fpsTex.loadText(fpsText + std::string(fpsPadding, ' ') + fpsStr, fpsFont, {200,50,50});
            SDL_FRect fpsRect = {offset, offset, (float)fpsTex.getWidth() + textOffsetX*2, 
                (float)fpsTex.getHeight() + textOffsetTop + textOffsetBottom};
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, 128);
            SDL_RenderFillRect(renderer, &fpsRect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
            SDL_RenderRect(renderer, &fpsRect);

            fpsTex.render(offset + textOffsetX, offset + textOffsetTop);
        }

        void error(std::string msg, std::string detail = "") {
            std::cerr << "[" << programName << "] " << msg; 
            if(!detail.empty())
                std::cerr << ": " << detail;
            std::cerr << "\n";
        }
};

inline double hue2rgb(double p, double q, double t) {
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

inline SDL_Color hslToRgb(double h, double s, double l) {
    if(s == 0)
        return SDL_Color{(Uint8) (l*255), (Uint8) (l*255), (Uint8) (l*255)};

    float q = (l < 0.5 ? l * (1 + s) : l + s - l * s);
    float p = 2 * l - q;
    float r = hue2rgb(p, q, h + 1./3) * 255;
    float g = hue2rgb(p, q, h) * 255;
    float b = hue2rgb(p, q, h - 1./3) * 255;
    return SDL_Color{(Uint8) r, (Uint8) g, (Uint8) b};
}
#endif /* SDLAPP_H */
