// 
// ConwayApp.h
// chip8-emulator
// 
// Noah Hitz 2025
// 

#ifndef CONWAYAPP_H
#define CONWAYAPP_H

#include<SDL3_ttf/SDL_ttf.h>
#include <string>

#include "sdl3app.h"

class ConwayApp : public SDLApp {
    private: 
        TTF_Font* fontSans = nullptr; 

        static const int minOffset = 100;
        const int gameSize = 500;
        const int cellCount;
        int pointSize; 
        int fontSize;
        int offsetX;
        int offsetY;

        bool* cells;
        bool* swap;
        int* count;

        Texture* numbers = nullptr;

        int mousePosX = 0;
        int mousePosY = 0;

        bool paused = true;
        int focusCellX = -1;
        int focusCellY = -1;

        int frame = 0;
        int advance = 0;

    public:
        ConwayApp(int size) : SDLApp("Conway", 640, 480), gameSize(size), cellCount(gameSize*gameSize) { 
            cells = new bool[cellCount];
            swap = new bool[cellCount];
            count = new int[cellCount];

            // for(int i = 0; i < cellCount; i++) cells[i] = (rand()%2 > 0);
            for(int i = 0; i < cellCount; i++) { cells[i] = false; count[i] = 0; }

            // This pattern should become honeycomb
            int x = gameSize/2.f;
            int y = gameSize/2.f;
            cells[x + gameSize*y] = true;
            cells[x+1 + gameSize*(y+1)] = true;
            cells[x+2 + gameSize*(y+1)] = true;
            cells[x+1 + gameSize*(y+2)] = true;

            windowResized();
        }

        ~ConwayApp() { 
            TTF_CloseFont(fontSans);
            delete[] numbers;
            delete[] cells;
            delete[] swap;
            delete[] count;
        }

        void windowResized() {
            SDL_GetRenderOutputSize(renderer, &screenWidth, &screenHeight);
            int size = (screenHeight < screenWidth ? screenHeight : screenWidth);
            pointSize = (size-2*minOffset)/gameSize; 
            fontSize = pointSize;
            offsetX = std::max((screenWidth-gameSize*pointSize)/2.f, 0.f);
            offsetY = std::max((screenHeight-gameSize*pointSize)/2.f, 0.f);

            if(fontSans != nullptr) {
                TTF_CloseFont(fontSans);
                fontSans = nullptr;
            }
            
            if(numbers != nullptr) {
                delete[] numbers;
                numbers = nullptr;
            }

            fontSans = TTF_OpenFont("resources/OpenSans-Regular.ttf", fontSize);
            if(fontSans == nullptr) 
                error("SDL Font creation failed", SDL_GetError());

            numbers = new Texture[9]();
            for(int i = 0; i < 9; i++) {
                    numbers[i].setRenderer(renderer);
                    numbers[i].loadText(std::to_string(i), fontSans, {0,0,255});
            }
        }
        
        // Any live cell with fewer than two live neighbours dies, as if by underpopulation.
        // Any live cell with two or three live neighbours lives on to the next generation.
        // Any live cell with more than three live neighbours dies, as if by overpopulation.
        // Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
        void calculateCount(int x, int y) {
            int left = (x-1 < 0 ? gameSize-1 : x-1);
            int right = (x+1 >= gameSize ? 0 : x+1);
            int up = (y-1 < 0 ? gameSize-1 : y-1);
            int down = (y+1 >= gameSize ? 0 : y+1);

            SDL_Point neighbours[] = {{left, up}, {x, up}, {right, up}, {left, y}, {right,y}, {left, down}, {x, down}, {right, down}};
            int aliveNeighb = 0;
            for(int i = 0; i < std::size(neighbours); i++) {
                aliveNeighb += cells[neighbours[i].x + neighbours[i].y * gameSize];
            }

            count[x + y * gameSize] = aliveNeighb;
        }

        void updateState(int x, int y) {
            swap[x + y * gameSize] = false;

            int aliveNeighb = count[x + y * gameSize];
            if(cells[x + y * gameSize] && aliveNeighb >= 2 && aliveNeighb <= 3)
                swap[x + y * gameSize] = true;
            else if(!cells[x + y * gameSize] && aliveNeighb == 3)
                swap[x + y * gameSize] = true;
        }

