
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

const int WINDOW_WIDTH = 500;
const int WINDOW_HEIGHT = 700;
const int COLUMN_WIDTH = 80;
const int DOT_SIZE = 50;

const float SPEED_INCREASE_RATE = 0.1f; // Speed increase per level
const int SPAWN_DECREASE_RATE = 5;      // Spawn interval decrease per level
const int POINTS_PER_LEVEL = 100;       // Points needed for next level

const int BUTTON_WIDTH = 120;
const int BUTTON_HEIGHT = 80;
const int BUTTON_Y = 600;

enum Color { RED = 0, GREEN = 1, BLUE = 2 };

struct Dot {
  float x, y;
  Color color;
  bool active;
  float speed; // Individual speed (for difficulty variation)

  Dot(float posX, float posY, Color c, float spd)
      : x(posX), y(posY), color(c), active(true), speed(spd) {}

  void move() { y += speed; }

  bool reachedBottom() { return y > (BUTTON_Y - DOT_SIZE - 10); }
};

class Game {
private:
  SDL_Window *window;
  SDL_Renderer *renderer;
  TTF_Font *font;
  TTF_Font *titleFont;
  TTF_Font *smallFont;
  bool running;

  Mix_Music *bgMusic;
  Mix_Chunk *correctSound;
  Mix_Chunk *wrongSound;
  Mix_Chunk *missSound;
  Mix_Chunk *levelUpSound;

  std::vector<Dot> dots;
  int frameCount;
  int score;
  int highScore;
  bool gameOver;
  bool paused;

  int level;
  float currentSpeed;
  int currentSpawnInterval;
  int lastLevelScore;
  bool showLevelUp;
  int levelUpTimer;

  std::vector<Color> colorPattern;
  int patternIndex;
  bool usePattern;

  bool buttonPressed[3] = {false, false, false};
  int buttonPressTimer[3] = {0, 0, 0};

  Color getRandomColor() { return static_cast<Color>(rand() % 3); }

  void generatePattern() {
    colorPattern.clear();
    patternIndex = 0;

    int patternLength = 3 + (level / 3); // Longer patterns at higher levels

    if (level < 3) {
      for (int i = 0; i < patternLength; i++) {
        colorPattern.push_back(getRandomColor());
      }
    } else if (level < 6) {
      Color first = getRandomColor();
      Color second = getRandomColor();
      for (int i = 0; i < patternLength; i++) {
        colorPattern.push_back(i % 2 == 0 ? first : second);
      }
    } else {
      for (int i = 0; i < patternLength; i++) {
        if (i % 3 == 0)
          colorPattern.push_back(RED);
        else if (i % 3 == 1)
          colorPattern.push_back(GREEN);
        else
          colorPattern.push_back(BLUE);
      }
      if (rand() % 2 == 0) {
        std::reverse(colorPattern.begin(), colorPattern.end());
      }
    }

    usePattern =
        (level >= 2 &&
         rand() % 100 < 30 + level * 5); // Increase pattern probability
  }

  Color getNextColor() {
    if (usePattern && !colorPattern.empty()) {
      Color c = colorPattern[patternIndex];
      patternIndex++;
      if (patternIndex >= colorPattern.size()) {
        patternIndex = 0;
        if (rand() % 100 < 40) {
          generatePattern();
        }
      }
      return c;
    }
    return getRandomColor();
  }

  void updateDifficulty() {
    int newLevel = 1 + (score / POINTS_PER_LEVEL);

    if (newLevel > level) {
      level = newLevel;
      lastLevelScore = score;

      currentSpeed = 2.0f + (level - 1) * SPEED_INCREASE_RATE;

      currentSpawnInterval = 120 - (level - 1) * SPAWN_DECREASE_RATE;
      if (currentSpawnInterval < 40)
        currentSpawnInterval = 40; // Minimum limit

      showLevelUp = true;
      levelUpTimer = 120; // Show for 2 seconds

      if (levelUpSound) {
        Mix_PlayChannel(-1, levelUpSound, 0);
      }

      generatePattern();

      std::cout << "🎉 LEVEL UP! Now Level " << level << std::endl;
      std::cout << "   Speed: " << currentSpeed
                << " | Spawn Rate: " << currentSpawnInterval << std::endl;
    }
  }

