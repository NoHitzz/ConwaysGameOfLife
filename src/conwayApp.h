// 
// conwayApp.h
// ConwaysGameOfLife
// 
// Noah Hitz 2025
// 

#ifndef CONWAYAPP_H
#define CONWAYAPP_H

#include <SDL3_ttf/SDL_ttf.h>
#include <cstdint>
#include <fstream>
#include <string>

#include "sdl3app.h"

class ConwayApp : public SDLApp {
    private: 
        const int minWindowSize = 256;

        const int gameSize = 0;
        const int rowLength = 0;
        const int numRows = 0;
        const int arrayLength = 0;
        int minOffset = 20;
        int pointSize; 
        int offsetX;
        int offsetY;
        long generation = 0;
        
        const uint64_t cellMaskAlive = 0x1;
        const uint64_t cellMaskCount = 0xF;
        
        const Uint32 cellColorAlive = 0xFFFFFFFF;
        const Uint32 cellColorDead = 0x00000000;

        uint64_t* cells;
        uint64_t* swap;
        uint64_t* count;

        TTF_Font* fontSans = nullptr; 
        bool withTextRendering;
        int textCutoff = 200;
        int fontSize;
        
        Texture numbers;
        int numWidth;
        int numHeight;

        SDL_Point mousePos = {0,0};
        SDL_Point lastMouseCell = {0,0};
        SDL_Point focusCell = {-1,-1};
        bool mouseLeftDown = false;
        bool mouseCellState = false;

        std::string numberKeys = "";
        const int NUMBERKEY_UPDATE = -1;
        const int NUMBERKEY_CANCEL = -2;
        Timer numberKeyTimer {}; 
        long numberKeyTimeout = 1000;
        Texture numberKeysTexture;

        bool paused = true;
        bool drawMode = false;
        int advance = 0;

        SDL_Surface* gameSurface;
        Texture gameTexture;
        Texture generationTexture;
        Texture helpTexture;
        int helpTextOffset = 8;

        std::vector<std::string> patterns {};

        std::string helpText = 
            "r: reset, c: clear, d: draw, ctrl-v: paste pattern, space: pause/continue, right arrow: step";

    public:
        ConwayApp(uint64_t size) : SDLApp("Conway's Game of Life", 640,  480), 
        gameSize(std::max(nextPowerOfTwo(size), 16)), 
        rowLength(gameSize / 16), // Every array entry packs 16 horizontal cells
        numRows(gameSize),
        arrayLength(rowLength * numRows) { 

            withTextRendering = (gameSize < textCutoff);
            helpTexture.setRenderer(renderer);
            helpTexture.loadText(helpText, monoFont, {200, 200, 200});
            numberKeysTexture.setRenderer(renderer);
            minOffset += helpTexture.getHeight();
            int gameWindowSize = (int)((gameSize+2*minOffset)/windowScreenRatio);
            int minHelpTextSize = (int)((2*helpTextOffset + helpTexture.getWidth())/windowScreenRatio);
            SDL_SetWindowMinimumSize(window, 
                    std::max( std::max(gameWindowSize, minWindowSize), minHelpTextSize ), 
                    std::max(gameWindowSize, minWindowSize));

            cells = new uint64_t[arrayLength];
            swap = new uint64_t[arrayLength];
            count = new uint64_t[arrayLength];
            // std::cout << "requesteSize: " << size << ", gameSize: " << gameSize 
            //     << ", packedLength: (" << rowLength << ", " << numRows 
            //     << "), arrayLength: " << arrayLength << "\n";

            gameSurface = SDL_CreateSurface(gameSize, gameSize, SDL_PIXELFORMAT_XRGB8888);
            if(gameSurface == nullptr)
                std::cerr << SDL_GetError() << "\n";
            
            gameTexture.setRenderer(renderer);
            gameTexture.loadBlank(gameSize, gameSize, 
                    SDL_TEXTUREACCESS_STREAMING, SDL_PIXELFORMAT_RGBA8888);
            SDL_SetTextureScaleMode(gameTexture.getTexture(), SDL_SCALEMODE_NEAREST);
            generationTexture.setRenderer(renderer);
            generationTexture.loadBlank(256, 512, SDL_TEXTUREACCESS_STREAMING, SDL_PIXELFORMAT_ARGB8888);

            loadPatterns();
            
            initGolRandom();
                        
            windowResized();
        }

