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

        SDL_FlipMode m_flip;
        SDL_FPoint* m_rotCenter;
        double m_rotation;

        int m_width;
        int m_height;

    public:
        Texture(SDL_Renderer* renderer) {
            assert(renderer != nullptr);
            m_texture = nullptr;
            m_width = 0;
            m_height = 0;
            m_renderer = renderer;

            setBlendMode(SDL_BLENDMODE_BLEND);
            m_flip = SDL_FLIP_NONE;
            m_rotCenter = nullptr;
            m_rotation = 0.0;
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

        bool loadText(std::string text, TTF_Font* font, SDL_Color color) {
            bool result = true;
            destroy();
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, text.c_str(), text.length(), color);
            if(textSurface == nullptr) {
                error("TTF_RenderText_Blended failed", SDL_GetError());
                result &= false;
            }
            m_texture = SDL_CreateTextureFromSurface(m_renderer, textSurface);
            if(m_texture == nullptr) {
                error("SDL_CreateTextureFromSurface failed", SDL_GetError());
                result &= false;
            }
            
            SDL_DestroySurface(textSurface);
            m_width = m_texture->w;
            m_height = m_texture->h;

            return result;
        }

        void destroy() {
            if(m_texture != nullptr) 
                SDL_DestroyTexture(m_texture);
            m_texture = nullptr;
            m_width = 0;
            m_height = 0;
            
            setBlendMode(SDL_BLENDMODE_BLEND);
            m_flip = SDL_FLIP_NONE;
            m_rotCenter = nullptr;
            m_rotation = 0.0;
        }

        void colorMult(Uint8 r, Uint8 g, Uint8 b) {
            SDL_SetTextureColorMod(m_texture, r, g, b);
        }

        void setBlendMode(SDL_BlendMode mode) {
            SDL_SetTextureBlendMode(m_texture, mode);
        }

        void alphaMult(Uint8 alpha) {
            SDL_SetTextureAlphaMod(m_texture, alpha);
        }

        void setRotation(double rotation, SDL_FPoint* center = nullptr) {
            m_rotation = rotation;
            m_rotCenter = center;
        }

        void flip(SDL_FlipMode flip) {
            m_flip = flip;
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
            SDL_RenderTextureRotated(m_renderer, m_texture, clip, &renderQuad, m_rotation, m_rotCenter, m_flip);
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