  void drawDot(const Dot &dot) {
    int alpha = 255;
    if (dot.speed > 3.0f) {
      alpha = 200 + (int)(55 * sin(frameCount * 0.1f));
    }

    switch (dot.color) {
    case RED:
      SDL_SetRenderDrawColor(renderer, 255, 50, 50, alpha);
      break;
    case GREEN:
      SDL_SetRenderDrawColor(renderer, 50, 255, 50, alpha);
      break;
    case BLUE:
      SDL_SetRenderDrawColor(renderer, 50, 100, 255, alpha);
      break;
    }

    SDL_Rect rect = {static_cast<int>(dot.x), static_cast<int>(dot.y), DOT_SIZE,
                     DOT_SIZE};
    SDL_RenderFillRect(renderer, &rect);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    if (dot.speed > 3.5f) {
      SDL_RenderDrawRect(renderer, &rect);
      SDL_Rect innerRect = {rect.x + 2, rect.y + 2, rect.w - 4, rect.h - 4};
      SDL_RenderDrawRect(renderer, &innerRect);
    } else {
      SDL_RenderDrawRect(renderer, &rect);
    }
  }

  void drawColumn() {
    int intensity = 80 + (level * 5);
    if (intensity > 120)
      intensity = 120;

    SDL_SetRenderDrawColor(renderer, intensity, intensity, intensity + 10, 255);
    SDL_Rect column = {WINDOW_WIDTH / 2 - COLUMN_WIDTH / 2, 0, COLUMN_WIDTH,
                       BUTTON_Y};
    SDL_RenderFillRect(renderer, &column);

    SDL_SetRenderDrawColor(renderer, 150 + level * 5, 150 + level * 5,
                           160 + level * 5, 255);
    SDL_RenderDrawRect(renderer, &column);
  }

  void drawButtons() {
    const char *labels[] = {"R", "G", "B"};
    SDL_Color colors[] = {
        {255, 50, 50, 255}, // Red
        {50, 255, 50, 255}, // Green
        {50, 100, 255, 255} // Blue
    };

    for (int i = 0; i < 3; i++) {
      int x = 40 + i * (BUTTON_WIDTH + 20);

      if (buttonPressed[i]) {
        SDL_SetRenderDrawColor(renderer, colors[i].r, colors[i].g, colors[i].b,
                               255);
      } else {
        SDL_SetRenderDrawColor(renderer, colors[i].r * 0.7, colors[i].g * 0.7,
                               colors[i].b * 0.7, 255);
      }

      SDL_Rect button = {x, BUTTON_Y, BUTTON_WIDTH, BUTTON_HEIGHT};
      SDL_RenderFillRect(renderer, &button);

      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_RenderDrawRect(renderer, &button);

      if (titleFont) {
        SDL_Surface *surface =
            TTF_RenderText_Solid(titleFont, labels[i], {255, 255, 255, 255});
        if (surface) {
          SDL_Texture *texture =
              SDL_CreateTextureFromSurface(renderer, surface);
          SDL_Rect textRect = {x + BUTTON_WIDTH / 2 - surface->w / 2,
                               BUTTON_Y + BUTTON_HEIGHT / 2 - surface->h / 2,
                               surface->w, surface->h};
          SDL_RenderCopy(renderer, texture, nullptr, &textRect);
          SDL_DestroyTexture(texture);
          SDL_FreeSurface(surface);
        }
      }
    }
  }

  void renderText(const std::string &text, int x, int y, TTF_Font *useFont,
                  SDL_Color color = {255, 255, 255, 255}) {
    if (!useFont)
      return;

    SDL_Surface *surface = TTF_RenderText_Solid(useFont, text.c_str(), color);
    if (surface) {
      SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
      SDL_Rect rect = {x, y, surface->w, surface->h};
      SDL_RenderCopy(renderer, texture, nullptr, &rect);
      SDL_DestroyTexture(texture);
      SDL_FreeSurface(surface);
    }
  }

