#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <unordered_map>

// Cache for text textures to avoid re-rendering the same text repeatedly
struct TextCache {
    std::unordered_map<std::string, SDL_Texture*> textures;
    
    void clear(SDL_Renderer* renderer) {
        for (auto& pair : textures) {
            SDL_DestroyTexture(pair.second);
        }
        textures.clear();
    }
    
    ~TextCache() {
        // Note: Textures should be destroyed before renderer is destroyed
        // Call clear() manually in cleanup
    }
};

// Text alignment options
enum class TextAlign {
    LEFT,
    CENTER,
    RIGHT
};

// Helper function to render text
// Returns true on success, false on failure
// x, y: position (meaning depends on alignment)
// alignX, alignY: horizontal and vertical alignment
bool renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text,
                float x, float y, SDL_Color color, 
                TextAlign alignX = TextAlign::LEFT, TextAlign alignY = TextAlign::LEFT,
                TextCache* cache = nullptr) {
    SDL_Texture* textTexture = nullptr;
    bool useCache = (cache != nullptr);
    
    // Check cache first if available
    if (useCache) {
        std::string cacheKey = text + "_" + std::to_string(color.r) + "_" + 
                              std::to_string(color.g) + "_" + std::to_string(color.b);
        auto it = cache->textures.find(cacheKey);
        if (it != cache->textures.end()) {
            textTexture = it->second;
        } else {
            // Create new texture and cache it
            SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, color);
            if (!surface) {
                std::cerr << "TTF_RenderText_Blended failed: " << SDL_GetError() << std::endl;
                return false;
            }
            textTexture = SDL_CreateTextureFromSurface(renderer, surface);
            SDL_DestroySurface(surface);
            
            if (!textTexture) {
                std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
                return false;
            }
            cache->textures[cacheKey] = textTexture;
        }
    } else {
        // No cache, create texture on the fly
        SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), 0, color);
        if (!surface) {
            std::cerr << "TTF_RenderText_Blended failed: " << SDL_GetError() << std::endl;
            return false;
        }
        textTexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
        
        if (!textTexture) {
            std::cerr << "SDL_CreateTextureFromSurface failed: " << SDL_GetError() << std::endl;
            return false;
        }
    }
    
    // Get texture dimensions
    float textWidth, textHeight;
    SDL_GetTextureSize(textTexture, &textWidth, &textHeight);
    
    // Set up destination rectangle
    SDL_FRect destRect = {x, y, textWidth, textHeight};
    
    // Render the texture
    SDL_RenderTexture(renderer, textTexture, nullptr, &destRect);
    
    // Clean up if not caching
    if (!useCache) {
        SDL_DestroyTexture(textTexture);
    }
    
    return true;
}

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_ttf
    if (!TTF_Init()) {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create window
    SDL_Window* window = SDL_CreateWindow(
        "SDL3 Text Rendering",
        800, 600,
        SDL_WINDOW_RESIZABLE
    );
    
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Load font
    TTF_Font* font = TTF_OpenFont("Gentium-R.ttf", 48);
    if (!font) {
        std::cerr << "TTF_OpenFont failed: " << SDL_GetError() << std::endl;
        std::cerr << "Make sure you have a font file in the same directory" << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }

    // Create a text cache for better performance
    TextCache textCache;

    // Main loop
    bool running = true;
    SDL_Event event;

    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            else if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        // Clear screen with dark background
        SDL_SetRenderDrawColor(renderer, 30, 30, 40, 255);
        SDL_RenderClear(renderer);

        // Get window dimensions for positioning
        int windowWidth, windowHeight;
        SDL_GetWindowSize(window, &windowWidth, &windowHeight);

        // Render multiple texts with different colors and positions
        SDL_Color white = {255, 255, 255, 255};
        SDL_Color red = {255, 100, 100, 255};
        SDL_Color green = {100, 255, 100, 255};
        SDL_Color blue = {100, 150, 255, 255};
        SDL_Color yellow = {255, 255, 100, 255};

        renderText(renderer, font, "Hello, SDL3!", 50, 50, white, &textCache);
        renderText(renderer, font, "Multiple Texts!", 50, 120, red, &textCache);
        renderText(renderer, font, "With Colors!", 50, 190, green, &textCache);
        renderText(renderer, font, "Easy Rendering", 50, 260, blue, &textCache);
        renderText(renderer, font, "Press ESC to quit", 50, windowHeight - 80, yellow, &textCache);

        // Present
        SDL_RenderPresent(renderer);

        // Small delay to prevent high CPU usage
        SDL_Delay(16); // ~60 FPS
    }

    // Cleanup
    textCache.clear(renderer);
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