        ~ConwayApp() { 
            TTF_CloseFont(fontSans);
            SDL_DestroySurface(gameSurface);
            delete[] cells;
            delete[] swap;
            delete[] count;
        }

        void windowResized() {
            SDL_GetRenderOutputSize(renderer, &screenWidth, &screenHeight);
            int size = std::min(screenHeight, screenWidth);
            pointSize = std::max((size-2*minOffset)/gameSize, 1);  //TODO: ?? does this max make sense?
            
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

        void loadPatterns() {
            std::string line;
            std::string path = getBasePath() + "../resources/patterns.txt";
            std::ifstream patternFile(path);
            if(!patternFile.is_open()) {
                error("Failed to load pattern file", path);
                return;
            }

            bool readingPattern = false;
            std::string pattern = "";
            while(std::getline(patternFile, line)) {
                // Trim start
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), 
                            [](unsigned char c) { return !std::isspace(c); }));

                if(line.find("//") == 0 || line.empty())
                    continue;

                if(line.find("{") != std::string::npos) {
                    readingPattern = true;
                    continue;
                } else if(line.find("}") != std::string::npos) {
                    readingPattern = false;
                    patterns.push_back(pattern);
                    pattern = "";
                    continue;
                }

                if(!readingPattern)
                    error("Syntax error in pattern file", line);

