// 
// conwayApp.h
// ConwaysGameOfLife
// 
// Noah Hitz 2025
// 

#ifndef CONWAYAPP_H
#define CONWAYAPP_H

#include <SDL3_ttf/SDL_ttf.h>
#include <iterator>
#include <string>

#include "sdl3app.h"

class ConwayApp : public SDLApp {
    private: 
        const int minWindowSize = 256;

        const int gameSize = 500;
        const int cellCount;
        int minOffset = 20;
        int pointSize; 
        int offsetX;
        int offsetY;
        
        const uint8_t cellMaskAlive = 0x01;
        const uint8_t cellMaskCount = 0x1E;
        
        const Uint32 cellColorAlive = 0xFFFFFFFF;
        const Uint32 cellColorDead = 0x00000000;

        uint8_t* cells;
        uint8_t* swap;

        TTF_Font* fontSans = nullptr; 
        bool withTextRendering;
        int textCutoff = 200;
        int fontSize;
        
        Texture numbers;
        int numWidth;
        int numHeight;

        int mousePosX = 0;
        int mousePosY = 0;
        int lastMouseCellX = -1;
        int lastMouseCellY = -1;
        bool mouseLeftDown = false;

        bool paused = true;
        bool drawMode = false;
        int focusCellX = -1;
        int focusCellY = -1;
        int advance = 0;

        SDL_Surface* gameSurface;
        Texture gameTexture;
        Texture helpTexture;
        int helpTextOffset = 8;

        std::string helpText = 
            "r: reset, c: clear, d: draw, space: pause/continue, right arrow: step";

    public:
        ConwayApp(int size) 
            : SDLApp("Conway's Game of Life", 640,  480), 
            gameSize(size), cellCount(gameSize*gameSize) { 
            withTextRendering = (gameSize < textCutoff);
            helpTexture.setRenderer(renderer);
            helpTexture.loadText(helpText, monoFont, {200, 200, 200});
            minOffset += helpTexture.getHeight();
            int gameWindowSize = (int)((gameSize+2*minOffset)/windowScreenRatio);
            int minHelpTextSize = (int)((2*helpTextOffset + helpTexture.getWidth())/windowScreenRatio);
            SDL_SetWindowMinimumSize(window, 
                    std::max( std::max(gameWindowSize, minWindowSize), minHelpTextSize ), 
                    std::max(gameWindowSize, minWindowSize));

            cells = new uint8_t[cellCount];
            swap = new uint8_t[cellCount];

            gameSurface = SDL_CreateSurface(gameSize, gameSize, SDL_PIXELFORMAT_XRGB8888);
            if(gameSurface == nullptr)
                std::cerr << SDL_GetError() << "\n";
            
            gameTexture.setRenderer(renderer);
            gameTexture.loadBlank(gameSize, gameSize, 
                    SDL_TEXTUREACCESS_STREAMING, SDL_PIXELFORMAT_RGBA8888);
            SDL_SetTextureScaleMode(gameTexture.getTexture(), SDL_SCALEMODE_NEAREST);
            
            initGolRandom();

            // Test pattern: honeycomb
            // for(int i = 0; i < cellCount; i++) { cells[i] = 0x00; }
            // int x = gameSize/2.f;
            // int y = gameSize/2.f;
            // cells[x + gameSize*y] = 0x01;
            // cells[x+1 + gameSize*(y+1)] = 0x01;
            // cells[x+2 + gameSize*(y+1)] = 0x01;
            // cells[x+1 + gameSize*(y+2)] = 0x01;
            
            windowResized();
        }

        ~ConwayApp() { 
            TTF_CloseFont(fontSans);
            SDL_DestroySurface(gameSurface);
            delete[] cells;
            delete[] swap;
        }

        void windowResized() {
            SDL_GetRenderOutputSize(renderer, &screenWidth, &screenHeight);
            int size = std::min(screenHeight, screenWidth);
            pointSize = (size-2*minOffset)/gameSize; 
            
            fontSize = pointSize;
            offsetX = std::max((screenWidth-gameSize*pointSize)/2.f, 0.f);
            offsetY = std::max((screenHeight-gameSize*pointSize)/2.f, 0.f);

            if(!withTextRendering)
                return;

            if(fontSans != nullptr) {
                TTF_CloseFont(fontSans);
                fontSans = nullptr;
            }
            
            fontSans = TTF_OpenFont((getBasePath() + "../resources/OpenSans-Regular.ttf").c_str(), fontSize);
            if(fontSans == nullptr) 
                error("SDL Font creation failed", SDL_GetError());

            numWidth = 0;
            numHeight = 0;
            Texture* nums = new Texture[9]();
            for(int i = 0; i < 9; i++) {
                    nums[i].setRenderer(renderer);
                    nums[i].loadText(std::to_string(i), fontSans, {0,0,255});
                    numWidth = std::max(numWidth, nums[i].getWidth());
                    numHeight = std::max(numHeight, nums[i].getHeight());
            }
            
            numbers.setRenderer(renderer);
            numbers.loadBlank(numWidth*3, numHeight*3, SDL_TEXTUREACCESS_TARGET, nums->getFormat());
            numbers.setAsRenderTarget();
            for(int i = 0; i < 9; i++) {
                int x = (numWidth-nums[i].getWidth())/2;
                int y = (numHeight-nums[i].getHeight())/2;
                SDL_FRect dest = {(float)((i%3)*numWidth + x), (float)((int)(i/3)*numHeight + y), 
                    (float)nums[i].getWidth(), (float)nums[i].getHeight()};
                nums[i].render(dest.x, dest.y);
            }
            SDL_SetRenderTarget(renderer, nullptr);
            delete[] nums;
        }

