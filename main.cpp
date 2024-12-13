#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

#ifdef _WIN32
#undef main // Undefine main if defined by SDL
#endif

// Screen dimensions
const int SCREEN_WIDTH = 580;
const int SCREEN_HEIGHT = 720;

// Speed constants
int ENEMY_SPEED = 2; // Enemy movement speed

// Global variables
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
SDL_Texture* carTexture = nullptr;
SDL_Texture* enemyTexture = nullptr;
SDL_Texture* trackTexture = nullptr;
TTF_Font* font = nullptr;
int carX = SCREEN_WIDTH / 2 - 20; // Initial car position (centered horizontally)
int carY = (3 * SCREEN_HEIGHT / 4); // Initial car position (3/4 down the screen)
int carSpeed = 5; // Car movement speed
std::vector<int> enemyX;
std::vector<int> enemyY;
std::vector<bool> enemyFlag;
int score = 0;
bool moveLeft = false;
bool moveRight = false;
Uint32 lastSpeedIncreaseTime = 0; // Time of the last speed increase
int trackOffsetY = 0; // Offset for track movement

// Global state variables
enum GameState { WAITING, PLAYING, GAME_OVER };
GameState gameState = WAITING;

// Function declarations
bool initSDL();
void closeSDL();
bool loadTextures();
bool loadFont();
void drawTrack();
void drawCar();
void drawEnemy(int ind);
void updateScore();
bool checkCollision(SDL_Rect carRect, SDL_Rect enemyRect);
void createEnemies(int numEnemies);
void drawWaitingScreen();
void drawGameOverScreen();
void resetGame();

int main(int argc, char* args[]) {
    if (!initSDL()) return -1;
    if (!loadTextures() || !loadFont()) return -1;

    bool quit = false;
    SDL_Event e;
    srand(static_cast<unsigned>(time(0))); // Initialize random seed

    while (!quit) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) quit = true;

            // Handle key press events
            if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_o: // Press O to start the game
                        if (gameState == WAITING) {
                            gameState = PLAYING; // Start the game
                            createEnemies(6); // Create 6 enemies
                            lastSpeedIncreaseTime = SDL_GetTicks(); // Reset speed increase time
                            score = 0; // Reset score
                        }
                        break;

                    case SDLK_a: // Move left
                        if (gameState == PLAYING) moveLeft = true;
                        break;
                    case SDLK_d: // Move right
                        if (gameState == PLAYING) moveRight = true;
                        break;

                    case SDLK_ESCAPE: // Press ESC to quit
                        if (gameState == GAME_OVER) quit = true;
                        break;

                    case SDLK_r: // Press R to restart the game
                        if (gameState == GAME_OVER) {
                            resetGame(); // Reset the game
                            gameState = PLAYING; // Start the game
                        }
                        break;

                    case SDLK_q: // Press Q to quit the game
                        if (gameState == GAME_OVER) quit = true;
                        break;
                }
            }

            if (e.type == SDL_KEYUP) {
                switch (e.key.keysym.sym) {
                    case SDLK_a:
                        moveLeft = false;
                        break;
                    case SDLK_d:
                        moveRight = false;
                        break;
                }
            }
        }

        // Game logic depending on current game state
        if (gameState == WAITING) {
            drawWaitingScreen(); // Draw waiting screen
        } else if (gameState == PLAYING) {
            // Handle gameplay logic here
            if (moveLeft && carX > 0) carX -= carSpeed;
            if (moveRight && carX < SCREEN_WIDTH - 100) carX += carSpeed;

            // Increase enemy speed every 5 seconds
            Uint32 currentTime = SDL_GetTicks();
            if (currentTime - lastSpeedIncreaseTime >= 5000) {
                ENEMY_SPEED++;
                lastSpeedIncreaseTime = currentTime;
            }

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Clear screen
            SDL_RenderClear(renderer);

            // Move track
            trackOffsetY += ENEMY_SPEED;
            if (trackOffsetY >= SCREEN_HEIGHT) {
                trackOffsetY = 0; // Reset when it has scrolled the entire screen
            }

            drawTrack();
            drawCar();

            // Update and draw enemies
            for (int i = 0; i < enemyX.size(); ++i) {
                if (enemyFlag[i]) {
                    enemyY[i] += ENEMY_SPEED;
                    if (enemyY[i] > SCREEN_HEIGHT) {
                        enemyY[i] = -40; // Reset enemy above the screen
                        enemyX[i] = 20 + (i % 3) * 200 + rand() % 100; // Random position within each lane
                        score++; // Increase score when passing an enemy
                    }
                    drawEnemy(i);

                    // Check collision
                    SDL_Rect carRect = {carX, carY, 70, 70};
                    SDL_Rect enemyRect = {enemyX[i], enemyY[i], 40, 40};
                    if (checkCollision(carRect, enemyRect)) {
                        gameState = GAME_OVER; // End the game if a collision occurs
                    }
                }
            }

            updateScore();
        } else if (gameState == GAME_OVER) {
            drawGameOverScreen(); // Draw the game over screen
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(15); // Delay for 15 milliseconds
    }

    closeSDL();
    return 0;
}