        void update() {
            bool advancing = false;
            if(paused && advance > 0) {
                advancing = true;
                advance--;
            }

            for(int y = 0; y < gameSize; y++) {
                for(int x = 0; x < gameSize; x++) {
                    calculateCount(x,y);

                    if(!paused || advancing) {
                        updateState(x,y);
                    }

                    SDL_FRect point = {(float)(offsetX + x * pointSize), (float)(offsetY + y * pointSize), (float)pointSize, (float)pointSize};
                    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                    if(cells[x + y * gameSize])
                        SDL_RenderFillRect(renderer, &point);

                    renderCellText(x,y);
                }
            }

            if(!paused || advancing) {
                bool* temp = cells;
                cells = swap;
                swap = temp;
            }
        }

        void renderCellText(int x, int y) {
            SDL_FRect point = {(float)(offsetX + x * pointSize), (float)(offsetY + y * pointSize), (float)pointSize, (float)pointSize};
            Texture* text = &numbers[count[x+y*gameSize]];
            SDL_FRect textRect = {point.x + (pointSize - text->getWidth())/2.f, point.y + (pointSize-text->getHeight())/2.f, 
                (float)text->getWidth(), (float)text->getHeight()};
            text->render(textRect.x, textRect.y);
        }

        void focus() {
            int x = focusCellX % gameSize;
            int y = focusCellY % gameSize;

            int left = (x-1 < 0 ? gameSize-1 : x-1);
            int right = (x+1 >= gameSize ? 0 : x+1);
            int up = (y-1 < 0 ? gameSize-1 : y-1);
            int down = (y+1 >= gameSize ? 0 : y+1);

            SDL_Point neighbours[] = {{left, up}, {x, up}, {right, up}, {left, y}, {right,y}, {left, down}, {x, down}, {right, down}};

            SDL_FRect point = {(float)(offsetX + x * pointSize), (float)(offsetY + y * pointSize), (float)pointSize, (float)pointSize};

            for(int i = 0; i < std::size(neighbours); i++) {
                int px = neighbours[i].x;
                int py = neighbours[i].y;
                if(!cells[px + py * gameSize])
                    continue;
                SDL_FRect point = {(float)(offsetX + px * pointSize), (float)(offsetY + py * pointSize), (float)pointSize, (float)pointSize};
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 200);
                drawRectangle(renderer, point, 6);
            }

            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 200);
            drawRectangle(renderer, point, 10);
        }

        static void drawRectangle(SDL_Renderer* renderer, SDL_FRect& rect, int thickness) {
            SDL_FPoint points[4] = {{rect.x+thickness/2.f, rect.y-thickness/2.f}, {rect.x + rect.w-thickness/2.f, rect.y-thickness/2.f}, 
                {rect.x + rect.w - thickness/2.f, rect.y + rect.h - thickness/2.f}, {rect.x - thickness/2.f, rect.y + rect.h + thickness/2.f}};
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

        void render() {
            update(); 
            if(focusCellX != -1 && focusCellY != -1)
                focus();
            
            renderDebugRect("mid", 
                    (screenWidth)/2.0, 20, screenWidth/2.0-40, screenHeight-40); 
            renderDebugRect("mid", 
                    20, screenHeight/2.0, screenWidth-40, screenHeight/2.0-40); 

            frame++;
        }

        void mouseDownEventHandler(SDL_Event& event) {
            focusCellX = (mousePosX-offsetX)/pointSize;
            focusCellY = (mousePosY-offsetY)/pointSize;
        }

        void mouseMoveEventHandler(SDL_Event& event) {
            int width, height;
            SDL_GetWindowSize(window, &width, &height);
            mousePosX = event.motion.x * screenWidth/width;
            mousePosY = event.motion.y* screenHeight/height;

        }

        void keyDownEventHandler(SDL_Event& event) {
            switch(event.key.key) {
                case SDLK_SPACE:
                    paused = !paused;
                    focusCellX = -1;
                    focusCellY = -1;
                    break;
                
                case SDLK_R:
                    for(int i = 0; i < cellCount; i++) {cells[i] = (rand()%2 > 0); }
                    focusCellX = -1;
                    focusCellY = -1;
                    break;

                case SDLK_C:
                    focusCellX = -1;
                    focusCellY = -1;
                    break;

                case SDLK_UP:
                    break;

                case SDLK_DOWN:
                    break;

                case SDLK_LEFT:
                    break;

                case SDLK_RIGHT:
                    advance++;
                    focusCellX = -1;
                    focusCellY = -1;
                    break;

                default:
                    break;
            }
        }
};



#endif /* CONWAYAPP_H */