  void drawUI() {
    renderText("RGB GUARDIAN", 10, 10, titleFont, {255, 255, 100, 255});

    renderText("Score: " + std::to_string(score), 10, 50, font);

    renderText("Best: " + std::to_string(highScore), 10, 75, font);

    SDL_Color levelColor = {100, 255, 255, 255};
    if (level > 5)
      levelColor = {255, 150, 50, 255}; // Orange for high levels
    if (level > 10)
      levelColor = {255, 50, 50, 255}; // Red for very high levels

    renderText("Level: " + std::to_string(level), 10, 100, font, levelColor);

    if (smallFont) {
      std::string speedText =
          "Speed: x" + std::to_string(currentSpeed).substr(0, 3);
      renderText(speedText, 10, 125, smallFont, {200, 200, 200, 255});
    }

    if (paused) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
      SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
      SDL_RenderFillRect(renderer, &overlay);

      SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
      SDL_Rect pauseBox = {WINDOW_WIDTH / 2 - 120, WINDOW_HEIGHT / 2 - 80, 240,
                           160};
      SDL_RenderFillRect(renderer, &pauseBox);

      SDL_SetRenderDrawColor(renderer, 100, 100, 255, 255);
      SDL_Rect pauseBoxBorder = {WINDOW_WIDTH / 2 - 125, WINDOW_HEIGHT / 2 - 85,
                                 250, 170};
      SDL_RenderDrawRect(renderer, &pauseBoxBorder);

      renderText("PAUSED", WINDOW_WIDTH / 2 - 60, WINDOW_HEIGHT / 2 - 50,
                 titleFont, {0, 0, 0, 255});

      if (smallFont) {
        renderText("Press P to resume", WINDOW_WIDTH / 2 - 75,
                   WINDOW_HEIGHT / 2 + 10, font, {50, 50, 50, 255});
        renderText("ESC to quit", WINDOW_WIDTH / 2 - 50, WINDOW_HEIGHT / 2 + 40,
                   smallFont, {100, 100, 100, 255});
      }

      return; // Don't draw other UI elements when paused
    }

    if (showLevelUp && levelUpTimer > 0) {
      int alpha = (levelUpTimer > 60) ? 255 : (levelUpTimer * 4);
      SDL_SetRenderDrawColor(renderer, 255, 215, 0, alpha * 0.8);
      SDL_Rect banner = {50, WINDOW_HEIGHT / 2 - 40, WINDOW_WIDTH - 100, 80};
      SDL_RenderFillRect(renderer, &banner);

      SDL_SetRenderDrawColor(renderer, 255, 255, 255, alpha);
      SDL_RenderDrawRect(renderer, &banner);

      std::string levelText = "LEVEL " + std::to_string(level) + "!";
      renderText(levelText, WINDOW_WIDTH / 2 - 60, WINDOW_HEIGHT / 2 - 20,
                 titleFont, {255, 255, 255, static_cast<Uint8>(alpha)});

      if (smallFont) {
        renderText("Difficulty Increased!", WINDOW_WIDTH / 2 - 80,
                   WINDOW_HEIGHT / 2 + 15, smallFont,
                   {255, 255, 255, static_cast<Uint8>(alpha)});
      }

      levelUpTimer--;
      if (levelUpTimer == 0) {
        showLevelUp = false;
      }
    }

    if (frameCount < 300 && !gameOver && level == 1) {
      renderText("Press R, G, or B keys!", WINDOW_WIDTH / 2 - 100,
                 BUTTON_Y - 40, font, {200, 200, 255, 255});
    }

    if (frameCount > 60 && frameCount < 240 && !gameOver && !paused &&
        smallFont) {
      renderText("Press P to pause", WINDOW_WIDTH - 140, 10, smallFont,
                 {150, 150, 200, 200});
    }

    if (usePattern && level >= 3 && smallFont) {
      renderText("Pattern Mode!", WINDOW_WIDTH / 2 - 50, BUTTON_Y - 40,
                 smallFont, {255, 200, 100, 255});
    }
  }

  void handleKeyPress(Color pressedColor) {
    if (gameOver || paused)
      return;

    buttonPressed[pressedColor] = true;
    buttonPressTimer[pressedColor] = 10;

    Dot *targetDot = nullptr;
    float lowestY = -1;

    for (auto &dot : dots) {
      if (dot.active && dot.y > lowestY) {
        lowestY = dot.y;
        targetDot = &dot;
      }
    }

    if (targetDot != nullptr) {
      if (targetDot->color == pressedColor) {
        targetDot->active = false;

        int points = 10;
        if (targetDot->speed > 3.5f)
          points = 20;
        else if (targetDot->speed > 2.5f)
          points = 15;

        score += points;
        if (score > highScore)
          highScore = score;

        Mix_PlayChannel(-1, correctSound, 0);

        if (points > 10) {
          std::cout << "✓ Perfect! +" << points << " points (Fast dot bonus!)"
                    << std::endl;
        } else {
          std::cout << "✓ Correct! Score: " << score << std::endl;
        }

        updateDifficulty();
      } else {
        Mix_PlayChannel(-1, wrongSound, 0);
        std::cout << "✗ Wrong color! Game Over! Final Level: " << level
                  << std::endl;
        gameOver = true;
        Mix_HaltMusic();
      }
    }
  }