        void initGolRandom() {
            for(int i = 0; i < cellCount; i++) { cells[i] = (rand()%3 < 1); }
        }
        
        void initClear() {
            for(int i = 0; i < cellCount; i++) { cells[i] = 0x00; }
        }
        
        // Any live cell with fewer than two live neighbours dies, as if by underpopulation.
        // Any live cell with two or three live neighbours lives on to the next generation.
        // Any live cell with more than three live neighbours dies, as if by overpopulation.
        // Any dead cell with exactly three live neighbours becomes a live cell, as if by reproduction.
        void calculateCount(int x, int y) {
            int n = (y-1 < 0 ? gameSize-1 : y-1);
            int e = (x+1 >= gameSize ? 0 : x+1);
            int s = (y+1 >= gameSize ? 0 : y+1);
            int w = (x-1 < 0 ? gameSize-1 : x-1);

            int aliveNeighb = 0;
            aliveNeighb += cells[w + n * gameSize] & cellMaskAlive;
            aliveNeighb += cells[x + n * gameSize] & cellMaskAlive;
            aliveNeighb += cells[e + n * gameSize] & cellMaskAlive;
            aliveNeighb += cells[w + y * gameSize] & cellMaskAlive;
            aliveNeighb += cells[e + y * gameSize] & cellMaskAlive;
            aliveNeighb += cells[w + s * gameSize] & cellMaskAlive;
            aliveNeighb += cells[x + s * gameSize] & cellMaskAlive;
            aliveNeighb += cells[e + s * gameSize] & cellMaskAlive;

            cells[x + y * gameSize] = (aliveNeighb << 1) | (cells[x + y * gameSize] & cellMaskAlive);
        }

        void calculateCountAll() {
            for(int y = 0; y < gameSize; y++) {
                for(int x = 0; x < gameSize; x++) {
                    int n = y-1 < 0 ? gameSize-1 : y-1;
                    int e = x+1 >= gameSize ? 0 : x+1;
                    int s = y+1 >= gameSize ? 0 : y+1;
                    int w = x-1 < 0 ? gameSize-1 : x-1;

                    int aliveNeighb = 0;
                    aliveNeighb += cells[w + n * gameSize] & cellMaskAlive;
                    aliveNeighb += cells[x + n * gameSize] & cellMaskAlive;
                    aliveNeighb += cells[e + n * gameSize] & cellMaskAlive;
                    aliveNeighb += cells[w + y * gameSize] & cellMaskAlive;
                    aliveNeighb += cells[e + y * gameSize] & cellMaskAlive;
                    aliveNeighb += cells[w + s * gameSize] & cellMaskAlive;
                    aliveNeighb += cells[x + s * gameSize] & cellMaskAlive;
                    aliveNeighb += cells[e + s * gameSize] & cellMaskAlive;

                    cells[x + y * gameSize] = (aliveNeighb << 1) | (cells[x + y * gameSize] & cellMaskAlive);
                }
            }
        }

        void updateCellState(int x, int y) {
            int idx = x + y * gameSize;

            bool alive = cells[idx] & cellMaskAlive;
            int aliveNeighb = (cells[idx] & cellMaskCount) >> 1;
            if(alive && aliveNeighb >= 2 && aliveNeighb <= 3) {
                swap[idx] = (cells[idx] & cellMaskCount) | cellMaskAlive;
            } else if(!alive && aliveNeighb == 3) {
                swap[idx] = (cells[idx] & cellMaskAlive) | cellMaskAlive;
            } else {
                swap[idx] &= (cells[idx] & cellMaskCount);
            }
        }

        void update() {
            for(int y = 0; y < gameSize; y++) {
                for(int x = 0; x < gameSize; x++) {
                    if(!paused || advance > 0) 
                        updateCellState(x, y);

                    renderCellToTexture(x,y);
                }
            }
        }

        void renderCellText() {
            for(int y = 0; y < gameSize; y++) {
                for(int x = 0; x < gameSize; x++) {
                    renderCellTextToTexture(x,y);
                }
            }
        }

        void renderCellToTexture(int x, int y) {
            bool alive = (cells[x + y * gameSize] & cellMaskAlive);
            Uint32* pixels = (Uint32*) (gameSurface->pixels);
            pixels[x + y * (gameSurface->pitch/4)] = (alive ?  cellColorAlive : cellColorDead);
        }

