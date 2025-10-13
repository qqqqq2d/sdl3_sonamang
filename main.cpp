#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <random>
#include <locale>
#include <codecvt>
#include <algorithm>
#include <map>

std::string sõna;
std::ifstream sonade_list("sonad3.txt");

std::string kombinatsioon;
std::ifstream kombinatsioonide_list("kombinatsioonid3.txt");

std::vector<std::string> kombinatsioonid{};
std::vector<std::string> sõnad{};

int suvaline_kombinatsioon;
std::random_device dev;
std::mt19937 rng(dev());

std::string sõnavahetus() {
    while (std::getline(kombinatsioonide_list, kombinatsioon)) {
        kombinatsioonid.push_back(kombinatsioon);
    }

    std::uniform_int_distribution<std::mt19937::result_type> kombinatsiooni_vahemik(0, kombinatsioonid.size()-1);
    suvaline_kombinatsioon = kombinatsiooni_vahemik(rng);
    return kombinatsioonid[suvaline_kombinatsioon];
}

void näita_sõnu(std::vector<std::string> sõnad) {
    for (auto i : sõnad) {
        std::cout << i << " ";
    }
    std::cout << sõnad.size() << std::endl;
}

void vilkumine(float &opacity, std::string choice, float multiplier, float &a_m) {
    if (choice == "up" && opacity < 0.95f)
        opacity += 0.01f * multiplier * a_m;
    else if (opacity < 0.95f) {
        opacity -= 0.01f * multiplier * a_m;
    }
}

void tervik_vale_vilkumine(bool &vale_vastus_vilkumine, bool &opacity_up, float &whole_scene_opacity, float &a_m) {
	if (vale_vastus_vilkumine) {
		if (opacity_up)
            vilkumine(whole_scene_opacity, "up", 20, a_m);
        if (whole_scene_opacity >= 0.75f) {
        	opacity_up = false;
        }
        if (!opacity_up) {
            vilkumine(whole_scene_opacity, "down", 1.1, a_m);
        }
        if (whole_scene_opacity <= 0) {
        	whole_scene_opacity = 0.0f;
            vale_vastus_vilkumine = false;
            opacity_up = true;
		}
	}
	
}

bool lastCharIsMultibyte(const std::string& s) {
    if (s.empty()) return false;

    // Walk backwards to find the start of the last UTF-8 character
    int i = s.size() - 1;
    // Move left while the byte is a UTF-8 continuation byte (10xxxxxx)
    while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80)
        --i;

    // Length of last character in bytes
    size_t charLen = s.size() - i;

    // If more than one byte, it's a multibyte character (non-ASCII)
    return charLen > 1;
}

void vale_vastus_muutujad(std::string &kombinatsiooni_tekst, std::vector<std::string> &kombinatsiooni_sõnad, bool &sõnad_push_back_done, float &aeg, int &elud, bool &vale_vastus_vilkumine) {
		
	elud--;
	kombinatsiooni_tekst = sõnavahetus();
	kombinatsiooni_sõnad.clear();
	sõnad_push_back_done = false;
	if (elud != 0)
		vale_vastus_vilkumine = true;
	aeg = 0;	
}

void vale_vastus_muutujad(std::string &kombinatsiooni_tekst, std::vector<std::string> &kombinatsiooni_sõnad, bool &sõnad_push_back_done, float &aeg) {	
	kombinatsiooni_tekst = sõnavahetus();
	kombinatsiooni_sõnad.clear();
	sõnad_push_back_done = false;
	aeg = 0;	
}

void renderSquare(SDL_Renderer* renderer, float x, float y, float size, 
                  int r, int g, int b, float opacity = 1.0f) {
    Uint8 alpha = static_cast<Uint8>(opacity * 255.0f);
    SDL_SetRenderDrawColor(renderer, r, g, b, alpha);
    SDL_FRect rect = {x, y, size, size};
    SDL_RenderFillRect(renderer, &rect);
}

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