public:
  Game()
      : window(nullptr), renderer(nullptr), font(nullptr), titleFont(nullptr),
        smallFont(nullptr), bgMusic(nullptr), correctSound(nullptr),
        wrongSound(nullptr), missSound(nullptr), levelUpSound(nullptr),
        running(true), frameCount(0), score(0), highScore(0), gameOver(false),
        paused(false), level(1), currentSpeed(2.0f), currentSpawnInterval(120),
        lastLevelScore(0), showLevelUp(false), levelUpTimer(0), patternIndex(0),
        usePattern(false) {
    srand(static_cast<unsigned>(time(nullptr)));
  }

  bool init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
      std::cerr << "SDL Init Error: " << SDL_GetError() << std::endl;
      return false;
    }

    if (TTF_Init() < 0) {
      std::cerr << "TTF Init Error: " << TTF_GetError() << std::endl;
      return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
      std::cerr << "Mixer Init Error: " << Mix_GetError() << std::endl;
      return false;
    }

    Mix_AllocateChannels(16);

    window = SDL_CreateWindow("RGB Guardian - Progressive Difficulty",
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);

    if (!window) {
      std::cerr << "Window Error: " << SDL_GetError() << std::endl;
      return false;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
      std::cerr << "Renderer Error: " << SDL_GetError() << std::endl;
      return false;
    }

    font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);
    titleFont = TTF_OpenFont(
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 32);
    smallFont =
        TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);

    if (!font || !titleFont) {
      std::cerr << "Font loading failed. Using fallback..." << std::endl;
      font = TTF_OpenFont(
          "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
          20);
      titleFont = TTF_OpenFont(
          "/usr/share/fonts/truetype/liberation/LiberationSans-Bold.ttf", 32);
      smallFont = TTF_OpenFont(
          "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
          16);
    }

    std::cout << "\n=== RGB GUARDIAN - PROGRESSIVE DIFFICULTY ===" << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  R = Red" << std::endl;
    std::cout << "  G = Green" << std::endl;
    std::cout << "  B = Blue" << std::endl;
    std::cout << "  P = Pause/Resume" << std::endl;
    std::cout << "  ESC = Exit" << std::endl;
    std::cout << "\nDifficulty System:" << std::endl;
    std::cout << "  - Every " << POINTS_PER_LEVEL << " points = Level Up!"
              << std::endl;
    std::cout << "  - Speed increases each level" << std::endl;
    std::cout << "  - More frequent spawns" << std::endl;
    std::cout << "  - Complex color patterns appear" << std::endl;
    std::cout << "  - Bonus points for fast dots!" << std::endl;
    std::cout << "=============================================" << std::endl;

    return true;
  }

  void loadAudio() {
    bgMusic = Mix_LoadMUS("assets/bg_music.ogg");
    if (bgMusic) {
      Mix_PlayMusic(bgMusic, -1);
      Mix_VolumeMusic(64);
      std::cout << "Background music loaded!" << std::endl;
    }

    correctSound = Mix_LoadWAV("assets/correct.wav");
    wrongSound = Mix_LoadWAV("assets/wrong.wav");
    missSound = Mix_LoadWAV("assets/miss.wav");
    levelUpSound = Mix_LoadWAV("assets/levelup.wav");

    if (!correctSound || !wrongSound || !missSound) {
      std::cout << "Some sound effects could not be loaded (continuing anyway)"
                << std::endl;
    }
  }

  void handleEvents() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
        case SDLK_r:
          handleKeyPress(RED);
          break;
        case SDLK_g:
          handleKeyPress(GREEN);
          break;
        case SDLK_b:
          handleKeyPress(BLUE);
          break;
        case SDLK_p:
          if (!gameOver) {
            paused = !paused;
            if (paused) {
              Mix_PauseMusic();
              std::cout << "⏸️  Game Paused" << std::endl;
            } else {
              Mix_ResumeMusic();
              std::cout << "▶️  Game Resumed" << std::endl;
            }
          }
          break;
        case SDLK_ESCAPE:
          running = false;
          break;
        case SDLK_SPACE:
          if (gameOver) {
            gameOver = false;
            paused = false;
            score = 0;
            level = 1;
            currentSpeed = 2.0f;
            currentSpawnInterval = 120;
            dots.clear();
            frameCount = 0;
            showLevelUp = false;
            usePattern = false;
            if (bgMusic)
              Mix_PlayMusic(bgMusic, -1);
            std::cout << "\n=== NEW GAME ===" << std::endl;
          }
          break;
        }
      }
    }
  }

  void update() {
    for (int i = 0; i < 3; i++) {
      if (buttonPressTimer[i] > 0) {
        buttonPressTimer[i]--;
        if (buttonPressTimer[i] == 0) {
          buttonPressed[i] = false;
        }
      }
    }

    if (gameOver || paused)
      return;

    frameCount++;

    if (frameCount % currentSpawnInterval == 0) {
      float x = WINDOW_WIDTH / 2 - DOT_SIZE / 2;
      Color c = getNextColor();

      float speedVariation = (level > 3) ? (rand() % 10) * 0.1f : 0.0f;
      float dotSpeed = currentSpeed + speedVariation;

      dots.push_back(Dot(x, -DOT_SIZE, c, dotSpeed));
    }

    for (auto &dot : dots) {
      if (dot.active) {
        dot.move();

        if (dot.reachedBottom()) {
          std::cout << "✗ Missed a dot! Game Over! Final Level: " << level
                    << std::endl;
          gameOver = true;
          Mix_HaltMusic();
          if (missSound)
            Mix_PlayChannel(-1, missSound, 0);
        }
      }
    }

    dots.erase(std::remove_if(dots.begin(), dots.end(),
                              [](const Dot &d) {
                                return !d.active && d.y > WINDOW_HEIGHT;
                              }),
               dots.end());
  }

  void render() {
    int bgDarkness = 25 - (level * 2);
    if (bgDarkness < 10)
      bgDarkness = 10;
    SDL_SetRenderDrawColor(renderer, bgDarkness, bgDarkness, bgDarkness + 10,
                           255);
    SDL_RenderClear(renderer);

    drawColumn();

    for (const auto &dot : dots) {
      if (dot.active) {
        drawDot(dot);
      }
    }

    drawButtons();

    drawUI();

    if (gameOver) {
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 200);
      SDL_Rect overlay = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
      SDL_RenderFillRect(renderer, &overlay);

      renderText("GAME OVER", WINDOW_WIDTH / 2 - 80, WINDOW_HEIGHT / 2 - 80,
                 titleFont, {255, 100, 100, 255});
      renderText("Final Score: " + std::to_string(score), WINDOW_WIDTH / 2 - 70,
                 WINDOW_HEIGHT / 2 - 30, font, {255, 255, 255, 255});
      renderText("Level Reached: " + std::to_string(level),
                 WINDOW_WIDTH / 2 - 75, WINDOW_HEIGHT / 2, font,
                 {255, 255, 100, 255});
      renderText("Best Score: " + std::to_string(highScore),
                 WINDOW_WIDTH / 2 - 65, WINDOW_HEIGHT / 2 + 30, smallFont,
                 {200, 200, 200, 255});
      renderText("Press SPACE to restart", WINDOW_WIDTH / 2 - 100,
                 WINDOW_HEIGHT / 2 + 60, font, {200, 200, 255, 255});
    }

    SDL_RenderPresent(renderer);
  }

  void run() {
    loadAudio();

    const int FPS = 60;
    const int frameDelay = 1000 / FPS;

    Uint32 frameStart;
    int frameTime;

    while (running) {
      frameStart = SDL_GetTicks();

      handleEvents();
      update();
      render();

      frameTime = SDL_GetTicks() - frameStart;
      if (frameDelay > frameTime) {
        SDL_Delay(frameDelay - frameTime);
      }
    }
  }

  void cleanup() {
    if (bgMusic)
      Mix_FreeMusic(bgMusic);
    if (correctSound)
      Mix_FreeChunk(correctSound);
    if (wrongSound)
      Mix_FreeChunk(wrongSound);
    if (missSound)
      Mix_FreeChunk(missSound);
    if (levelUpSound)
      Mix_FreeChunk(levelUpSound);

    if (font)
      TTF_CloseFont(font);
    if (titleFont)
      TTF_CloseFont(titleFont);
    if (smallFont)
      TTF_CloseFont(smallFont);

    if (renderer)
      SDL_DestroyRenderer(renderer);
    if (window)
      SDL_DestroyWindow(window);

    Mix_CloseAudio();
    Mix_Quit();
    TTF_Quit();
    SDL_Quit();

    std::cout << "\n=== FINAL STATS ===" << std::endl;
    std::cout << "Score: " << score << std::endl;
    std::cout << "High Score: " << highScore << std::endl;
    std::cout << "Level Reached: " << level << std::endl;
    std::cout << "Thanks for playing! 🎮" << std::endl;
  }

  ~Game() { cleanup(); }
};

int main(int argc, char *argv[]) {
  Game game;

  if (!game.init()) {
    std::cerr << "Failed to initialize game!" << std::endl;
    return -1;
  }

  game.run();

  return 0;
}