        void renderCellTextToTexture(int x, int y) {
            int count = (cellMaskCount & cells[x + y*gameSize]) >> 1;
            SDL_FRect point = {
                (float)(offsetX + x * pointSize), 
                (float)(offsetY + y * pointSize), 
                (float)pointSize, (float)pointSize};
            SDL_FRect clip = {
                (float)((count%3)*numWidth), 
                (float)((int)(count/3)*numHeight), 
                (float)numWidth, (float)numHeight};
            SDL_FRect textRect = {
                point.x + (pointSize - numWidth)/2.f, 
                point.y + (pointSize-numHeight)/2.f, 
                (float)numWidth, (float)numHeight};
            numbers.render(textRect.x, textRect.y, &clip);
        }

        void focus() {
            int x = focusCellX % gameSize;
            int y = focusCellY % gameSize;

            int n = (y-1 < 0 ? gameSize-1 : y-1);
            int e = (x+1 >= gameSize ? 0 : x+1);
            int s = (y+1 >= gameSize ? 0 : y+1);
            int w = (x-1 < 0 ? gameSize-1 : x-1);

            const SDL_Point neighbours[] = { 
                {w, n}, {x, n}, {e, n}, 
                {w, y}, {e,y}, 
                {w, s}, {x, s}, {e, s}
            };

            SDL_FRect point = {
                (float)(offsetX + x * pointSize), 
                (float)(offsetY + y * pointSize), 
                (float)pointSize, (float)pointSize};

            for(int i = 0; i < std::size(neighbours); i++) {
                int px = neighbours[i].x;
                int py = neighbours[i].y;
                if(!(cells[px + py * gameSize] & cellMaskAlive))
                    continue;
                SDL_FRect point = {
                    (float)(offsetX + px * pointSize), 
                    (float)(offsetY + py * pointSize), 
                    (float)pointSize, (float)pointSize};
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 200);
                drawRectangle(renderer, point, 6);
            }

            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 200);
            drawRectangle(renderer, point, 10);
        }

        void render() {
            calculateCountAll();
            update(); 
            gameTexture.update(gameSurface);
            gameTexture.render(offsetX, offsetY, gameSize*pointSize, gameSize*pointSize);

            if(withTextRendering)
                renderCellText();

            if(focusCellX != -1 && focusCellY != -1)
                focus();
            
            renderDebugRect("Conway's Game of Life", offsetX, offsetY, 
                    gameSize * pointSize, gameSize * pointSize); 

            if(!paused || advance > 0) {
                uint8_t* temp = cells;
                cells = swap;
                swap = temp;
            }

            if(advance > 0)
                advance--;

            helpTexture.render(helpTextOffset, screenHeight-helpTexture.getHeight()-helpTextOffset);
        }

        void mouseDownEventHandler(SDL_Event& event) {
            mouseLeftDown = true;
            lastMouseCellX = -1;
            lastMouseCellY = -1;
            mouseInteraction();
        }
        
        void mouseUpEventHandler(SDL_Event& event) { 
            mouseLeftDown = false;
        }

        void mouseInteraction() {
            if(mousePosX < offsetX || mousePosX >= offsetX + gameSize*pointSize ||
                    mousePosY < offsetY || mousePosY >= offsetY + gameSize*pointSize) {
                focusCellX = -1;
                focusCellY = -1;
                drawMode = false;
                return;
            }

            int x = (mousePosX-offsetX)/pointSize;
            int y = (mousePosY-offsetY)/pointSize;

            if(x == lastMouseCellX && y == lastMouseCellY)
                return;
           
            if(drawMode) {
                int idx = x + y * gameSize;
                cells[idx] = (cells[idx] & cellMaskCount) | (~cells[idx] & cellMaskAlive);
            } else {
                focusCellX = x;
                focusCellY = y;
            }

            lastMouseCellX = x;
            lastMouseCellY = y;
        }

        void mouseMoveEventHandler(SDL_Event& event) {
            mousePosX = event.motion.x * windowScreenRatio;
            mousePosY = event.motion.y* windowScreenRatio;
            
            if(mouseLeftDown)
                mouseInteraction();
        }

        void keyDownEventHandler(SDL_Event& event) {
            switch(event.key.key) {
                case SDLK_SPACE:
                    drawMode = false;
                    paused = !paused;
                    focusCellX = -1;
                    focusCellY = -1;
                    break;
                
                case SDLK_R:
                    initGolRandom();
                    focusCellX = -1;
                    focusCellY = -1;
                    break;

                case SDLK_ESCAPE:
                    focusCellX = -1;
                    focusCellY = -1;
                    drawMode = false;
                    break;

                case SDLK_C:
                    initClear();
                    break;

                case SDLK_D:
                    drawMode = !drawMode;
                    paused = true;
                    focusCellX = -1;
                    focusCellY = -1;
                    lastMouseCellX = -1;
                    lastMouseCellY = -1;
                    break;

                case SDLK_UP:
                    break;

                case SDLK_DOWN:
                    break;

                case SDLK_LEFT:
                    break;

                case SDLK_RIGHT:
                    paused = true;
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

