// 
// texture.h
// ConwaysGameOfLife
// 
// Noah Hitz 2025
// 

#ifndef TEXTURE_H
#define TEXTURE_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>
#include <string>
#include <vector>

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
        Texture(SDL_Renderer* renderer = nullptr) {
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

        void setRenderer(SDL_Renderer* renderer) {
            m_renderer = renderer;
        }

        void setAsRenderTarget() {
            SDL_SetRenderTarget(m_renderer, m_texture);
        }

        void unsetRenderTarget() {
            SDL_SetRenderTarget(m_renderer, nullptr);
        }

        bool loadSurface(SDL_Surface* surface) {
            destroy();
            m_texture = SDL_CreateTextureFromSurface(m_renderer, surface);
            if(m_texture == nullptr) {
                error("SDL_CreateTextureFromSurface failed", SDL_GetError());
                return false;
            }
            
            m_width = m_texture->w;
            m_height = m_texture->h;

            return true;

        }

        bool loadBlank(int width, int height, SDL_TextureAccess access, SDL_PixelFormat format) {
            destroy();

            m_texture = SDL_CreateTexture(m_renderer, format, access, width, height);
            if(m_texture == nullptr) {
                error("SDL_CreateTexture failed", SDL_GetError());
                return false;
            }

            m_width = width;
            m_height = height;

            return true;
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
        
        bool loadImgFromMemory(void* pixels, int width, int height, 
                SDL_PixelFormat format, int pitch, SDL_Palette* palette) {
            destroy();
            bool result = true;
            SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, format, pixels, pitch);
            if(surface == nullptr) {
                error("SDL_CreateSurfaceFrom failed", SDL_GetError());
                result &= false;
            }
            
            SDL_SetSurfacePalette(surface, palette);
            m_texture = SDL_CreateTextureFromSurface(m_renderer, surface);
            if(m_texture == nullptr) {
                error("SDL_CreateTextureFromSurface failed", SDL_GetError());
                result &= false;
            }
            
            SDL_DestroySurface(surface);
            SDL_DestroyPalette(palette);
            m_width = m_texture->w;
            m_height = m_texture->h;

            return result;
        }

        bool loadText(std::string text, TTF_Font* font, SDL_Color color) {
            bool result = true;
            destroy();
            SDL_Surface* textSurface = TTF_RenderText_Blended(font, 
                    text.c_str(), text.length(), color);
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

        bool loadTextFast(std::string text, TTF_Font* font, SDL_Color color) {
            bool result = true;
            destroy();
            SDL_Surface* textSurface = TTF_RenderText_Solid(font, 
                    text.c_str(), text.length(), color);
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

        bool loadWrappedText(std::string text, TTF_Font* font, SDL_Color color, int maxWidth) {
            bool result = true;
            destroy();
            SDL_Surface* textSurface = TTF_RenderText_Blended_Wrapped(font,
                    text.c_str(), text.length(), color, maxWidth);
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

        /*
         * From SDL Wiki: This is a fairly slow function, 
         * intended for use with static textures that do not change often.         
         * Maybe use lock/unlock on streaming texture instead
         */
        void update(SDL_Surface* surface, SDL_Rect* clip = nullptr) {
            SDL_UpdateTexture(m_texture, clip, surface->pixels, surface->pitch);
        }
        
        /*
         * From SDL Wiki: This is a fairly slow function, 
         * intended for use with static textures that do not change often.         
         * Maybe use lock/unlock on streaming texture instead
         */
        void update(void* pixels, int pitch, SDL_Rect* clip = nullptr) {
            SDL_UpdateTexture(m_texture, clip, pixels, pitch);
        }

        int lock(const SDL_Rect* rect, void** pixels, int* pitch) {
            return SDL_LockTexture(m_texture, rect, pixels, pitch);
        }

        void unlock() {
            SDL_UnlockTexture(m_texture);
        }

        static SDL_Palette* generateGrayscalePalette(const int steps) {
            SDL_Palette* pal = SDL_CreatePalette(steps);
            std::vector<SDL_Color> colors(steps);
            for (int i = 0; i < steps; i++) {
                Uint8 c = (Uint8) (255.0*((float)i/steps));
                colors[i] = {c, c, c, 0xFF};
            }

            SDL_SetPaletteColors(pal, colors.data(), 0, steps);
            return pal;
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

        void clear(SDL_Color color) {
            setAsRenderTarget();
            SDL_SetRenderDrawColor(m_renderer, color.r, color.g, color.b, color.a);
            SDL_RenderClear(m_renderer);
            unsetRenderTarget();
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
            SDL_FRect renderQuad = {x, y, width, height};

            SDL_RenderTextureRotated( m_renderer, m_texture, 
                    clip, &renderQuad, m_rotation, m_rotCenter, m_flip);
        }

        int getWidth() { return m_width; }
        int getHeight() { return m_height; }

        SDL_Texture* getTexture() { return m_texture; }
        SDL_PixelFormat getFormat() { return m_texture->format; }

        bool isLoaded() { return m_texture != nullptr; }

    private:
        void error(std::string msg, std::string detail = "") {
            std::cerr << "[" << "Texture" << "] " << msg; 
            if(!detail.empty())
                std::cerr << ": " << detail;
            std::cerr << "\n";
        }
};

#endif /* TEXTURE_H */