bool initSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("Car Racing Game",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              SCREEN_WIDTH, SCREEN_HEIGHT,
                              SDL_WINDOW_SHOWN);
    if (!window) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }

    if (TTF_Init() == -1) {
        printf("SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }

    return true;
}

void closeSDL() {
    SDL_DestroyTexture(carTexture);
    SDL_DestroyTexture(enemyTexture);
    SDL_DestroyTexture(trackTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    TTF_Quit();
    SDL_Quit();
}

bool loadTextures() {
    SDL_Surface* surface = IMG_Load("./car2.png"); // Path to car image
    if (!surface) {
        printf("Failed to load car image! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }
    carTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("./barrier.png"); // Path to enemy image
    if (!surface) {
        printf("Failed to load enemy image! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }
    enemyTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    surface = IMG_Load("road.png"); // Path to track image
    if (!surface) {
        printf("Failed to load track image! SDL_image Error: %s\n", IMG_GetError());
        return false;
    }
    trackTexture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    return true;
}

bool loadFont() {
    font = TTF_OpenFont("./fontGame.ttf", 24); // Path to font
    if (!font) {
        printf("Failed to load font! SDL_ttf Error: %s\n", TTF_GetError());
        return false;
    }
    return true;
}

void drawTrack() {
    SDL_Rect trackRect1 = {0, trackOffsetY, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_Rect trackRect2 = {0, trackOffsetY - SCREEN_HEIGHT, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(renderer, trackTexture, NULL, &trackRect1);
    SDL_RenderCopy(renderer, trackTexture, NULL, &trackRect2);
}

void drawCar() {
    SDL_Rect carRect = {carX, carY, 70, 70}; // Position the car
    SDL_RenderCopy(renderer, carTexture, NULL, &carRect);
}

void drawEnemy(int ind) {
    SDL_Rect enemyRect = {enemyX[ind], enemyY[ind], 40, 40};
    SDL_RenderCopy(renderer, enemyTexture, NULL, &enemyRect);
}

void updateScore() {
    SDL_Color white = {255, 255, 255};
    SDL_Surface* textSurface = TTF_RenderText_Solid(font, ("Score: " + std::to_string(score)).c_str(), white);
    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    SDL_Rect textRect = {10, 10, textSurface->w, textSurface->h};

    SDL_RenderCopy(renderer, textTexture, NULL, &textRect);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

bool checkCollision(SDL_Rect carRect, SDL_Rect enemyRect) {
    return SDL_HasIntersection(&carRect, &enemyRect);
}

void createEnemies(int numEnemies) {
    for (int i = 0; i < numEnemies; ++i) {
        enemyX.push_back(20 + (i % 3) * 200 + rand() % 100); // Random position within each lane
        enemyY.push_back(-40 - (i * 100)); // Staggered start positions
        enemyFlag.push_back(true);
    }
}

void drawWaitingScreen() {
    SDL_Color white = {255, 255, 255};

    // Chia thông điệp thành các dòng riêng biệt
    std::string message1 = "Press O to Start";
    std::string message2 = "Press 'A' or 'D' to go left or right";
    
    // Tạo và vẽ dòng đầu tiên
    SDL_Surface* textSurface1 = TTF_RenderText_Solid(font, message1.c_str(), white);
    SDL_Texture* textTexture1 = SDL_CreateTextureFromSurface(renderer, textSurface1);
    SDL_Rect textRect1 = {SCREEN_WIDTH / 2 - textSurface1->w / 2, SCREEN_HEIGHT / 2 - textSurface1->h / 2, textSurface1->w, textSurface1->h};
    SDL_RenderCopy(renderer, textTexture1, NULL, &textRect1);
    
    // Tạo và vẽ dòng thứ hai
    SDL_Surface* textSurface2 = TTF_RenderText_Solid(font, message2.c_str(), white);
    SDL_Texture* textTexture2 = SDL_CreateTextureFromSurface(renderer, textSurface2);
    SDL_Rect textRect2 = {SCREEN_WIDTH / 2 - textSurface2->w / 2, SCREEN_HEIGHT / 2 + textSurface1->h / 2 + 10, textSurface2->w, textSurface2->h};
    SDL_RenderCopy(renderer, textTexture2, NULL, &textRect2);

    // Giải phóng tài nguyên
    SDL_FreeSurface(textSurface1);
    SDL_DestroyTexture(textTexture1);
    SDL_FreeSurface(textSurface2);
    SDL_DestroyTexture(textTexture2);
}


void drawGameOverScreen() {
    SDL_Color white = {255, 255, 255};
    std::string message1 = "Game Over";
    std::string message2 = "Score: " + std::to_string(score);
    std::string message3 = "Press R to Play Again or Q to Quit";

    SDL_Surface* textSurface1 = TTF_RenderText_Solid(font, message1.c_str(), white);
    SDL_Texture* textTexture1 = SDL_CreateTextureFromSurface(renderer, textSurface1);
    SDL_Rect textRect1 = {SCREEN_WIDTH / 2 - textSurface1->w / 2, SCREEN_HEIGHT / 2 - textSurface1->h / 2 - 40, textSurface1->w, textSurface1->h};

    SDL_Surface* textSurface2 = TTF_RenderText_Solid(font, message2.c_str(), white);
    SDL_Texture* textTexture2 = SDL_CreateTextureFromSurface(renderer, textSurface2);
    SDL_Rect textRect2 = {SCREEN_WIDTH / 2 - textSurface2->w / 2, SCREEN_HEIGHT / 2 - textSurface2->h / 2, textSurface2->w, textSurface2->h};

    SDL_Surface* textSurface3 = TTF_RenderText_Solid(font, message3.c_str(), white);
    SDL_Texture* textTexture3 = SDL_CreateTextureFromSurface(renderer, textSurface3);
    SDL_Rect textRect3 = {SCREEN_WIDTH / 2 - textSurface3->w / 2, SCREEN_HEIGHT / 2 - textSurface3->h / 2 + 40, textSurface3->w, textSurface3->h};

    SDL_RenderCopy(renderer, textTexture1, NULL, &textRect1);
    SDL_RenderCopy(renderer, textTexture2, NULL, &textRect2);
    SDL_RenderCopy(renderer, textTexture3, NULL, &textRect3);

    SDL_FreeSurface(textSurface1);
    SDL_DestroyTexture(textTexture1);
    SDL_FreeSurface(textSurface2);
    SDL_DestroyTexture(textTexture2);
    SDL_FreeSurface(textSurface3);
    SDL_DestroyTexture(textTexture3);
}

void resetGame() {
    carX = SCREEN_WIDTH / 2 - 20; // Reset car position
    carY = (3 * SCREEN_HEIGHT / 4); // Reset car position
    ENEMY_SPEED = 2; // Reset enemy speed
    enemyX.clear();
    enemyY.clear();
    enemyFlag.clear();
    createEnemies(6); // Create 6 enemies
    lastSpeedIncreaseTime = SDL_GetTicks(); // Reset speed increase time
    score = 0; // Reset score
}