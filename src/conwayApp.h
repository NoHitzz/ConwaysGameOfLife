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
        int minWindowSize = 448;

        const int gameSize = 0;
        const int rowLength = 0;
        const int numRows = 0;
        const int arrayLength = 0;
        int minOffset = 50;
        double pointSize = 1.0; 
        int offsetX;
        int offsetY;
        long generation = 0;
        int zoomFactor;
        SDL_Point zoomIndexOffset;
        int zoomedSize;

        const uint64_t cellMaskAlive = 0x1;
        const uint64_t cellMaskCount = 0xF;

        const Uint32 cellColorAlive = 0xFFFFFFFF;
        const Uint32 cellColorDead = 0x00000000;

        uint64_t* cells;
        uint64_t* swap;
        uint64_t* count;

        TTF_Font* fontSans = nullptr; 
        bool withTextRendering;
        int textCutoff = 128;
        int fontSize;

        Texture numbers;
        int maxNumWidth;
        int maxNumHeight;

        SDL_Point mousePos = {0,0};
        SDL_Point lastMouseCell = {-1,-1};
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
        bool showHelp = false;
        int advance = 0;

        int pixelPitch;
        Uint32* pixelData = nullptr;
        Texture gameTexture;
        Texture generationTexture;
        Texture helpTexture;
        Texture statusTexture;
        int statusOffset = 8;
        int helpTextPadding = 100;
        SDL_Point helpTextOffset = {0,0};

        std::vector<std::string> patterns {};

        std::string helpText = 
            "            --- Help --- \n"
            " \n"
            "Controls: \n"
            "Keycombination:     Function:   \n"
            "  r                   reset   \n"
            "  c                   clear \n"
            "   \n"
            "  d                   enter/exit draw mode \n"
            "  left mouse          inspect/draw \n"
            "  esc                 leave mode/selection \n"
            "   \n"
            "  ctrl-v              paste pattern  \n"
            "  0-9*                load pattern from file \n"
            "   \n"
            "  scroll              zoom in/out  \n"
            " \n"
            "  space               pause/continue  \n"
            "  right arrow         step \n"
            " \n"
            " \n"
            "Paste patterns must follow the Life Lexicon format. \n"
            "Press escape to close this pop-up. \n";

    public:
        ConwayApp(uint64_t size) : SDLApp("Game of Life", 640,  480), 
        gameSize(std::max(nextPowerOfTwo(size), 16)), 
        rowLength(gameSize / 16), // Every array entry packs 16 horizontal cells
        numRows(gameSize),
        arrayLength(rowLength * numRows) { 

            zoomIndexOffset = {0,0};
            zoomFactor = 1;
            zoomedSize = gameSize;
            helpTexture.setRenderer(renderer);
            helpTexture.loadWrappedText(helpText, monoFont, {255, 255, 255}, 640);
            numberKeysTexture.setRenderer(renderer);
            statusTexture.setRenderer(renderer);
            statusTexture.loadText("press 'h' for help", monoFont, {200, 200, 200});

            SDL_SetWindowMinimumSize(window, minWindowSize, minWindowSize);

            cells = new uint64_t[arrayLength];
            swap = new uint64_t[arrayLength];
            count = new uint64_t[arrayLength];
            
            pixelPitch = gameSize * sizeof(Uint32);
            pixelData = new Uint32[numRows* pixelPitch/4];
            gameTexture.setRenderer(renderer);
            gameTexture.loadBlank(gameSize, gameSize, SDL_TEXTUREACCESS_STREAMING, SDL_PIXELFORMAT_RGBA8888);
            SDL_SetTextureScaleMode(gameTexture.getTexture(), SDL_SCALEMODE_NEAREST);
            generationTexture.setRenderer(renderer);
            generationTexture.loadBlank(256, 512, SDL_TEXTUREACCESS_STREAMING, SDL_PIXELFORMAT_ARGB8888);

            loadPatterns();
            windowResized();

            initGolRandom();

            // std::cout << "requesteSize: " << size << ", gameSize: " << gameSize 
            //     << ", packedLength: (" << rowLength << ", " << numRows 
            //     << "), arrayLength: " << arrayLength << "\n";
        }

        ~ConwayApp() { 
            TTF_CloseFont(fontSans);
            delete[] pixelData;
            delete[] cells;
            delete[] swap;
            delete[] count;
        }

        void windowResized() {
            SDL_GetRenderOutputSize(renderer, &screenWidth, &screenHeight);
            int size = std::min(screenHeight, screenWidth);
            pointSize = (float)(size-2*minOffset)/zoomedSize;  

            offsetX = (screenWidth-zoomedSize*pointSize)/2.f;
            offsetY = (screenHeight-zoomedSize*pointSize)/2.f;

            helpTextOffset.x = (screenWidth - helpTexture.getWidth())/2;
            helpTextOffset.y = (screenHeight - helpTexture.getHeight())/2;

            withTextRendering = (zoomedSize <= textCutoff);

            loadNumbersTextue();
        }

        void loadNumbersTextue() {
            if(!withTextRendering)
                return;

            if(fontSans != nullptr) {
                TTF_CloseFont(fontSans);
                fontSans = nullptr;
            }

            fontSize = pointSize;
            fontSans = TTF_OpenFont((getBasePath() + "../resources/OpenSans-Regular.ttf").c_str(), fontSize);
            if(fontSans == nullptr) 
                error("SDL Font creation failed", SDL_GetError());

            maxNumWidth = 0;
            maxNumHeight = 0;
            Texture* nums = new Texture[9]();
            for(int i = 0; i < 9; i++) {
                nums[i].setRenderer(renderer);
                nums[i].loadText(std::to_string(i), fontSans, {0,0,255});
                maxNumWidth = std::max(maxNumWidth, nums[i].getWidth());
                maxNumHeight = std::max(maxNumHeight, nums[i].getHeight());
            }

            numbers.setRenderer(renderer);
            numbers.loadBlank(maxNumWidth*3, maxNumHeight*3, SDL_TEXTUREACCESS_TARGET, nums->getFormat());
            numbers.setAsRenderTarget();
            for(int i = 0; i < 9; i++) {
                int x = (maxNumWidth-nums[i].getWidth())/2;
                int y = (maxNumHeight-nums[i].getHeight())/2;
                SDL_FRect dest = {(float)((i%3)*maxNumWidth + x), (float)((int)(i/3)*maxNumHeight + y), 
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

        void renderBlockToTexture(int x, int y) {
            uint64_t c = cells[x + y * rowLength];
            const int rows = y * pixelPitch/4;

            // Set pixel color with mask from block (-(0x00...01) = FFFFFFFFFFFFFFFF)
            for(int i = 0; i < 16; i++) {
                int s = 4 * ((16-1) - i );
                pixelData[(x*16) + i + rows] = cellColorAlive & -((c & (cellMaskAlive << s)) >> s);
            }
        }

        void updateCellText() {
            for(int y = zoomIndexOffset.y; y < zoomIndexOffset.y + zoomedSize; y++) {
                for(int x = std::floor(zoomIndexOffset.x/16.f); 
                        x < std::ceil((zoomIndexOffset.x + zoomedSize)/16.f); x++) {
                    renderBlockTextToTexture(x,y);
                }
            }
        }

        void renderBlockTextToTexture(int x, int y) {
            uint64_t blockCount = count[x + y * rowLength];

            for(int i = 0; i < 16; i++) {
                int s = 4 * ((16-1) - i);
                int c = (blockCount & (cellMaskCount << s)) >> s;

                int xPos = (x * 16) + i;

                if(xPos < zoomIndexOffset.x || xPos >= (zoomIndexOffset.x + zoomedSize) 
                        || y < zoomIndexOffset.y || y >= (zoomIndexOffset.y + zoomedSize))
                    continue;

                SDL_FRect point = {
                    (float)(offsetX + (xPos-zoomIndexOffset.x) * pointSize),
                    (float)(offsetY + (y - zoomIndexOffset.y)  * pointSize),
                    (float)pointSize, (float)pointSize};
                SDL_FRect clip = {
                    (float)(     (c%3) * maxNumWidth),
                    (float)((int)(c/3) * maxNumHeight),
                    (float)maxNumWidth, (float)maxNumHeight};
                SDL_FRect textRect = {
                    (float)(point.x + (pointSize - maxNumWidth)/2.f),
                    (float)(point.y + (pointSize-maxNumHeight)/2.f),
                    (float)maxNumWidth, (float)maxNumHeight};
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
                (float)(offsetX + (gx-zoomIndexOffset.x) * pointSize), 
                (float)(offsetY + (gy-zoomIndexOffset.y) * pointSize), 
                (float)pointSize, (float)pointSize};

            for(int i = 0; i < std::size(neighbours); i++) {
                int px = neighbours[i].x;
                int py = neighbours[i].y;
                if(!getCellState(px,py) 
                        || px < zoomIndexOffset.x || px >= (zoomedSize + zoomIndexOffset.x)
                        || py < zoomIndexOffset.y || py >= (zoomedSize + zoomIndexOffset.y))
                    continue;
                SDL_FRect point = {
                    (float)(offsetX + (px-zoomIndexOffset.x) * pointSize), 
                    (float)(offsetY + (py-zoomIndexOffset.y) * pointSize), 
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
            update(); 
            
            SDL_Rect zoomedUpdateClip = {0, zoomIndexOffset.y, pixelPitch/4, zoomedSize};
            gameTexture.update((pixelData + zoomIndexOffset.y * pixelPitch/4), pixelPitch, &zoomedUpdateClip);

            SDL_FRect zoomClip = {(float)(zoomIndexOffset.x), (float)(zoomIndexOffset.y), 
                (float)(zoomedSize), (float)(zoomedSize)};
            gameTexture.render(offsetX, offsetY, zoomedSize * pointSize, zoomedSize * pointSize, &zoomClip);

            if(withTextRendering)
                updateCellText();

            if(focusCell.x != -1 && focusCell.y != -1)
                focus();

            renderDebugRect("Conway's Game of Life", offsetX, offsetY, 
                    zoomedSize * pointSize, zoomedSize * pointSize); 

            if(!paused || advance > 0) {
                uint64_t* temp = cells;
                cells = swap;
                swap = temp;
                generation++;
            }

            if(advance > 0)
                advance--;

            renderGeneration();

            onNumberKey(NUMBERKEY_UPDATE);
            if(numberKeysTexture.isLoaded())
                numberKeysTexture.render(screenWidth-numberKeysTexture.getWidth()-statusOffset, statusOffset);

            if(showHelp) {
                SDL_FRect helpBackground = {(float)helpTextOffset.x - helpTextPadding, 
                    (float)helpTextOffset.y - helpTextPadding, 
                    (float)helpTexture.getWidth() + 2*helpTextPadding, 
                    (float)helpTexture.getHeight() + 2*helpTextPadding};
                SDL_SetRenderDrawColor(renderer, 15, 15, 15, 250);
                SDL_RenderFillRect(renderer, &helpBackground);
                SDL_SetRenderDrawColor(renderer, 255, 255, 255, 25);
                drawRectangle(renderer, helpBackground, 2);
                helpTexture.render(helpTextOffset.x, helpTextOffset.y);
            }

            statusTexture.render(statusOffset, screenHeight-statusOffset-statusTexture.getHeight());
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
            cells[gx/16 + gy * rowLength] = block | (cellMaskAlive << offset);
        }

        void unsetCellState(int gx, int gy) {
            int offset = (15-gx%16) * 4;
            uint64_t block = cells[gx/16 + gy * rowLength];
            cells[gx/16 + gy * rowLength] = block & ~(cellMaskAlive << offset);
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
            char dead = '.';

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
            // initGolClear();

            int offsetX = (gameSize - lineLength)/2;
            int offsetY = (gameSize - lines.size())/2;

            int gx = 0;
            int gy = 0;
            for(int i = 0; i < lines.size(); i++) {
                gx = 0;
                for(int c = 0; c < lines[i].length(); c++) {
                    if(lines[i].at(c) == alive)
                        setCellState(offsetX + gx, offsetY + gy);          
                    else if(lines[i].at(c) == dead)
                        unsetCellState(offsetX + gx, offsetY + gy);          
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
            lastMouseCell = {-1, -1};
            mouseCellState = false;
        }

        SDL_Point getCellPosFromScreenPos(const SDL_Point& screenPoint) {
            int x = (int)((screenPoint.x - offsetX) / pointSize);
            int y = (int)((screenPoint.y - offsetY) / pointSize);
            return {x + zoomIndexOffset.x, y + zoomIndexOffset.y};
        }

        void mouseInteraction(bool isClick) {
            if(mousePos.x < offsetX || mousePos.x >= offsetX + zoomedSize*pointSize ||
                    mousePos.y < offsetY || mousePos.y >= offsetY + zoomedSize*pointSize) {
                focusCell = {-1,-1};
                drawMode = false;
                return;
            }

            SDL_Point cellPos = getCellPosFromScreenPos(mousePos); 

            if(isClick)
                mouseCellState = getCellState(cellPos.x, cellPos.y);

            if(cellPos.x  == lastMouseCell.x && cellPos.y == lastMouseCell.y)
                return;

            if(drawMode && !mouseCellState) {
                setCellState(cellPos.x, cellPos.y);
            } else if(drawMode && mouseCellState) {
                unsetCellState(cellPos.x, cellPos.y);
            } else {
                focusCell = {cellPos.x, cellPos.y};
            }

            lastMouseCell = {cellPos.x, cellPos.y};
        }

        void mouseMoveEventHandler(SDL_Event& event) {
            mousePos.x = event.motion.x * windowScreenRatio;
            mousePos.y = event.motion.y* windowScreenRatio;

            if(mouseLeftDown)
                mouseInteraction(false);
        }

        void mouseWheelEventHandler(SDL_Event& event) {
            zoom(event.wheel.y);
        }

        bool isInRect(const SDL_Point& point, const SDL_Rect& rect) {
            return point.x >= rect.x && point.y >= rect.y 
                && point.x <= (rect.x + rect.w) && point.y <= (rect.y + rect.h);
        }

        void zoom(float amount) {
            SDL_Point mouseCell = getCellPosFromScreenPos(mousePos);
            bool useMousePos = isInRect(mousePos, {offsetX, offsetY, 
                    (int)(zoomedSize*pointSize), (int)(zoomedSize*pointSize)});
            int maxZoom = gameSize/2;
            if(amount > 0 && zoomFactor != maxZoom) {
                zoomFactor = std::min(zoomFactor * 2, maxZoom);            
                zoomedSize = gameSize / zoomFactor;
                zoomIndexOffset.x = std::min(std::min(std::max((useMousePos ? 
                                    mouseCell.x - zoomedSize/2 
                                    : (gameSize-zoomedSize)/2), 0), gameSize-2), gameSize - zoomedSize);
                zoomIndexOffset.y = std::min(std::min(std::max((useMousePos ? 
                                    mouseCell.y - zoomedSize/2 
                                    : (gameSize-zoomedSize)/2), 0), gameSize-2), gameSize - zoomedSize);
                windowResized();
            } else if(amount < 0 && zoomFactor != 1) {
                zoomFactor = std::max(zoomFactor / 2, 1);
                zoomedSize = gameSize / zoomFactor;
                zoomIndexOffset.x = std::min(std::min(std::max((useMousePos ? 
                                    mouseCell.x - zoomedSize/2 
                                    : (gameSize-zoomedSize)/2), 0), gameSize-2), gameSize - zoomedSize);
                zoomIndexOffset.y = std::min(std::min(std::max((useMousePos ? 
                                    mouseCell.y - zoomedSize/2 
                                    : (gameSize-zoomedSize)/2), 0), gameSize-2), gameSize - zoomedSize);
                windowResized();
            }
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

                case SDLK_H: showHelp = !showHelp; break;

                case SDLK_R:
                             initGolRandom();
                             focusCell = {-1,-1};
                             break;

                case SDLK_ESCAPE:
                             focusCell = {-1,-1};
                             drawMode = false;
                             onNumberKey(NUMBERKEY_CANCEL);
                             showHelp = false;
                             break;

                case SDLK_C:
                             initGolClear();
                             break;

                case SDLK_D:
                             drawMode = !drawMode;
                             paused = true;
                             focusCell = {-1,-1};
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

