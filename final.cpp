#include <ncurses.h>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdlib>

// --------------------------------------------------
// Data structures
// --------------------------------------------------

struct Bullet {
  int x;
  int y;
};

struct Enemy {
  int x;
  int y;
};

// --------------------------------------------------
// Game states
// --------------------------------------------------

enum GameState {
  STATE_HOME,
  STATE_PLAYING,
  STATE_GAME_OVER
};

// --------------------------------------------------
// Constants
// --------------------------------------------------

const int FPS = 60;
const int FRAME_TIME = 1000 / FPS;

// --------------------------------------------------
// Helper functions
// --------------------------------------------------

void setupNcurses() {
  initscr();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);
  curs_set(0);
}

void resetGame(
  int &playerX,
  int maxX,
  std::vector<Bullet> &bullets,
  std::vector<Enemy> &enemies,
  int &score,
  int &spawnCounter,
  int &frameCount
) {
  playerX = maxX / 2;
  bullets.clear();
  enemies.clear();
  score = 0;
  spawnCounter = 0;
  frameCount = 0;
}

// --------------------------------------------------
// Main
// --------------------------------------------------

int main() {
  setupNcurses();

  int maxY, maxX;
  getmaxyx(stdscr, maxY, maxX);

  // Player
  int playerX = maxX / 2;
  int playerY = maxY - 2;

  // Game objects
  std::vector<Bullet> bullets;
  std::vector<Enemy> enemies;

  // Game state
  GameState state = STATE_HOME;
  bool running = true;

  // Game variables
  int score = 0;
  int spawnCounter = 0;
  int frameCount = 0;

  // Difficulty variables (Part 9)
  int spawnDelay = 60;         // lower = more enemies
  int inverseEnemySpeed = 10;  // lower = faster enemies

  // --------------------------------------------------
  // Main loop
  // --------------------------------------------------

  while (running) {
    auto frameStart = std::chrono::high_resolution_clock::now();
    int ch = getch();

    clear();

    // ==================================================
    // HOME STATE
    // ==================================================
    if (state == STATE_HOME) {
      mvprintw(maxY / 2 - 1, maxX / 2 - 8, "TERMINAL INVADERS");
      mvprintw(maxY / 2 + 1, maxX / 2 - 10, "Press ENTER to start");
      mvprintw(maxY / 2 + 2, maxX / 2 - 7, "Press Q to quit");

      if (ch == '\n') {
        resetGame(playerX, maxX, bullets, enemies,
                  score, spawnCounter, frameCount);
        spawnDelay = 60;
        inverseEnemySpeed = 10;
        state = STATE_PLAYING;
      }

      if (ch == 'q' || ch == 'Q') {
        running = false;
      }
    }

    // ==================================================
    // PLAYING STATE
    // ==================================================
    else if (state == STATE_PLAYING) {
      // -------- Input --------
      switch (ch) {
        case 'q':
        case 'Q':
          running = false;
          break;

        case KEY_LEFT:
          playerX--;
          break;

        case KEY_RIGHT:
          playerX++;
          break;

        case ' ':
          bullets.push_back({ playerX, playerY - 1 });
          break;

        default:
          break;
      }

      // -------- Bounds --------
      if (playerX < 0) playerX = 0;
      if (playerX >= maxX) playerX = maxX - 1;

      // -------- Difficulty scaling (Part 9) --------
      inverseEnemySpeed = 10 - (score / 5);
      if (inverseEnemySpeed < 3)
        inverseEnemySpeed = 3;

      spawnDelay = 60 - (score * 2);
      if (spawnDelay < 15)
        spawnDelay = 15;

      // -------- Spawn enemies --------
      spawnCounter++;
      if (spawnCounter >= spawnDelay) {
        enemies.push_back({ rand() % maxX, 1 });
        spawnCounter = 0;
      }

      // -------- Update bullets --------
      for (auto &b : bullets) {
        b.y--;
      }

      for (size_t i = 0; i < bullets.size(); ) {
        if (bullets[i].y < 0) {
          bullets.erase(bullets.begin() + i);
        } else {
          i++;
        }
      }

      // -------- Update enemies --------
      frameCount++;
      if (frameCount % inverseEnemySpeed == 0) {
        for (auto &e : enemies) {
          e.y++;
        }
      }

      // -------- Bullet–Enemy collision --------
      for (size_t i = 0; i < bullets.size(); ) {
        bool hit = false;

        for (size_t j = 0; j < enemies.size(); ) {
          if (bullets[i].x == enemies[j].x &&
              bullets[i].y == enemies[j].y) {
            bullets.erase(bullets.begin() + i);
            enemies.erase(enemies.begin() + j);
            score++;
            hit = true;
            break;
          } else {
            j++;
          }
        }

        if (!hit) i++;
      }

      // -------- Game over condition --------
      for (const auto &e : enemies) {
        if (e.y >= playerY) {
          state = STATE_GAME_OVER;
          break;
        }
      }

      // -------- Render --------
      mvaddch(playerY, playerX, 'A');

      for (const auto &b : bullets) {
        mvaddch(b.y, b.x, '|');
      }

      for (const auto &e : enemies) {
        mvaddch(e.y, e.x, 'V');
      }

      mvprintw(0, 0, "Score: %d", score);
      mvprintw(1, 0, "Enemy speed: %d | Spawn delay: %d",
               inverseEnemySpeed, spawnDelay);
      mvprintw(2, 0, "LEFT/RIGHT move | SPACE shoot | Q quit");
    }

    // ==================================================
    // GAME OVER STATE
    // ==================================================
    else if (state == STATE_GAME_OVER) {
      mvprintw(maxY / 2,     maxX / 2 - 5, "GAME OVER");
      mvprintw(maxY / 2 + 1, maxX / 2 - 6, "Score: %d", score);
      mvprintw(maxY / 2 + 3, maxX / 2 - 9, "Press R to restart");
      mvprintw(maxY / 2 + 4, maxX / 2 - 8, "Press Q to quit");

      if (ch == 'r' || ch == 'R') {
        resetGame(playerX, maxX, bullets, enemies,
                  score, spawnCounter, frameCount);
        spawnDelay = 60;
        inverseEnemySpeed = 10;
        state = STATE_PLAYING;
      }

      if (ch == 'q' || ch == 'Q') {
        running = false;
      }
    }

    refresh();

    // -------- FPS cap --------
    auto frameEnd = std::chrono::high_resolution_clock::now();
    auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(
        frameEnd - frameStart).count();

    if (elapsed < FRAME_TIME) {
      std::this_thread::sleep_for(
        std::chrono::milliseconds(FRAME_TIME - elapsed));
    }
  }

  endwin();
  return 0;
}