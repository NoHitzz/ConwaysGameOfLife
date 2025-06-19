// 
// main.cpp
// chip8-emulator
// 
// Noah Hitz 2025
// 

#ifndef MAIN_CPP
#define MAIN_CPP

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <string>

class Texture {
    private:
        SDL_Renderer* m_renderer; 
        SDL_Texture* m_texture;

        int m_width;
        int m_height;

    public:
        Texture(SDL_Renderer* renderer) {
            assert(renderer != nullptr);
            m_texture = nullptr;
            m_width = 0;
            m_height = 0;
            m_renderer = renderer;
        }

        ~Texture() {
            destroy();
        }


        bool loadImg(std::string path) {
            destroy();
            m_texture = IMG_LoadTexture(m_renderer, path.c_str());
            if(m_texture == nullptr) {
                error("IMG_LoadTexture failed", SDL_GetError());
                return false;
            }

            m_width = m_texture->w;
            m_height = m_texture->h;
            return true;
        }


        void destroy() {
            if(m_texture != nullptr) 
                SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
            m_width = 0;
            m_height = 0;
        }


        void render(float x, float y, SDL_FRect* clip = nullptr) {
            float width = m_width;
            float height = m_height;
            if(clip != nullptr) {
                width = clip->w;
                height = clip->h;
            }

            render(x, y, width, height, clip);
        }

        void render(float x, float y, float width, float height, 
                SDL_FRect* clip = nullptr) {
            SDL_FRect renderQuad = {x, y, 
                width, height};
            SDL_RenderTexture(m_renderer, m_texture, clip, &renderQuad);
        }

        int getWidth() { return m_width; }
        int getHeight() { return m_height; }

        SDL_Texture* getTexture() { return m_texture; }

    private:
        void error(std::string msg, std::string detail = "") {
            std::cerr << "[" << "Texture" << "] " << msg; 
            if(!detail.empty())
                std::cerr << ": " << detail;
            std::cerr << "\n";
        }
};

#endif /* MAIN_CPP */