bool renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text,
                float x, float y, SDL_Color color, TextCache* cache = nullptr) {
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
                //std::cerr << "TTF_RenderText_Blended failed: " << SDL_GetError() << std::endl;
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

std::map<int, TTF_Font*> fontCache;

TTF_Font* getFont(const std::string& fontPath, int size) {
    // Check if font at this size is already loaded
    if (fontCache.find(size) != fontCache.end()) {
        return fontCache[size];
    }
    
    // Load new font size
    TTF_Font* font = TTF_OpenFont(fontPath.c_str(), size);
    if (font) {
        fontCache[size] = font;
    }
    return font;
}

void clearFontCache() {
    for (auto& pair : fontCache) {
        TTF_CloseFont(pair.second);
    }
    fontCache.clear();
}

bool getTextSize(TTF_Font* font, const std::string& text, float& width, float& height) {
    int w, h;
    if (!TTF_GetStringSize(font, text.c_str(), 0, &w, &h)) {
        std::cerr << "TTF_GetStringSize failed: " << SDL_GetError() << std::endl;
        return false;
    }
    width = static_cast<float>(w);
    height = static_cast<float>(h);
    return true;
}

std::string kombinatsiooni_tekst = sõnavahetus();
std::string sisendi_tekst = "";
std::string elu_tekst = "<3";
std::string aeg_tekst = "";
std::string skoor_tekst = "Skoor: 0";
std::string skoor_lõpp_tekst = "";

bool text_entered = false;

void processInput(SDL_Event event, bool& running, bool mäng_läbi) {
	if (event.type == SDL_EVENT_KEY_DOWN) {
	
        if (event.key.key == SDLK_ESCAPE) {
            running = false;
        }
        
        if (event.key.key == SDLK_BACKSPACE && !sisendi_tekst.empty()) {
            int n = 1;
        	
            if (!sisendi_tekst.empty()) {
            	if (lastCharIsMultibyte(sisendi_tekst))
            		n = 2;
            	sisendi_tekst.erase(sisendi_tekst.length() - n);
            }
		}
		if (event.key.key == SDLK_RETURN) {
			text_entered = true;
		}		
	}
    
    else if (event.type == SDL_EVENT_TEXT_INPUT && !mäng_läbi) {
    	std::string input = event.text.text;
    
		// Simple uppercase conversion (handles ASCII and some UTF-8)
		for (size_t i = 0; i < input.length(); i++) {
		    unsigned char c = input[i];
		    
		    // Handle ASCII letters
		    if (c >= 'a' && c <= 'z') {
		        input[i] = c - 32;
		    }
		    // Handle Estonian specific UTF-8 characters
		    else if (i + 1 < input.length()) {
		        // ä (C3 A4) -> Ä (C3 84)
		        if ((unsigned char)input[i] == 0xC3 && (unsigned char)input[i+1] == 0xA4) {
		            input[i+1] = 0x84;
		        }
		        // ö (C3 B6) -> Ö (C3 96)
		        else if ((unsigned char)input[i] == 0xC3 && (unsigned char)input[i+1] == 0xB6) {
		            input[i+1] = 0x96;
		        }
		        // ü (C3 BC) -> Ü (C3 9C)
		        else if ((unsigned char)input[i] == 0xC3 && (unsigned char)input[i+1] == 0xBC) {
		            input[i+1] = 0x9C;
		        }
		        // õ (C3 B5) -> Õ (C3 95)
		        else if ((unsigned char)input[i] == 0xC3 && (unsigned char)input[i+1] == 0xB5) {
		            input[i+1] = 0x95;
		        }
		        // ž (C5 BE) -> Ž (C5 BD)
		        else if ((unsigned char)input[i] == 0xC5 && (unsigned char)input[i+1] == 0xBE) {
		            input[i+1] = 0xBD;
		        }
		        // š (C5 A1) -> Š (C5 A0)
		        else if ((unsigned char)input[i] == 0xC5 && (unsigned char)input[i+1] == 0xA1) {
		            input[i+1] = 0xA0;
		        }
		    }
		}
		
		// Filter: only accept letters (A-Z and special characters)
		bool only_letters = true;
		for (size_t i = 0; i < input.length(); i++) {
		    unsigned char c = input[i];
		    
		    // Check if it's an ASCII letter (A-Z)
		    if (c >= 'A' && c <= 'Z') {
		        continue;
		    }
		    // Check if it's part of a UTF-8 multibyte sequence (for äöüõšž etc.)
		    else if (c >= 0xC0) {
		        continue;
		    }
		    else if (c >= 0x80 && c < 0xC0) {
		        // Continuation byte, part of multibyte character
		        continue;
		    }
		    else {
		        // Not a letter (number, symbol, space, etc.)
		        only_letters = false;
		        break;
		    }
		}
		
		// Only add if all characters are letters
		if (only_letters && !input.empty()) {
		    sisendi_tekst += input;
		}
	}
}

int teksti_nihe = 0;
float whole_scene_opacity = 0;

void Render(SDL_Renderer* renderer, SDL_Window* window, TTF_Font* font, TextCache textCache) {

	SDL_SetRenderDrawColor(renderer, 0, 0, 55, 255);
	SDL_RenderClear(renderer);	
	
	renderText(renderer, getFont("Gentium-R.ttf", 100), kombinatsiooni_tekst, 230, 160, {255, 255, 255, 255}, &textCache);
	renderText(renderer, getFont("Gentium-R.ttf", 100), sisendi_tekst, 230-teksti_nihe, 300, {255, 255, 0, 255}, &textCache);
	renderText(renderer, getFont("Gentium-R.ttf", 50), elu_tekst, 20, 0, {255, 0, 0, 100}, &textCache);
	renderText(renderer, getFont("Gentium-R.ttf", 50), aeg_tekst, 305, 420, {255, 255, 255, 100}, &textCache);
	renderText(renderer, getFont("Gentium-R.ttf", 50), skoor_tekst, 450, 0, {255, 255, 255, 100}, &textCache);
	
	
	renderSquare(renderer, 0, 0, 700, 255, 0, 0, whole_scene_opacity);
	renderText(renderer, getFont("Gentium-R.ttf", 100), skoor_lõpp_tekst, 60, 160, {255, 255, 255, 255}, &textCache);
	
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	SDL_RenderPresent(renderer);
}

int main() {

	if (!SDL_Init(SDL_INIT_VIDEO)) {
		SDL_Log("Failed");
		return -1;
	}
	
	if (!TTF_Init()) {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }
	
	SDL_Window* window;
	window = SDL_CreateWindow("Sonamäng", 640, 480, 0);	
	
	SDL_StartTextInput(window);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
	if (!renderer) {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    
    if (!SDL_SetRenderVSync(renderer, 1)) {
    	std::cerr << "Warning: VSync not supported: " << SDL_GetError() << std::endl;
	}
       
    TTF_Font* font = TTF_OpenFont("Gentium-R.ttf", 100);
    TTF_Font* font_smaller = TTF_OpenFont("Gentium-R.ttf", 50);
    
    if (!font) {
        std::cerr << "TTF_OpenFont failed: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        TTF_Quit();
        SDL_Quit();
        return 1;
    }
    
    TextCache textCache;       
	SDL_Event event;	
	
	int elud = 3;
    bool vastus = true;
    std::vector<std::string> kombinatsiooni_sõnad;
    std::string vaadeldav_sõna;
    while (std::getline(sonade_list, sõna)) {
    	sõnad.push_back(sõna);
    }
    bool sõnad_push_back_done = false;
    
    int skoor = 0;
    
    
    float aeg = 0;
    float vastupidine_aeg = 0;
    int aeg_int;
    int sekundite_arv = 10;
	
	bool opacity_up = true;
    bool vale_vastus_vilkumine = false;
	
	bool running = true;
	
	Uint64 lastTime = SDL_GetTicks();
	float deltaTime = 0.0f;
	bool mäng_läbi = false;
	
	while (running) {
		
		SDL_Delay(1);
		
		// Time
		Uint64 currentTime = SDL_GetTicks();
		deltaTime = (currentTime - lastTime) / 1000.0f;
		lastTime = currentTime;
		
		float a_m = deltaTime * 62.5;
		
		std::cout << whole_scene_opacity << std::endl;
		whole_scene_opacity += 0.01f * a_m;
		
		aeg = aeg + 0.0166 * a_m;
    	vastupidine_aeg = (sekundite_arv+1) - aeg;
    	aeg_int = static_cast<int>(vastupidine_aeg);
    	aeg_tekst = std::to_string(aeg_int);
		
		elu_tekst = std::to_string(elud);
		
		
		
		while(SDL_PollEvent(&event)) {
					
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			processInput(event, running, mäng_läbi);				
		}		
		
		// Mäng
		
		if (aeg_int <= 0) {
			vale_vastus_muutujad(kombinatsiooni_tekst, kombinatsiooni_sõnad, sõnad_push_back_done, aeg, elud, vale_vastus_vilkumine);
			
			sisendi_tekst = "";
		}
		
		if (!sõnad_push_back_done) {
            for (auto i : sõnad) {
                size_t pos = i.find(kombinatsiooni_tekst);
                if (pos != std::string::npos) {
                    kombinatsiooni_sõnad.push_back(i);
                }
            }
            sõnad_push_back_done = true;
            näita_sõnu(kombinatsiooni_sõnad);
        }
		
		if (text_entered) {
			text_entered = false;
			vastus = false;
			for (auto i : kombinatsiooni_sõnad) {
                if (i == sisendi_tekst) {
                    sõnad.erase(std::remove(sõnad.begin(), sõnad.end(), sisendi_tekst), sõnad.end());
                    vale_vastus_muutujad(kombinatsiooni_tekst, kombinatsiooni_sõnad, sõnad_push_back_done, aeg);        
                    vastus = true;
                    skoor++;
                    skoor_tekst = "Skoor: " + std::to_string(skoor);
					
                    break;
                }
               
            }
            if (sisendi_tekst != "" && !vastus) {
            	vale_vastus_muutujad(kombinatsiooni_tekst, kombinatsiooni_sõnad, sõnad_push_back_done, aeg, elud, vale_vastus_vilkumine);
            }
            sisendi_tekst = "";
		}
		
		if (elud == 0) {
            vilkumine(whole_scene_opacity, "up", 2, a_m);
            skoor_lõpp_tekst = "Sinu skoor: " + std::to_string(skoor);
            mäng_läbi = true;
        }
		//std::cout << mäng_läbi << std::endl;
		
		tervik_vale_vilkumine(vale_vastus_vilkumine, opacity_up, whole_scene_opacity, a_m);
		
		// Teksti kontroll
		float textWidth, textHeight;
		if (getTextSize(font, sisendi_tekst, textWidth, textHeight)) {			
			
			if (textWidth > 400+teksti_nihe) {
				
				teksti_nihe += 60;
			}
			if (textWidth < 400) {
				teksti_nihe = 0;
			}
		}
		
		// Render
		Render(renderer, window, font, textCache);

	}
	
    //TTF_CloseFont(font);
    clearFontCache();
    
	SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
	SDL_Quit();

	return 0;
}
