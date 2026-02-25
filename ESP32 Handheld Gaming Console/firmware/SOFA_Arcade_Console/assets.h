#ifndef ASSETS_H
#define ASSETS_H

// Game states for FSM 
enum GameState {
  MAIN_MENU,        
  GAME_SELECTION,   
  PLAYING,
  GAME_OVER,
  CALCULATOR,
  STOPWATCH
};

extern GameState gameState;

// High score memory addresses [cite: 154]
#define EEPROM_EASY_ADDR 4       
#define EEPROM_NORMAL_ADDR 8       
#define EEPROM_HARD_ADDR 12

#endif