                pattern.append(line + "\n");
            }
        }

        void initGolPattern(int id = 0) {
            generation = 0;
            if(id > patterns.size()) {
                error("Invalid pattern code", std::to_string(id));
                return;
            }

            displayPattern(patterns[id]);
        }

        void debugCellArray() {
            assert(arrayLength <= 512);
            std::cout << "Full cell Array: \n";
            for(int y = 0; y < numRows; y++) {
                for(int x = 0; x < rowLength; x++) {
                    std::cout << std::dec << x + y*rowLength << ": " 
                        << std::hex << cells[x + y * rowLength] 
                        << std::dec << (x != rowLength-1 ? ", " : "\n"); 
                }
            }
            std::cout << "\n";
        }

        void initGolRandom() {
            generation = 0;
            initGolClear();
            for(int i = 0; i < arrayLength; i++) { 
                for(int j = 0; j < 16; j++)  {
                    cells[i] |= (cellMaskAlive & (rand() % 3 < 1)) << (j*4); 
                }
            }
        }
        
        void initGolClear() {
            generation = 0;
            for(int i = 0; i < arrayLength; i++) { cells[i] = 0x0; }
        }

        void initGolFull() {
            generation = 0;
            for(int i = 0; i < arrayLength; i++) { 
                cells[i] = 0x1111111111111111;
            }
        }

        /*
         * More information about this algorithm, see section 2.4 of:
         * https://www.gathering4gardner.org/g4g13gift/math/RokickiTomas-GiftExchange-LifeAlgorithms-G4G13.pdf  
         * */
        void nextBlockState(int x, int y) {
            int nC = (x + 1) >= rowLength ?           0 : x+1;
            int pC = (x - 1) <          0 ? rowLength-1 : x-1;
            int nR = (y + 1) >= numRows   ?           0 : y+1;
            int pR = (y - 1) <          0 ?   numRows-1 : y-1;

            uint64_t c = cells[x + y * rowLength];

            uint64_t nw = cells[pC + pR * rowLength];
            uint64_t n  = cells[x  + pR * rowLength];
            uint64_t ne = cells[nC + pR * rowLength];

            uint64_t e  = cells[nC + y * rowLength];
            uint64_t w  = cells[pC + y * rowLength];

            uint64_t sw = cells[pC + nR * rowLength];
            uint64_t s  = cells[x  + nR * rowLength];
            uint64_t se = cells[nC + nR * rowLength];

            uint64_t r = (c << 4) + (c >> 4) 
                + (n  << 4) + n + (n  >> 4)
                + (s  << 4) + s + (s  >> 4)
                + (nw << 60)  +   (ne >> 60)
                + (w  << 60)  +   (e  >> 60)
                + (sw << 60)  +   (se >> 60);

            count[x + y * rowLength] = r;
            swap[x + y * rowLength] = (r | c) 
                & (r >> 1) & ~(r >> 2) & ~(r >> 3) 
                & 0x1111111111111111;
        }

        void update() {
            for(int y = 0; y < numRows; y++) {
                for(int x = 0; x < rowLength; x++) {
                    nextBlockState(x, y);
                    renderBlockToTexture(x,y);
                }
            }
        }

        void renderCellText() {
            for(int y = 0; y < numRows; y++) {
                for(int x = 0; x < rowLength; x++) {
                    renderBlockTextToTexture(x,y);
                }
            }
        }

        void renderBlockToTexture(int x, int y) {
            uint64_t c = cells[x + y * rowLength];
            const int rows = y * (gameSurface->pitch / 4);
            Uint32* pixels = (Uint32*) (gameSurface->pixels);
            
            // Set pixel color with mask from block (-(0x00...01) = FFFFFFFFFFFFFFFF)
            for(int i = 0; i < 16; i++) {
                int s = 4 * ((16-1) - i );
                pixels[(x*16) + i + rows] = cellColorAlive & -((c & (cellMaskAlive << s)) >> s);
            }
        }

        void renderBlockTextToTexture(int x, int y) {
            uint64_t blockCount = count[x + y * rowLength];
            
            for(int i = 0; i < 16; i++) {
                int s = 4 * ((16-1) - i);
                int c = (blockCount & (cellMaskCount << s)) >> s;
                
                int xPos = (x * 16) + i;
                SDL_FRect point = {
                    (float)(offsetX + xPos * pointSize), 
                    (float)(offsetY + y * pointSize), 
                    (float)pointSize, (float)pointSize};
                SDL_FRect clip = {
                    (float)(     (c%3) * numWidth), 
                    (float)((int)(c/3) * numHeight), 
                    (float)numWidth, (float)numHeight};
                SDL_FRect textRect = {
                    point.x + (pointSize - numWidth)/2.f, 
                    point.y + (pointSize-numHeight)/2.f, 
                    (float)numWidth, (float)numHeight};
                numbers.render(textRect.x, textRect.y, &clip);
            }
        }

        void focus() {
            int gx = focusCell.x % gameSize;
            int gy = focusCell.y % gameSize;

            int n = (gy-1 < 0 ? gameSize-1 : gy-1);
            int e = (gx+1 >= gameSize ? 0 : gx+1);
            int s = (gy+1 >= gameSize ? 0 : gy+1);
            int w = (gx-1 < 0 ? gameSize-1 : gx-1);

            const SDL_Point neighbours[] = { 
                {w, n}, {gx, n}, {e, n}, 
                {w, gy}, {e,gy}, 
                {w, s}, {gx, s}, {e, s}
            };

            SDL_FRect point = {
                (float)(offsetX + gx * pointSize), 
                (float)(offsetY + gy * pointSize), 
                (float)pointSize, (float)pointSize};

            for(int i = 0; i < std::size(neighbours); i++) {
                int px = neighbours[i].x;
                int py = neighbours[i].y;
                if(!getCellState(px,py))
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
            // TODO: Maybe save update calculations
            // if paused and already calculated 
            // (always set calculated to false when swaping, 
            // set it to true after calculation)
            // don't forget to disable surface invalidation
            SDL_FillSurfaceRect(gameSurface, nullptr, cellColorDead);
            update(); 
            gameTexture.update(gameSurface);
            gameTexture.render(offsetX, offsetY, gameSize*pointSize, gameSize*pointSize);

            if(withTextRendering)
                renderCellText();

            if(focusCell.x != -1 && focusCell.y != -1)
                focus();
            
            renderDebugRect("Conway's Game of Life", offsetX, offsetY, 
                    gameSize * pointSize, gameSize * pointSize); 

            if(!paused || advance > 0) {
                uint64_t* temp = cells;
                cells = swap;
                swap = temp;
                generation++;
            }

            if(advance > 0)
                advance--;
                

            onNumberKey(NUMBERKEY_UPDATE);
            if(numberKeysTexture.isLoaded())
                numberKeysTexture.render(screenWidth-numberKeysTexture.getWidth()-helpTextOffset, helpTextOffset);
            renderGeneration();
            helpTexture.render(helpTextOffset, screenHeight-helpTexture.getHeight()-helpTextOffset);
        }

        /*
         * Computes the next closest power of two
         * More information: https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
         */
        int nextPowerOfTwo(uint64_t n) {
           n--;
           n |= n >> 1;
           n |= n >> 2;
           n |= n >> 4;
           n |= n >> 8;
           n |= n >> 16;
           n |= n >> 32;
           return ++n;
        }

        bool getCellState(int gx, int gy) {
            int offset = (15 - gx%16) * 4;
            uint64_t block = cells[gx/16 + gy * rowLength];
            
            return block & (cellMaskAlive << offset);
        }

        void invertCellState(int gx, int gy) {
            int offset = (15-gx%16) * 4;
            uint64_t block = cells[gx/16 + gy * rowLength];
            cells[gx/16 + gy * rowLength] = block ^ (cellMaskAlive << offset);
        }
        
        void setCellState(int gx, int gy) {
            int offset = (15-gx%16) * 4;
            uint64_t block = cells[gx/16 + gy * rowLength];
            cells[gx/16 + gy * rowLength] = block & ~(cellMaskAlive << offset);
        }

        void unsetCellState(int gx, int gy) {
            int offset = (15-gx%16) * 4;
            uint64_t block = cells[gx/16 + gy * rowLength];
            cells[gx/16 + gy * rowLength] = block | (cellMaskAlive << offset);
        }

        void renderGeneration() {
            std::string str = "Gen: " + std::to_string(generation);
            SDL_Surface* textSurface = TTF_RenderText_Blended(monoFont, 
                    str.c_str(), str.length(), {255, 255, 255});
            SDL_FRect fclip = {0.0, 0.0, (float)textSurface->w, (float)textSurface->h};
            generationTexture.update(textSurface);

            generationTexture.render((screenWidth - textSurface->w)/2.f, 
                    10.f, &fclip);
            SDL_DestroySurface(textSurface);
        }
                    
        /*
         * Parses a game of life pattern string 
         * where 'O' is a live cell and all other characters dead cells.
         * Such pattern can be found at:
         * http://www.radicaleye.com/lifepage/lexicon.html         
         */
        void displayPattern(const std::string& patternStr) {
            char alive = 'O';

            int lineLength = 0;
            std::vector<std::string> lines {};
            
            std::string line;    
            std::istringstream stream(patternStr);
            while (std::getline(stream, line)) {
                // Trim start/end
                line.erase(line.begin(), std::find_if(line.begin(), line.end(), 
                            [](unsigned char c) { return !std::isspace(c); }));
                line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char c) 
                            { return !std::isspace(c); }).base(), line.end());

                if(lineLength == 0)
                    lineLength = line.length();

                if(line.length() != lineLength) {
                    error("Pasted pattern contains uneven line lengths");
                    return;
                }

                lines.push_back(line);
            }

            if(lines.size() > gameSize || lineLength > gameSize) {
                error("Pasted pattern is too large for game of size", std::to_string(gameSize));
                return;
            }

            paused = true;
            initGolClear();

            int offsetX = (gameSize - lineLength)/2;
            int offsetY = (gameSize - lines.size())/2;

            int gx = 0;
            int gy = 0;
            for(int i = 0; i < lines.size(); i++) {
                gx = 0;
                for(int c = 0; c < lines[i].length(); c++) {
                    if(lines[i].at(c) == alive)
                        invertCellState(offsetX + gx, offsetY + gy);          
                    gx++;
                }
                gy++;
            }
        }

        void mouseDownEventHandler(SDL_Event& event) {
            mouseLeftDown = true;
            focusCell = {-1,-1};
            mouseInteraction(true);
        }
        
        void mouseUpEventHandler(SDL_Event& event) { 
            mouseLeftDown = false;
            mouseCellState = false;
        }

        void mouseInteraction(bool isClick) {
            if(mousePos.x < offsetX || mousePos.x >= offsetX + gameSize*pointSize ||
                    mousePos.y < offsetY || mousePos.y >= offsetY + gameSize*pointSize) {
                focusCell = {-1,-1};
                drawMode = false;
                return;
            }

            int gx = (mousePos.x-offsetX)/pointSize;
            int gy = (mousePos.y-offsetY)/pointSize;

            if(isClick)
                mouseCellState = getCellState(gx,gy);

            if(gx == lastMouseCell.x && gy == lastMouseCell.y)
                return;
           
            if(drawMode && mouseCellState) {
                setCellState(gx, gy);
            } else if(drawMode && !mouseCellState) {
                unsetCellState(gx, gy);
            } else {
                focusCell = {gx, gy};
            }

            lastMouseCell = {gx,gy};
        }

        void mouseMoveEventHandler(SDL_Event& event) {
            mousePos.x = event.motion.x * windowScreenRatio;
            mousePos.y = event.motion.y* windowScreenRatio;
            
            if(mouseLeftDown)
                mouseInteraction(false);
        }

        void onNumberKey(int n) {
            bool running = numberKeyTimer.isRunning();
            if(!running && n == NUMBERKEY_UPDATE) 
                return;

            numberKeyTimer.stop();
            long time = numberKeyTimer.getMs();

            if(n == NUMBERKEY_CANCEL) {
                numberKeys = "";
                numberKeysTexture.destroy();
                numberKeyTimer.stop();
                return;
            } else if(n == NUMBERKEY_UPDATE && running) {
                if(time > numberKeyTimeout) {
                    int patternId = 0;
					std::stringstream convert;
					convert << numberKeys;
					convert >> patternId;
                    initGolPattern(patternId);
                    numberKeys = "";
                    numberKeysTexture.destroy();
                    numberKeyTimer.stop();
                    return;
                }
                
                numberKeyTimer.resume();
                return;
            } 
            
            if(!running)
                numberKeyTimer.start();
            else
                numberKeyTimer.resume();

            numberKeys.append(std::to_string(n));
            numberKeysTexture.loadText(numberKeys, monoFont, {255,255,255});
        }

        void keyDownEventHandler(SDL_Event& event) {
            switch(event.key.key) {
                case SDLK_SPACE:
                    drawMode = false;
                    paused = !paused;
                    focusCell = {-1,-1};
                    break;
                
                case SDLK_R:
                    initGolRandom();
                    focusCell = {-1,-1};
                    break;

                case SDLK_ESCAPE:
                    focusCell = {-1,-1};
                    drawMode = false;
                    onNumberKey(NUMBERKEY_CANCEL);
                    break;

                case SDLK_C:
                    initGolClear();
                    break;

                case SDLK_D:
                    drawMode = !drawMode;
                    paused = true;
                    focusCell = {-1,-1};
                    lastMouseCell = {-1,-1};
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
                    focusCell = {-1,-1};
                    break;

                case SDLK_1: onNumberKey(1); break;
                case SDLK_2: onNumberKey(2); break;
                case SDLK_3: onNumberKey(3); break;
                case SDLK_4: onNumberKey(4); break;
                case SDLK_5: onNumberKey(5); break;
                case SDLK_6: onNumberKey(6); break;
                case SDLK_7: onNumberKey(7); break;
                case SDLK_8: onNumberKey(8); break;
                case SDLK_9: onNumberKey(9); break;
                case SDLK_0: onNumberKey(0); break;

                case SDLK_V: 
                    if(isPaste()) {
                        paused = true;
                        drawMode = false;
                        displayPattern(SDL_GetClipboardText());
                    }
                    break;

                default:
                    break;
            }
        }

        bool isPaste() {
            SDL_Keymod modifier = SDL_GetModState();
            return (modifier == SDL_KMOD_LCTRL) 
                | (modifier == SDL_KMOD_RCTRL) 
                | (modifier == SDL_KMOD_LGUI) 
                | (modifier == SDL_KMOD_RGUI);
        }
};



#endif /* CONWAYAPP_H */

