#include <Arduino.h>
#include "assets.h"

// Logic for switching between games [cite: 83]
void updateGameLogic() {
  switch(gameState) {
    case GAME_SNAKE:
      updateSnake(); [cite: 85]
      break;
    case GAME_FLAPPY:
      updateFlappyBird(); [cite: 331]
      break;
    case GAME_PING_PONG:
      updatePingPong(); [cite: 332]
      break;
    case CALCULATOR:
      handleCalculatorInput(); [cite: 264]
      break;
  }
}

// Example: Snake movement boundaries [cite: 340]
void checkSnakeCollision(int x, int y) {
  if (x < 2 || x > 126 || y < 12 || y > 62) {
    gameState = GAME_OVER; [cite: 340]
  }
}