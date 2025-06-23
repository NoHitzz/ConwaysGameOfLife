// 
// sdl3app.h
// ConwaysGameOfLife
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

#include "timer.h"
#include "texture.h"

struct DebugRect {
            SDL_Color color;
            Texture* text;
            int offset;
};

inline SDL_Color hslToRgb(double h, double s, double l);

class SDLApp {
    protected:
        const std::string programName;
        const std::string basePath;
        int screenWidth, screenHeight;

        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        float windowScreenRatio = 1.0;

        SDL_Event event;
        bool quit = false;
        Timer frameTimer = Timer();
        long lastFrameTimeMs = 0;
        SDL_Color background = {30, 30, 30};
        
        int debugFontSize = 14;
        TTF_Font* debugFont = nullptr;
        TTF_Font* monoFont = nullptr;
        int monoFontSize = 20;

    private:
        TTF_Font* fpsFont = nullptr;
        const std::string fpsText = "Fps:";
        const int fpsFontSize = 16;
        Texture fpsTexture;
        std::unordered_map<std::string, DebugRect> debugRects{}; 

    public:
        SDLApp(std::string name, int initWidth, int initHeight) : programName(name), basePath(SDL_GetBasePath()) {
            if(basePath.empty())
                error("SDL failed to get app directory path", SDL_GetError());

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

            windowScreenRatio = SDL_GetWindowPixelDensity(window);

            monoFont = TTF_OpenFont((getBasePath() + "../resources/RobotoMono-Regular.ttf").c_str(), monoFontSize);
            fpsFont = TTF_OpenFont((getBasePath() + "../resources/RobotoMono-Regular.ttf").c_str(), fpsFontSize);
            debugFont = TTF_OpenFont((getBasePath() + "../resources/RobotoMono-Regular.ttf").c_str(), debugFontSize);

            if(debugFont == nullptr || monoFont == nullptr || fpsFont == nullptr) 
                error("SDL font creation failed", SDL_GetError());

            fpsTexture.setRenderer(renderer);
            fpsTexture.loadBlank(256, 256, SDL_TEXTUREACCESS_STREAMING, SDL_PIXELFORMAT_ARGB8888);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        }

        ~SDLApp() {
            for(auto r : debugRects)
                delete r.second.text;
            TTF_CloseFont(debugFont);
            TTF_CloseFont(fpsFont);
            TTF_CloseFont(monoFont);
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
                else if (event.type == SDL_EVENT_WINDOW_RESIZED)
                    windowResized();
            }
        }

        void run() {
            while(!quit) {
                frameTimer.start();
                SDL_GetRenderOutputSize(renderer, &screenWidth, &screenHeight);
                SDL_SetRenderDrawColor(renderer, background.r, background.g, background.b, 255);
                SDL_RenderClear(renderer);
                
                eventHandler();
                render();
                
                renderFps();

                SDL_RenderPresent(renderer);
                frameTimer.stop();
                lastFrameTimeMs = frameTimer.getMs();
            }
        }

        std::string getBasePath() { return basePath; }



    protected:
        virtual void render() { }
        
        virtual void keyDownEventHandler(SDL_Event& event) { }
        
        virtual void keyUpEventHandler(SDL_Event& event) { }
       
        virtual void mouseUpEventHandler(SDL_Event& event) { }
        
        virtual void mouseDownEventHandler(SDL_Event& event) { }
       
        virtual void mouseMoveEventHandler(SDL_Event& event) { }
        
        virtual void windowResized() { }

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
            fpsStr = fpsText + std::string(fpsPadding, ' ') + fpsStr;
            SDL_Surface* textSurface = TTF_RenderText_Blended(fpsFont, 
                    fpsStr.c_str(), fpsStr.length(), {200,50,50});
            SDL_FRect fclip = {0.0, 0.0, (float)textSurface->w, (float)textSurface->h};
            fpsTexture.update(textSurface);
            SDL_FRect fpsRect = {offset, offset, (float)fclip.w + textOffsetX*2, 
                (float)fclip.h + textOffsetTop + textOffsetBottom};
            SDL_SetRenderDrawColor(renderer, 25, 25, 25, 128);
            SDL_RenderFillRect(renderer, &fpsRect);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 30);
            SDL_RenderRect(renderer, &fpsRect);

            fpsTexture.render(offset + textOffsetX, offset + textOffsetTop, &fclip);
            SDL_DestroySurface(textSurface);
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

inline void drawRectangle(SDL_Renderer* renderer, SDL_FRect& rect, int thickness) {
    SDL_FPoint points[4] = {
        {rect.x+thickness/2.f, rect.y-thickness/2.f}, 
        {rect.x + rect.w-thickness/2.f, rect.y-thickness/2.f}, 
        {rect.x + rect.w - thickness/2.f, rect.y + rect.h - thickness/2.f}, 
        {rect.x - thickness/2.f, rect.y + rect.h + thickness/2.f}};
    SDL_FRect r;
    r = {points[0].x, points[0].y, rect.w-thickness, (float)thickness};
    SDL_RenderFillRect(renderer, &r);
    r = {points[1].x, points[1].y, (float)thickness, rect.h + thickness};
    SDL_RenderFillRect(renderer, &r);
    r = {points[2].x, points[2].y, -(rect.w-thickness), (float)thickness};
    SDL_RenderFillRect(renderer, &r);
    r = {points[3].x, points[3].y, (float)thickness, -(rect.h + thickness)};
    SDL_RenderFillRect(renderer, &r);
}



#endif /* SDLAPP_H */
