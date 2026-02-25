// COMPLETE ARCADE CONSOLE - SNAKE, FLAPPY BIRD, PING PONG + APPS
// HARDWARE: ESP32 + 1.3" SH1106 OLED
// INPUTS: Joystick (32, 33, 25) + Buttons (14, 16, 17)

#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EEPROM.h>
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h> // <--- Add this line!
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// Initialize 1.3" OLED display
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Joystick Pins
#define VRX_PIN 32
#define VRY_PIN 33
#define SW_PIN 25

// Button Pins
#define HOME_BUTTON 14
#define BACK_BUTTON 16
#define ACTION_BUTTON 17

// Calibration
#define CENTER_X 1621
#define CENTER_Y 1615
#define THRESHOLD_X 400
#define THRESHOLD_Y 800

// ============ ESP-NOW SETTINGS ============
// REPLACE THIS WITH YOUR CAR'S MAC ADDRESS (From your previous code)
uint8_t broadcastAddress[] = {0x8C, 0xAA, 0xB5, 0x7F, 0xB9, 0x0C};

// Must match the Car's receiver struct exactly!
typedef struct struct_message {
  int16_t x;
  int16_t y;
} struct_message;

struct_message espNowData;
esp_now_peer_info_t peerInfo;
bool espNowInitialized = false;
volatile bool connectionStatus = false; // New flag for connection status

// ============ CINEMATIC SEQUENCE SETTINGS ============
#define EYE_FADE_IN_TIME 600     
#define EYE_PUPIL_MOVE_TIME 700  
#define EYE_HOLD_TIME 500        
#define EYE_FADE_OUT_TIME 700    
#define BRAND_FADE_IN_TIME 800   
#define BRAND_HOLD_TIME 1500     
#define BRAND_FADE_OUT_TIME 800  

// ============ GAME OVER SETTINGS ============
#define GAME_OVER_FADE_IN_TIME 800    
#define GAME_OVER_HOLD_TIME 800       
#define GAME_OVER_FADE_OUT_TIME 800    
#define SCORES_HOLD_TIME 3000         

// Game Over animation state
enum GameOverState {
  GAME_OVER_FADE_IN,
  GAME_OVER_HOLD,
  GAME_OVER_FADE_OUT,
  GAME_OVER_SHOW_SCORES
};

GameOverState gameOverState = GAME_OVER_FADE_IN;
unsigned long gameOverAnimationStart = 0;
int gameOverFadeValue = 0;

// ============ MENU SYSTEM ============
enum MenuOption {
  MENU_GAMES,
  MENU_ESPNOW,
  MENU_STOPWATCH,
  MENU_CALENDAR,
  MENU_CALCULATOR,
  MENU_COUNT  // Total number of menu options
};

MenuOption currentMenuOption = MENU_GAMES;

// ============ GAME TYPE SELECTION ============
enum GameType {
  GAME_SNAKE,
  GAME_FLAPPY,
  GAME_PING_PONG
};

GameType currentGame = GAME_SNAKE;
int selectedGame = 0;  // 0=Snake, 1=Flappy Bird, 2=Ping Pong

// ============ SNAKE VARIABLES ============
#define SNAKE_SIZE 4
#define MAX_SNAKE_LENGTH 100
#define INITIAL_SNAKE_LENGTH 3

int snakeX[MAX_SNAKE_LENGTH], snakeY[MAX_SNAKE_LENGTH];
int snakeLength = INITIAL_SNAKE_LENGTH;
int dirX = 1, dirY = 0;
int foodX, foodY;
int score = 0;

// Separate high scores for each difficulty
int highScoreEasy = 0;
int highScoreNormal = 0;
int highScoreHard = 0;

// EEPROM addresses for persistent storage
#define EEPROM_SIZE 256
#define EEPROM_MAGIC_ADDR 0      
#define EEPROM_EASY_ADDR 4       
#define EEPROM_NORMAL_ADDR 8       
#define EEPROM_HARD_ADDR 12      
#define EEPROM_FLAPPY_EASY_ADDR 16    
#define EEPROM_FLAPPY_NORMAL_ADDR 20  
#define EEPROM_FLAPPY_HARD_ADDR 24    
#define EEPROM_PING_EASY_ADDR 28      
#define EEPROM_PING_NORMAL_ADDR 32    
#define EEPROM_PING_HARD_ADDR 36      
#define EEPROM_MAGIC_VALUE 0xABCF     

// ============ FLAPPY BIRD VARIABLES ============
#define BIRD_WIDTH 8
#define BIRD_HEIGHT 6
#define BIRD_X 20              
#define GRAVITY 0.4f
#define FLAP_STRENGTH -3.0f
#define MAX_PIPES 3
#define PIPE_WIDTH 10
#define PIPE_GAP 22            
#define PIPE_SPEED 2
#define FLAPPY_GROUND_Y 58     

// Flappy Bird state
float birdY = 30.0f;
float birdVelocity = 0.0f;
bool birdFlapping = false;

// Pipes
int pipeX[MAX_PIPES];
int pipeGapY[MAX_PIPES];       
bool pipeActive[MAX_PIPES];
bool pipePassed[MAX_PIPES];    

// Flappy Bird high scores
int flappyHighScoreEasy = 0;
int flappyHighScoreNormal = 0;
int flappyHighScoreHard = 0;

// Flappy Bird speed settings
#define FLAPPY_SPEED_EASY 50
#define FLAPPY_SPEED_NORMAL 35
#define FLAPPY_SPEED_HARD 25

// ============ PING PONG VARIABLES ============
#define PADDLE_WIDTH 3
#define PADDLE_HEIGHT 15
#define PADDLE_SPEED 4  
#define BALL_SIZE 3

// Ping Pong scores
int pingPongHighScoreEasy = 0;
int pingPongHighScoreNormal = 0;
int pingPongHighScoreHard = 0;

// Ping Pong speed settings
#define PING_PONG_SPEED_EASY 30
#define PING_PONG_SPEED_NORMAL 20
#define PING_PONG_SPEED_HARD 10

// Ping Pong game state
float pingBallX = SCREEN_WIDTH / 2;
float pingBallY = SCREEN_HEIGHT / 2;
float pingBallSpeedX = 2.0;
float pingBallSpeedY = 1.5;
int leftPaddleY = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
int rightPaddleY = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
int pingLifeScore = 0;    
int pingPlayerScore = 0;  

// Ping Pong boundaries
#define PLAY_AREA_TOP 10
#define PLAY_AREA_BOTTOM (SCREEN_HEIGHT - 10)
#define PLAY_AREA_LEFT 5
#define PLAY_AREA_RIGHT (SCREEN_WIDTH - 8)

// LIFE difficulty settings
#define LIFE_REACTION_EASY 25
#define LIFE_REACTION_NORMAL 15   
#define LIFE_REACTION_HARD 5

// ============ DATE VARIABLES ============
// Default Date: Dec 25, 2025
int calDay = 25;
int calMonth = 12;
int calYear = 2025;

const char* monthNames[] = {
  "", "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", 
  "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
};

unsigned long lastCalendarMove = 0;
#define CALENDAR_MOVE_DELAY 250

// ============ STOPWATCH VARIABLES ============
unsigned long stopwatchStartTime = 0;
unsigned long stopwatchPausedTime = 0;
bool stopwatchRunning = false;

// Flag to prevent direction reversal
bool directionChanged = false;

// Game boundaries
#define BORDER_LEFT 2          
#define BORDER_RIGHT SCREEN_WIDTH - SNAKE_SIZE - 2  
#define BORDER_TOP 12          
#define BORDER_BOTTOM SCREEN_HEIGHT - SNAKE_SIZE - 2  

// Game states
enum GameState {
  HOME_SCREEN, // New Home Screen State
  MAIN_MENU,        
  GAME_SELECTION,   
  MODE_SELECTION,   
  PLAYING,
  PAUSED,
  GAME_OVER,
  ESPNOW_MENU,      
  STOPWATCH,        
  CALENDAR,         
  CALCULATOR        
};

GameState gameState = MAIN_MENU;

// Game modes
enum GameMode {
  EASY_MODE,
  NORMAL_MODE,
  HARD_MODE
};

GameMode currentMode = NORMAL_MODE;

// Timing
unsigned long lastUpdate = 0;
#define GAME_SPEED_EASY 180
#define GAME_SPEED_NORMAL 140
#define GAME_SPEED_HARD 100
#define DEBOUNCE_DELAY 50 

// Button states
bool buttonPressed = false; // Joystick button
bool lastButtonState = true;

bool homePressed = false;
bool backPressed = false;
bool actionPressed = false;
bool lastHomeState = true;
bool lastBackState = true;
bool lastActionState = true;

// Button timing for debouncing
unsigned long lastHomeTime = 0;
unsigned long lastBackTime = 0;
unsigned long lastActionTime = 0;
unsigned long lastJoystickButtonTime = 0;

// Obstacles for HARD mode
#define MAX_OBSTACLES 6
#define OBSTACLE_SIZE 12  
int obstaclesX[MAX_OBSTACLES];
int obstaclesY[MAX_OBSTACLES];
int obstaclesCount = 2;  

// Mode selection cursor
int selectedMode = 1; 

// Pause menu selection
int pauseSelection = 0; 

// ============ CALCULATOR VARIABLES ============
String calcInput = "0";       
float firstNum = 0;           
char calcOperation = 0;       
bool newEntry = true;         
int calcCursorX = 0;          
int calcCursorY = 0;          
unsigned long lastCalcMove = 0; 
#define MOVE_DELAY 200

// Calculator Grid Layout
const char calcKeys[4][4] = {
  {'7', '8', '9', '/'},
  {'4', '5', '6', '*'},
  {'1', '2', '3', '-'},
  {'0', 'C', '=', '+'}
};

// ============ FUNCTION PROTOTYPES ============
void playBootIntro();
void playEyeAnimation();
void showBrandScreen();
void resetGame();
void updateGame();
void drawGame();
void generateFood();
void generateObstacles();
int getGameSpeed();
int getCurrentHighScore();
int* getCurrentHighScorePtr();
void loadHighScores();
void saveHighScores();
void resetFlappyBird();
void updateFlappyBird();
void drawFlappyBird();
void generatePipe(int index);
void resetPingPong();
void updatePingPong();
void drawPingPong();
void controlLife();
void resetBall();
void checkPingPongGameOver();
void handleCalculatorInput();
void performCalculation();
void drawCalculator();
void resetCalculator();
void handleDateInput();
void drawDateScreen();
bool isLeapYear(int year);
int getDaysInMonth(int month, int year);
int getDayOfWeek(int d, int m, int y);
void handleStopwatchInput();
void drawStopwatch();
String formatTime(unsigned long milliseconds);
void readJoystick();
void checkButtons();
void handleMainMenu();
void handleGameSelection();
void handleModeSelection();
void handlePauseMenu();
void handleGameOver(unsigned long currentTime);
void drawMainMenu();
void drawGameSelection();
void drawModeSelection();
void drawPlaying();
void drawPauseMenu();
void drawGameOver();
void drawGameOverScores();
void drawESPNowMenu();
void handleHomeScreen();
void drawHomeScreen();

// ============ ESP-NOW FUNCTIONS ============
void initESPNow();
void updateESPNow();
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  connectionStatus = (status == ESP_NOW_SEND_SUCCESS);
}
void initESPNow() {
  // If already initialized, stop here (prevents double-loading)
  if (espNowInitialized) return;
  
  Serial.println("Starting WiFi...");

  // 1. Set Mode
  WiFi.mode(WIFI_STA);
  
  // 2. Force Channel 1
  // (This matches your car code)
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  
  // 3. Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
  
  // 4. Add Peer (The Car)
  // Clear the variable first to prevent garbage data crashes
  memset(&peerInfo, 0, sizeof(peerInfo)); 
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  espNowInitialized = true;
  Serial.println("ESP-NOW Ready!");
}
void updateESPNow() {
  if (!espNowInitialized) return;

  // 1. Read Raw Inputs
  int rawX = analogRead(VRX_PIN);
  int rawY = analogRead(VRY_PIN);

  // 2. Deadzone Configuration
  // 200 is a safe zone. If drift continues, increase to 300.
  const int DEADZONE = 200; 

  // 3. Calculate X (With Deadzone)
  // Mapping logic: 0..4095 -> 1000..-1000 (Inverted X)
  if (rawX < CENTER_X - DEADZONE) {
    // Left Side (0 to Center)
    espNowData.x = map(rawX, 0, CENTER_X - DEADZONE, 1000, 0);
  } 
  else if (rawX > CENTER_X + DEADZONE) {
    // Right Side (Center to 4095)
    espNowData.x = map(rawX, CENTER_X + DEADZONE, 4095, 0, -1000);
  } 
  else {
    // Deadzone (Stick is centered)
    espNowData.x = 0;
  }

  // 4. Calculate Y (With Deadzone)
  // Mapping logic: 0..4095 -> -1000..1000 (Standard Y)
  if (rawY < CENTER_Y - DEADZONE) {
    // Down Side (0 to Center)
    espNowData.y = map(rawY, 0, CENTER_Y - DEADZONE, -1000, 0);
  } 
  else if (rawY > CENTER_Y + DEADZONE) {
    // Up Side (Center to 4095)
    espNowData.y = map(rawY, CENTER_Y + DEADZONE, 4095, 0, 1000);
  } 
  else {
    // Deadzone (Stick is centered)
    espNowData.y = 0;
  }

  // 5. Send Data
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &espNowData, sizeof(espNowData));
}
// ============ CINEMATIC ANIMATION FUNCTIONS ============

void playBootIntro() {
  playEyeAnimation();
  showBrandScreen();
}

void playEyeAnimation() {
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  delay(100);
  
  int leftEyeX = SCREEN_WIDTH / 2 - 16;
  int rightEyeX = SCREEN_WIDTH / 2 + 16;
  int eyeY = SCREEN_HEIGHT / 2;
  int eyeRadius = 6;
  int pupilRadius = 2;
  
  for (int i = 0; i <= 4; i++) {
    u8g2.clearBuffer();
    for (int j = 0; j < i; j++) {
      u8g2.drawCircle(leftEyeX, eyeY, eyeRadius - j);
      u8g2.drawCircle(rightEyeX, eyeY, eyeRadius - j);
    }
    u8g2.sendBuffer();
    delay(EYE_FADE_IN_TIME / 5);
  }
  
  for (int i = 1; i <= 3; i++) {
    u8g2.clearBuffer();
    u8g2.drawCircle(leftEyeX, eyeY, eyeRadius);
    u8g2.drawCircle(rightEyeX, eyeY, eyeRadius);
    u8g2.drawDisc(leftEyeX, eyeY, i);
    u8g2.drawDisc(rightEyeX, eyeY, i);
    u8g2.sendBuffer();
    delay(100);
  }
  
  int pupilOffset = 3;
  for (int i = 0; i <= pupilOffset; i++) {
    u8g2.clearBuffer();
    u8g2.drawCircle(leftEyeX, eyeY, eyeRadius);
    u8g2.drawCircle(rightEyeX, eyeY, eyeRadius);
    u8g2.drawDisc(leftEyeX - i, eyeY, pupilRadius);
    u8g2.drawDisc(rightEyeX - i, eyeY, pupilRadius);
    u8g2.sendBuffer();
    delay(EYE_PUPIL_MOVE_TIME / pupilOffset);
  }
  
  delay(EYE_HOLD_TIME / 3);
  
  for (int i = 0; i <= pupilOffset * 2; i++) {
    u8g2.clearBuffer();
    u8g2.drawCircle(leftEyeX, eyeY, eyeRadius);
    u8g2.drawCircle(rightEyeX, eyeY, eyeRadius);
    u8g2.drawDisc(leftEyeX - pupilOffset + i, eyeY, pupilRadius);
    u8g2.drawDisc(rightEyeX - pupilOffset + i, eyeY, pupilRadius);
    u8g2.sendBuffer();
    delay(EYE_PUPIL_MOVE_TIME / (pupilOffset * 2));
  }
  
  delay(EYE_HOLD_TIME / 3);
  
  for (int i = 0; i <= pupilOffset; i++) {
    u8g2.clearBuffer();
    u8g2.drawCircle(leftEyeX, eyeY, eyeRadius);
    u8g2.drawCircle(rightEyeX, eyeY, eyeRadius);
    u8g2.drawDisc(leftEyeX + pupilOffset - i, eyeY, pupilRadius);
    u8g2.drawDisc(rightEyeX + pupilOffset - i, eyeY, pupilRadius);
    u8g2.sendBuffer();
    delay(EYE_PUPIL_MOVE_TIME / pupilOffset);
  }
  
  delay(EYE_HOLD_TIME);
  
  for (int i = 4; i >= 0; i--) {
    u8g2.clearBuffer();
    for (int j = 0; j < i; j++) {
      u8g2.drawCircle(leftEyeX, eyeY, eyeRadius - j);
      u8g2.drawCircle(rightEyeX, eyeY, eyeRadius - j);
    }
    if (i >= 2) {
      u8g2.drawDisc(leftEyeX, eyeY, i - 2);
      u8g2.drawDisc(rightEyeX, eyeY, i - 2);
    }
    u8g2.sendBuffer();
    delay(EYE_FADE_OUT_TIME / 5);
  }
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  delay(100);
}

void showBrandScreen() {
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  delay(100);
  
  for (int i = 0; i <= 4; i++) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    int presentedWidth = u8g2.getStrWidth("PRESENTED BY");
    int presentedX = (SCREEN_WIDTH - presentedWidth) / 2;
    int presentedY = 15;
    for (int j = 0; j < i; j++) {
      u8g2.drawStr(presentedX, presentedY, "PRESENTED BY");
    }
    u8g2.sendBuffer();
    delay(BRAND_FADE_IN_TIME / 5);
  }
  
  u8g2.setFont(u8g2_font_logisoso28_tf);
  int sofaWidth = u8g2.getStrWidth("SOFA");
  int sofaX = (SCREEN_WIDTH - sofaWidth) / 2;
  int sofaY = 52;
  
  for (int i = 0; i <= 4; i++) {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr((SCREEN_WIDTH - u8g2.getStrWidth("PRESENTED BY")) / 2, 15, "PRESENTED BY");
    u8g2.setFont(u8g2_font_logisoso28_tf);
    u8g2.setDrawColor(1);
    for (int j = 0; j < i; j++) {
      u8g2.drawStr(sofaX, sofaY, "SOFA");
    }
    u8g2.sendBuffer();
    delay(100);
  }
  
  delay(BRAND_HOLD_TIME);
  
  for (int i = 4; i >= 0; i--) {
    u8g2.clearBuffer();
    if (i >= 2) {
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr((SCREEN_WIDTH - u8g2.getStrWidth("PRESENTED BY")) / 2, 15, "PRESENTED BY");
    }
    if (i >= 1) {
      u8g2.setFont(u8g2_font_logisoso28_tf);
      u8g2.setDrawColor(1);
      u8g2.drawStr(sofaX, sofaY, "SOFA");
    }
    u8g2.sendBuffer();
    delay(BRAND_FADE_OUT_TIME / 5);
  }
  u8g2.clearBuffer();
  u8g2.sendBuffer();
  delay(100);
}
// ============ SETUP AND MAIN LOOP ============
void setup() {
  // 1. DISABLE BROWNOUT DETECTOR (Prevents crash on WiFi start)
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  
  Serial.begin(115200);
  delay(1000); // Give serial time to start

  // 2. Initialize Hardware
  // Initialize EEPROM and load high scores
  EEPROM.begin(EEPROM_SIZE);
  loadHighScores();
  
  // Initialize OLED
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.sendBuffer();

  // 3. Init WiFi/ESP-NOW *BEFORE* Animation
  // Doing this early helps stabilize the power draw
  Serial.println("Initializing Radio...");
  initESPNow();

  // 4. Play Intro
  Serial.println("Playing cinematic boot intro...");
  playBootIntro();
  
  // 5. Initialize Pins
  Serial.println("Boot intro complete. Initializing pins...");
  pinMode(VRX_PIN, INPUT);
  pinMode(VRY_PIN, INPUT);
  pinMode(SW_PIN, INPUT_PULLUP);
  
  pinMode(HOME_BUTTON, INPUT_PULLUP);
  pinMode(BACK_BUTTON, INPUT_PULLUP);
  pinMode(ACTION_BUTTON, INPUT_PULLUP);
  
  randomSeed(analogRead(0));
  
  // 6. Ready
  gameState = HOME_SCREEN;
  Serial.println("System ready.");
}
void loop() {
  unsigned long currentTime = millis();
  
  // FIX: ALWAYS Check buttons first (Crucial for Home button to work in Calc/Stopwatch)
  checkButtons(); 

  // Read joystick logic based on state
  if (gameState != CALCULATOR) {
    readJoystick();
  } else {
    // Calculator handles its own joystick analog reading for grid movement
    handleCalculatorInput();
  }
  
  // Handle HOME button (pin 14) - goes to main menu from anywhere
  if (homePressed) {
    if (gameState == STOPWATCH && stopwatchRunning) {
      // Stop stopwatch before exiting
      stopwatchRunning = false;
      stopwatchPausedTime = currentTime - stopwatchStartTime;
    }
    gameState = MAIN_MENU;
    homePressed = false;
    // Debounce is handled in checkButtons, but small delay helps state transition
    delay(50); 
  }
  
  // Handle BACK button (pin 16) - goes back one step
  if (backPressed) {
    switch(gameState) {
      case MAIN_MENU:
        gameState = HOME_SCREEN; // Back from Main Menu goes to Home/Lock Screen
        break;
      case GAME_SELECTION:
        gameState = MAIN_MENU;
        break;
      case ESPNOW_MENU:
      case STOPWATCH:
      case CALENDAR:
      case CALCULATOR:
        gameState = MAIN_MENU;
        break;
      case MODE_SELECTION:
        gameState = GAME_SELECTION;
        break;
      case PLAYING:
        gameState = PAUSED;
        pauseSelection = 0;
        break;
      case PAUSED:
        gameState = MODE_SELECTION;
        break;
      case GAME_OVER:
        gameState = MODE_SELECTION;
        break;
      default:
        break;
    }
    backPressed = false;
    delay(50);
  }
  
  // Handle Global ACTION button logic (mostly for Flappy Bird here)
  if (actionPressed) {
    if (gameState == PLAYING && currentGame == GAME_FLAPPY) {
      birdVelocity = FLAP_STRENGTH;
      actionPressed = false; // Consume click
    }
  }
  
  switch(gameState) {
    case HOME_SCREEN:
      handleHomeScreen();
      break;

    case MAIN_MENU:
      handleMainMenu();
      break;
      
    case GAME_SELECTION:
      handleGameSelection();
      break;
      
    case MODE_SELECTION:
      handleModeSelection();
      break;
      
    case PLAYING:
      if (buttonPressed) { // Joystick button pauses
        gameState = PAUSED;
        pauseSelection = 0;
        buttonPressed = false;
        delay(DEBOUNCE_DELAY);
      } else if (currentTime - lastUpdate > getGameSpeed()) {
        updateGame();
        lastUpdate = currentTime;
      }
      break;
      
    case PAUSED:
      handlePauseMenu();
      break;
      
    case GAME_OVER:
      handleGameOver(currentTime);
      break;
      
    case STOPWATCH:
      handleStopwatchInput();
      break;
      
    case CALENDAR:
      handleDateInput();
      break;
      
    case ESPNOW_MENU:
      //initESPNow(); // Ensure wifi is on
      updateESPNow(); // Send data
      drawESPNowMenu(); // Draw screen
      break;
      
    case CALCULATOR:
      // These are handled by their own drawing functions or top-level inputs
      break;
  }
  
  drawGame();
  delay(10);
}

// ============ INPUT FUNCTIONS ============

void readJoystick() {
  int x = analogRead(VRX_PIN);
  int y = analogRead(VRY_PIN);
  
  // In main menu, use joystick HORIZONTAL movement for navigation
  if (gameState == MAIN_MENU) {
    static unsigned long lastMenuMove = 0;
    if (millis() - lastMenuMove > 300) {
      if (x < (CENTER_X - THRESHOLD_X)) {
        // Move left
        currentMenuOption = static_cast<MenuOption>((currentMenuOption - 1 + MENU_COUNT) % MENU_COUNT);
        lastMenuMove = millis();
      } 
      else if (x > (CENTER_X + THRESHOLD_X)) {
        // Move right
        currentMenuOption = static_cast<MenuOption>((currentMenuOption + 1) % MENU_COUNT);
        lastMenuMove = millis();
      }
    }
    return;
  }
  
  // In game selection, use joystick for navigation
  if (gameState == GAME_SELECTION) {
    static unsigned long lastGameSelectMove = 0;
    if (millis() - lastGameSelectMove > 200) {
      if (y < (CENTER_Y - THRESHOLD_Y)) {
        selectedGame = (selectedGame - 1 + 3) % 3;
        lastGameSelectMove = millis();
      } 
      else if (y > (CENTER_Y + THRESHOLD_Y)) {
        selectedGame = (selectedGame + 1) % 3;
        lastGameSelectMove = millis();
      }
    }
    return;
  }
  
  // In mode selection, use joystick for navigation
  if (gameState == MODE_SELECTION) {
    static unsigned long lastMenuMove = 0;
    if (millis() - lastMenuMove > 200) {
      if (y < (CENTER_Y - THRESHOLD_Y)) {
        selectedMode--;
        if (selectedMode < 0) selectedMode = 2;
        lastMenuMove = millis();
      } 
      else if (y > (CENTER_Y + THRESHOLD_Y)) {
        selectedMode++;
        if (selectedMode > 2) selectedMode = 0;
        lastMenuMove = millis();
      }
    }
    return;
  }
  
  // In pause menu, use VERTICAL movement for selection
  if (gameState == PAUSED) {
    static unsigned long lastPauseMove = 0;
    if (millis() - lastPauseMove > 200) {
      if (y < (CENTER_Y - THRESHOLD_Y)) {
        pauseSelection = 0;
        lastPauseMove = millis();
      } 
      else if (y > (CENTER_Y + THRESHOLD_Y)) {
        pauseSelection = 1;
        lastPauseMove = millis();
      }
    }
    return;
  }
  
  if (gameState != PLAYING) return;
  
  // ========== FLAPPY BIRD INPUT ==========
  if (currentGame == GAME_FLAPPY) {
    if (y < (CENTER_Y - THRESHOLD_Y) || x < (CENTER_X - THRESHOLD_X) || x > (CENTER_X + THRESHOLD_X)) {
      if (!birdFlapping) {
        birdVelocity = FLAP_STRENGTH;
        birdFlapping = true;
      }
    } else {
      birdFlapping = false;
    }
    return;
  }
  
  // ========== PING PONG INPUT ==========
  if (currentGame == GAME_PING_PONG) {
    // Player paddle control (right paddle)
    if (y < (CENTER_Y - THRESHOLD_Y)) {
      rightPaddleY -= PADDLE_SPEED;
      if (rightPaddleY < PLAY_AREA_TOP) rightPaddleY = PLAY_AREA_TOP;
    } 
    else if (y > (CENTER_Y + THRESHOLD_Y)) {
      rightPaddleY += PADDLE_SPEED;
      if (rightPaddleY > PLAY_AREA_BOTTOM - PADDLE_HEIGHT) 
        rightPaddleY = PLAY_AREA_BOTTOM - PADDLE_HEIGHT;
    }
    return;
  }
  
  // ========== SNAKE INPUT ==========
  if (directionChanged) return;
  
  int prevDirX = dirX;
  int prevDirY = dirY;
  
  // Horizontal movement
  if (x < (CENTER_X - THRESHOLD_X)) {
    if (prevDirX == 0) {
      dirX = -1;
      dirY = 0;
      directionChanged = true;
    }
  } 
  else if (x > (CENTER_X + THRESHOLD_X)) {
    if (prevDirX == 0) {
      dirX = 1;
      dirY = 0;
      directionChanged = true;
    }
  }
  
  // Vertical movement
  if (y < (CENTER_Y - THRESHOLD_Y)) {
    if (prevDirY == 0) {
      dirX = 0;
      dirY = -1;
      directionChanged = true;
    }
  } 
  else if (y > (CENTER_Y + THRESHOLD_Y)) {
    if (prevDirY == 0) {
      dirX = 0;
      dirY = 1;
      directionChanged = true;
    }
  }
}

void checkButtons() {
  unsigned long currentTime = millis();
  
  // Check joystick button with debouncing
  bool currentJoystickState = (digitalRead(SW_PIN) == LOW);
  if (currentJoystickState && !lastButtonState && (currentTime - lastJoystickButtonTime > DEBOUNCE_DELAY)) {
    buttonPressed = true;
    lastJoystickButtonTime = currentTime;
    
    // Joystick click also flaps in Flappy bird
    if (gameState == PLAYING && currentGame == GAME_FLAPPY) {
      birdVelocity = FLAP_STRENGTH;
    }
  }
  lastButtonState = currentJoystickState;
  
  // Check HOME button (pin 14) with debouncing
  bool currentHomeState = (digitalRead(HOME_BUTTON) == LOW);
  if (currentHomeState && !lastHomeState && (currentTime - lastHomeTime > DEBOUNCE_DELAY)) {
    homePressed = true;
    lastHomeTime = currentTime;
  }
  lastHomeState = currentHomeState;
  
  // Check BACK button (pin 16) with debouncing
  bool currentBackState = (digitalRead(BACK_BUTTON) == LOW);
  if (currentBackState && !lastBackState && (currentTime - lastBackTime > DEBOUNCE_DELAY)) {
    backPressed = true;
    lastBackTime = currentTime;
  }
  lastBackState = currentBackState;
  
  // Check ACTION button (pin 17) with debouncing
  bool currentActionState = (digitalRead(ACTION_BUTTON) == LOW);
  if (currentActionState && !lastActionState && (currentTime - lastActionTime > DEBOUNCE_DELAY)) {
    actionPressed = true;
    lastActionTime = currentTime;
  }
  lastActionState = currentActionState;
}

// ============ MENU FUNCTIONS ============

void handleMainMenu() {
  if (buttonPressed || actionPressed) { // Allow Action button to select too
    switch(currentMenuOption) {
      case MENU_GAMES:
        gameState = GAME_SELECTION;
        selectedGame = 0;
        break;
      case MENU_ESPNOW:
        //initESPNow();
        gameState = ESPNOW_MENU;
        break;
      case MENU_STOPWATCH:
        gameState = STOPWATCH;
        stopwatchRunning = false;
        stopwatchStartTime = 0;
        stopwatchPausedTime = 0;
        break;
      case MENU_CALENDAR:
        gameState = CALENDAR;
        break;
      case MENU_CALCULATOR:
        gameState = CALCULATOR;
        resetCalculator();
        break;
      default:
        break;
    }
    buttonPressed = false;
    actionPressed = false;
    delay(DEBOUNCE_DELAY);
  }
}

void handleGameSelection() {
  if (buttonPressed || actionPressed) {
    if (selectedGame == 0) {
      currentGame = GAME_SNAKE;
    } else if (selectedGame == 1) {
      currentGame = GAME_FLAPPY;
    } else if (selectedGame == 2) {
      currentGame = GAME_PING_PONG;
    }
    
    gameState = MODE_SELECTION;
    selectedMode = 1;  // Default to Normal
    buttonPressed = false;
    actionPressed = false;
    delay(DEBOUNCE_DELAY);
  }
}

void handleModeSelection() {
  if (buttonPressed || actionPressed) {
    switch(selectedMode) {
      case 0: currentMode = EASY_MODE; break;
      case 1: currentMode = NORMAL_MODE; break;
      case 2: currentMode = HARD_MODE; break;
    }
    
    resetGame();
    gameState = PLAYING;
    buttonPressed = false;
    actionPressed = false;
    delay(DEBOUNCE_DELAY);
  }
}

void handlePauseMenu() {
  if (buttonPressed || actionPressed) {
    if (pauseSelection == 0) {
      gameState = PLAYING;
    } else {
      gameState = MAIN_MENU;
    }
    buttonPressed = false;
    actionPressed = false;
    delay(DEBOUNCE_DELAY);
  }
}

void handleGameOver(unsigned long currentTime) {
  if (gameOverAnimationStart == 0) {
    gameOverAnimationStart = currentTime;
    gameOverState = GAME_OVER_FADE_IN;
    gameOverFadeValue = 0;
  }
  
  unsigned long elapsed = currentTime - gameOverAnimationStart;
  
  switch(gameOverState) {
    case GAME_OVER_FADE_IN:
      gameOverFadeValue = map(elapsed, 0, GAME_OVER_FADE_IN_TIME, 0, 4);
      if (elapsed >= GAME_OVER_FADE_IN_TIME) {
        gameOverState = GAME_OVER_HOLD;
        gameOverAnimationStart = currentTime;
      }
      break;
      
    case GAME_OVER_HOLD:
      if (elapsed >= GAME_OVER_HOLD_TIME) {
        gameOverState = GAME_OVER_FADE_OUT;
        gameOverAnimationStart = currentTime;
      }
      break;
      
    case GAME_OVER_FADE_OUT:
      gameOverFadeValue = map(elapsed, 0, GAME_OVER_FADE_OUT_TIME, 4, 0);
      if (elapsed >= GAME_OVER_FADE_OUT_TIME) {
        gameOverState = GAME_OVER_SHOW_SCORES;
        gameOverAnimationStart = currentTime;
      }
      break;
      
    case GAME_OVER_SHOW_SCORES:
      if (elapsed >= SCORES_HOLD_TIME) {
        gameState = MODE_SELECTION;
        gameOverAnimationStart = 0;
      }
      break;
  }
  
  if (buttonPressed || homePressed || backPressed || actionPressed) {
    gameState = MODE_SELECTION;
    gameOverAnimationStart = 0;
    buttonPressed = false;
    homePressed = false;
    backPressed = false;
    actionPressed = false;
    delay(DEBOUNCE_DELAY);
  }
}

// ============ GAME LOGIC FUNCTIONS ============

void updateGame() {
  if (currentGame == GAME_FLAPPY) {
    updateFlappyBird();
    return;
  } else if (currentGame == GAME_PING_PONG) {
    updatePingPong();
    return;
  }
  
  // ========== SNAKE UPDATE ==========
  directionChanged = false;
  
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  
  snakeX[0] += dirX * SNAKE_SIZE;
  snakeY[0] += dirY * SNAKE_SIZE;
  
  if (currentMode == EASY_MODE) {
    if (snakeX[0] < BORDER_LEFT) {
      snakeX[0] = BORDER_RIGHT;
    } else if (snakeX[0] > BORDER_RIGHT) {
      snakeX[0] = BORDER_LEFT;
    }
    
    if (snakeY[0] < BORDER_TOP) {
      snakeY[0] = BORDER_BOTTOM;
    } else if (snakeY[0] > BORDER_BOTTOM) {
      snakeY[0] = BORDER_TOP;
    }
  } else if (currentMode == NORMAL_MODE || currentMode == HARD_MODE) {
    if (snakeX[0] < BORDER_LEFT || snakeX[0] > BORDER_RIGHT ||
        snakeY[0] < BORDER_TOP || snakeY[0] > BORDER_BOTTOM) {
      gameState = GAME_OVER;
      return;
    }
  }
  
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameState = GAME_OVER;
      return;
    }
  }
  
  if (currentMode == HARD_MODE) {
    for (int i = 0; i < obstaclesCount; i++) {
      if (snakeX[0] < obstaclesX[i] + OBSTACLE_SIZE && 
          snakeX[0] + SNAKE_SIZE > obstaclesX[i] &&
          snakeY[0] < obstaclesY[i] + OBSTACLE_SIZE && 
          snakeY[0] + SNAKE_SIZE > obstaclesY[i]) {
        gameState = GAME_OVER;
        return;
      }
    }
  }
  
  if (snakeX[0] < foodX + SNAKE_SIZE && 
      snakeX[0] + SNAKE_SIZE > foodX &&
      snakeY[0] < foodY + SNAKE_SIZE && 
      snakeY[0] + SNAKE_SIZE > foodY) {
    if (snakeLength < MAX_SNAKE_LENGTH) {
      snakeLength++;
      snakeX[snakeLength - 1] = snakeX[snakeLength - 2];
      snakeY[snakeLength - 1] = snakeY[snakeLength - 2];
    }
    
    score += 10;
    
    int* currentHighScore = getCurrentHighScorePtr();
    if (score > *currentHighScore) {
      *currentHighScore = score;
      saveHighScores();
    }
    
    generateFood();
  }
}

int getGameSpeed() {
  if (currentGame == GAME_FLAPPY) {
    switch(currentMode) {
      case EASY_MODE: return FLAPPY_SPEED_EASY;
      case NORMAL_MODE: return FLAPPY_SPEED_NORMAL;
      case HARD_MODE: return FLAPPY_SPEED_HARD;
      default: return FLAPPY_SPEED_NORMAL;
    }
  } else if (currentGame == GAME_PING_PONG) {
    switch(currentMode) {
      case EASY_MODE: return PING_PONG_SPEED_EASY;
      case NORMAL_MODE: return PING_PONG_SPEED_NORMAL;
      case HARD_MODE: return PING_PONG_SPEED_HARD;
      default: return PING_PONG_SPEED_NORMAL;
    }
  }
  
  switch(currentMode) {
    case EASY_MODE: return GAME_SPEED_EASY;
    case NORMAL_MODE: return GAME_SPEED_NORMAL;
    case HARD_MODE: return GAME_SPEED_HARD;
    default: return GAME_SPEED_NORMAL;
  }
}

void generateFood() {
  bool validPosition = false;
  int attempts = 0;
  
  int gridCols = (SCREEN_WIDTH / SNAKE_SIZE);
  int gridRows = (SCREEN_HEIGHT / SNAKE_SIZE);
  
  int minYGrid = (BORDER_TOP / SNAKE_SIZE) + 1;
  int maxYGrid = (BORDER_BOTTOM / SNAKE_SIZE) - 1;
  int minXGrid = (BORDER_LEFT / SNAKE_SIZE) + 1;
  int maxXGrid = (BORDER_RIGHT / SNAKE_SIZE) - 1;
  
  while (!validPosition && attempts < 100) {
    int gridX = random(minXGrid, maxXGrid);
    int gridY = random(minYGrid, maxYGrid);
    
    foodX = gridX * SNAKE_SIZE;
    foodY = gridY * SNAKE_SIZE;
    
    if (foodX < BORDER_LEFT || foodX > BORDER_RIGHT || 
        foodY < BORDER_TOP || foodY > BORDER_BOTTOM) {
      attempts++;
      continue;
    }
    
    validPosition = true;
    for (int i = 0; i < snakeLength; i++) {
      if (foodX == snakeX[i] && foodY == snakeY[i]) {
        validPosition = false;
        break;
      }
    }
    
    if (validPosition && currentMode == HARD_MODE) {
      for (int i = 0; i < obstaclesCount; i++) {
        if (foodX < obstaclesX[i] + OBSTACLE_SIZE && 
            foodX + SNAKE_SIZE > obstaclesX[i] &&
            foodY < obstaclesY[i] + OBSTACLE_SIZE && 
            foodY + SNAKE_SIZE > obstaclesY[i]) {
          validPosition = false;
          break;
        }
      }
    }
    
    attempts++;
  }
  
  if (!validPosition) {
    foodX = SCREEN_WIDTH / 2;
    foodY = SCREEN_HEIGHT / 2;
  }
}

void generateObstacles() {
  obstaclesCount = 2;
  
  for (int i = 0; i < obstaclesCount; i++) {
    obstaclesX[i] = -100;
    obstaclesY[i] = -100;
  }
  
  for (int i = 0; i < obstaclesCount; i++) {
    bool validPosition = false;
    int attempts = 0;
    
    while (!validPosition && attempts < 100) {
      int minX = BORDER_LEFT + 5;
      int maxX = BORDER_RIGHT - OBSTACLE_SIZE - 5;
      int minY = BORDER_TOP + 5;
      int maxY = BORDER_BOTTOM - OBSTACLE_SIZE - 5;
      
      if (maxX <= minX) maxX = minX + 10;
      if (maxY <= minY) maxY = minY + 10;
      
      obstaclesX[i] = random(minX, maxX);
      obstaclesY[i] = random(minY, maxY);
      
      obstaclesX[i] = (obstaclesX[i] / SNAKE_SIZE) * SNAKE_SIZE;
      obstaclesY[i] = (obstaclesY[i] / SNAKE_SIZE) * SNAKE_SIZE;
      
      validPosition = true;
      
      int centerX = SCREEN_WIDTH / 2;
      int centerY = SCREEN_HEIGHT / 2;
      if (abs(obstaclesX[i] - centerX) < 32 && abs(obstaclesY[i] - centerY) < 32) {
        validPosition = false;
        attempts++;
        continue;
      }
      
      for (int j = 0; j < i; j++) {
        if (obstaclesX[j] != -100 && obstaclesY[j] != -100) {
          if (obstaclesX[i] < obstaclesX[j] + OBSTACLE_SIZE && 
              obstaclesX[i] + OBSTACLE_SIZE > obstaclesX[j] &&
              obstaclesY[i] < obstaclesY[j] + OBSTACLE_SIZE && 
              obstaclesY[i] + OBSTACLE_SIZE > obstaclesY[j]) {
            validPosition = false;
            break;
          }
          
          if (abs(obstaclesX[i] - obstaclesX[j]) < 16 && 
              abs(obstaclesY[i] - obstaclesY[j]) < 16) {
            validPosition = false;
            break;
          }
        }
      }
      
      attempts++;
    }
    
    if (!validPosition) {
      if (i == 0) {
        obstaclesX[i] = BORDER_LEFT + 30;
        obstaclesY[i] = BORDER_TOP + 30;
      } else {
        obstaclesX[i] = BORDER_RIGHT - 30 - OBSTACLE_SIZE;
        obstaclesY[i] = BORDER_BOTTOM - 30 - OBSTACLE_SIZE;
      }
    }
  }
}

void resetGame() {
  score = 0;
  lastUpdate = millis();
  
  if (currentGame == GAME_FLAPPY) {
    resetFlappyBird();
    return;
  } else if (currentGame == GAME_PING_PONG) {
    resetPingPong();
    return;
  }
  
  snakeLength = INITIAL_SNAKE_LENGTH;
  dirX = 1;
  dirY = 0;
  directionChanged = false;
  
  obstaclesCount = 0;
  
  int centerX = (SCREEN_WIDTH / 2) - ((SCREEN_WIDTH / 2) % SNAKE_SIZE);
  int centerY = (SCREEN_HEIGHT / 2) - ((SCREEN_HEIGHT / 2) % SNAKE_SIZE);
  
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = centerX - (i * SNAKE_SIZE);
    snakeY[i] = centerY;
    
    if (snakeX[i] < BORDER_LEFT) snakeX[i] = BORDER_LEFT;
    if (snakeX[i] > BORDER_RIGHT) snakeX[i] = BORDER_RIGHT;
    if (snakeY[i] < BORDER_TOP) snakeY[i] = BORDER_TOP;
    if (snakeY[i] > BORDER_BOTTOM) snakeY[i] = BORDER_BOTTOM;
  }
  
  if (currentMode == HARD_MODE) {
    generateObstacles();
  }
  
  generateFood();
}

// ============ FLAPPY BIRD FUNCTIONS ============

void resetFlappyBird() {
  birdY = SCREEN_HEIGHT / 2;
  birdVelocity = 0;
  birdFlapping = false;
  
  for (int i = 0; i < MAX_PIPES; i++) {
    pipeActive[i] = false;
    pipePassed[i] = false;
  }
  
  generatePipe(0);
  score = 0;
}

void generatePipe(int index) {
  pipeX[index] = SCREEN_WIDTH + 10;
  
  int minGap, maxGap;
  switch(currentMode) {
    case EASY_MODE:
      minGap = 18;
      maxGap = FLAPPY_GROUND_Y - 18;
      break;
    case NORMAL_MODE:
      minGap = 16;
      maxGap = FLAPPY_GROUND_Y - 16;
      break;
    case HARD_MODE:
      minGap = 14;
      maxGap = FLAPPY_GROUND_Y - 14;
      break;
    default:
      minGap = 16;
      maxGap = FLAPPY_GROUND_Y - 16;
  }
  
  pipeGapY[index] = random(minGap, maxGap);
  pipeActive[index] = true;
  pipePassed[index] = false;
}

void updateFlappyBird() {
  float gravityMultiplier = 1.0f;
  if (currentMode == HARD_MODE) gravityMultiplier = 1.2f;
  
  birdVelocity += GRAVITY * gravityMultiplier;
  birdY += birdVelocity;
  
  if (birdY < 0) {
    birdY = 0;
    birdVelocity = 0;
  }
  
  if (birdY > FLAPPY_GROUND_Y - BIRD_HEIGHT) {
    birdY = FLAPPY_GROUND_Y - BIRD_HEIGHT;
    gameState = GAME_OVER;
    return;
  }
  
  int pipeSpeed = PIPE_SPEED;
  if (currentMode == HARD_MODE) pipeSpeed = 3;
  else if (currentMode == EASY_MODE) pipeSpeed = 1;
  
  int currentPipeGap = PIPE_GAP;
  if (currentMode == EASY_MODE) currentPipeGap = 28;
  else if (currentMode == HARD_MODE) currentPipeGap = 18;
  
  for (int i = 0; i < MAX_PIPES; i++) {
    if (pipeActive[i]) {
      pipeX[i] -= pipeSpeed;
      
      if (pipeX[i] < -PIPE_WIDTH) {
        pipeActive[i] = false;
      }
      
      if (!pipePassed[i] && pipeX[i] + PIPE_WIDTH < BIRD_X) {
        pipePassed[i] = true;
        score++;
        
        int* currentHighScore = getCurrentHighScorePtr();
        if (score > *currentHighScore) {
          *currentHighScore = score;
          saveHighScores();
        }
      }
      
      if (pipeX[i] < BIRD_X + BIRD_WIDTH && pipeX[i] + PIPE_WIDTH > BIRD_X) {
        int gapTop = pipeGapY[i] - currentPipeGap / 2;
        int gapBottom = pipeGapY[i] + currentPipeGap / 2;
        
        if (birdY < gapTop || birdY + BIRD_HEIGHT > gapBottom) {
          gameState = GAME_OVER;
          return;
        }
      }
    }
  }
  
  int pipeSpacing = 60;
  if (currentMode == EASY_MODE) pipeSpacing = 80;
  else if (currentMode == HARD_MODE) pipeSpacing = 50;
  
  int rightmostX = 0;
  for (int i = 0; i < MAX_PIPES; i++) {
    if (pipeActive[i] && pipeX[i] > rightmostX) {
      rightmostX = pipeX[i];
    }
  }
  
  if (rightmostX < SCREEN_WIDTH - pipeSpacing) {
    for (int i = 0; i < MAX_PIPES; i++) {
      if (!pipeActive[i]) {
        generatePipe(i);
        break;
      }
    }
  }
}

void drawFlappyBird() {
  u8g2.clearBuffer();
  
  u8g2.setFont(u8g2_font_7x13_tf);
  char scoreStr[20];
  sprintf(scoreStr, "%d", score);
  int scoreWidth = u8g2.getStrWidth(scoreStr);
  u8g2.drawStr((SCREEN_WIDTH - scoreWidth) / 2, 10, scoreStr);
  
  u8g2.setFont(u8g2_font_6x10_tf);
  sprintf(scoreStr, "H:%d", getCurrentHighScore());
  int highWidth = u8g2.getStrWidth(scoreStr);
  u8g2.drawStr(SCREEN_WIDTH - highWidth - 2, 10, scoreStr);
  
  int currentPipeGap = PIPE_GAP;
  if (currentMode == EASY_MODE) currentPipeGap = 28;
  else if (currentMode == HARD_MODE) currentPipeGap = 18;
  
  for (int i = 0; i < MAX_PIPES; i++) {
    if (pipeActive[i]) {
      int gapTop = pipeGapY[i] - currentPipeGap / 2;
      int gapBottom = pipeGapY[i] + currentPipeGap / 2;
      
      if (gapTop > 0) {
        u8g2.drawBox(pipeX[i], 0, PIPE_WIDTH, gapTop);
        u8g2.drawBox(pipeX[i] - 2, gapTop - 4, PIPE_WIDTH + 4, 4);
      }
      
      if (gapBottom < FLAPPY_GROUND_Y) {
        u8g2.drawBox(pipeX[i], gapBottom, PIPE_WIDTH, FLAPPY_GROUND_Y - gapBottom);
        u8g2.drawBox(pipeX[i] - 2, gapBottom, PIPE_WIDTH + 4, 4);
      }
    }
  }
  
  u8g2.drawLine(0, FLAPPY_GROUND_Y, SCREEN_WIDTH, FLAPPY_GROUND_Y);
  for (int x = 0; x < SCREEN_WIDTH; x += 8) {
    u8g2.drawLine(x, FLAPPY_GROUND_Y + 2, x + 4, SCREEN_HEIGHT);
  }
  
  int birdIntY = (int)birdY;
  
  u8g2.drawBox(BIRD_X + 1, birdIntY + 1, BIRD_WIDTH - 2, BIRD_HEIGHT - 2);
  u8g2.drawPixel(BIRD_X, birdIntY + 2);
  u8g2.drawPixel(BIRD_X, birdIntY + 3);
  u8g2.drawPixel(BIRD_X + BIRD_WIDTH - 1, birdIntY + 2);
  u8g2.drawPixel(BIRD_X + BIRD_WIDTH - 1, birdIntY + 3);
  
  u8g2.setDrawColor(0);
  u8g2.drawPixel(BIRD_X + BIRD_WIDTH - 3, birdIntY + 2);
  u8g2.setDrawColor(1);
  
  u8g2.drawTriangle(
    BIRD_X + BIRD_WIDTH - 1, birdIntY + 3,
    BIRD_X + BIRD_WIDTH + 3, birdIntY + 4,
    BIRD_X + BIRD_WIDTH - 1, birdIntY + 5
  );
  
  if (birdVelocity < 0) {
    u8g2.drawLine(BIRD_X + 2, birdIntY + 2, BIRD_X - 1, birdIntY);
    u8g2.drawLine(BIRD_X + 3, birdIntY + 2, BIRD_X, birdIntY);
  } else {
    u8g2.drawLine(BIRD_X + 2, birdIntY + 4, BIRD_X - 1, birdIntY + 6);
    u8g2.drawLine(BIRD_X + 3, birdIntY + 4, BIRD_X, birdIntY + 6);
  }
}

// ============ PING PONG FUNCTIONS ============

void resetPingPong() {
  pingBallX = SCREEN_WIDTH / 2;
  pingBallY = SCREEN_HEIGHT / 2;
  pingBallSpeedX = 2.0;
  pingBallSpeedY = 1.5;
  leftPaddleY = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
  rightPaddleY = SCREEN_HEIGHT / 2 - PADDLE_HEIGHT / 2;
  pingLifeScore = 0;
  pingPlayerScore = 0;
  score = 0; // Reset game score for high score tracking
}

void resetBall() {
  pingBallX = SCREEN_WIDTH / 2;
  pingBallY = SCREEN_HEIGHT / 2;
  
  // Set initial speed based on difficulty
  float baseSpeed = 2.0;
  switch(currentMode) {
    case EASY_MODE:
      baseSpeed = 1.8;
      break;
    case NORMAL_MODE:
      baseSpeed = 2.2;
      break;
    case HARD_MODE:
      baseSpeed = 2.8;
      break;
  }
  
  // Random direction with slight bias
  pingBallSpeedX = (random(0, 2) == 0) ? baseSpeed : -baseSpeed;
  pingBallSpeedY = (random(0, 2) == 0) ? random(8, 15) / 10.0 : -random(8, 15) / 10.0;
}

void controlLife() {
  // LIFE controls left paddle - IMPROVED PHYSICS
  int lifeReaction;
  float lifeSpeed;
  
  switch(currentMode) {
    case EASY_MODE:
      lifeReaction = LIFE_REACTION_EASY;
      lifeSpeed = 2.5;
      break;
    case NORMAL_MODE:
      lifeReaction = LIFE_REACTION_NORMAL;
      lifeSpeed = 3.0;
      break;
    case HARD_MODE:
      lifeReaction = LIFE_REACTION_HARD;
      lifeSpeed = 3.5;
      break;
  }
  
  // Predict ball position for better AI
  int predictedBallY = pingBallY;
  if (pingBallSpeedX < 0) {  // Ball coming towards LIFE
    // Estimate where ball will be when it reaches paddle
    float timeToPaddle = (pingBallX - PLAY_AREA_LEFT) / abs(pingBallSpeedX);
    predictedBallY = pingBallY + (pingBallSpeedY * timeToPaddle);
    
    // Keep prediction within bounds
    predictedBallY = constrain(predictedBallY, PLAY_AREA_TOP, PLAY_AREA_BOTTOM - BALL_SIZE);
  }
  
  int paddleCenter = leftPaddleY + PADDLE_HEIGHT / 2;
  int targetY = predictedBallY - PADDLE_HEIGHT / 2;
  
  // Move towards target position
  if (abs(paddleCenter - predictedBallY) > lifeReaction) {
    if (paddleCenter < predictedBallY) {
      leftPaddleY += lifeSpeed;
    } else {
      leftPaddleY -= lifeSpeed;
    }
  }
  
  // Clamp LIFE paddle position with buffer
  int topBuffer = PLAY_AREA_TOP + 2;
  int bottomBuffer = PLAY_AREA_BOTTOM - PADDLE_HEIGHT - 2;
  leftPaddleY = constrain(leftPaddleY, topBuffer, bottomBuffer);
}

void updatePingPong() {
  // Control LIFE
  controlLife();
  
  // Move ball with floating point precision
  pingBallX += pingBallSpeedX;
  pingBallY += pingBallSpeedY;
  
  // Ball collision with top and bottom - FIXED BOUNDARIES
  if (pingBallY <= PLAY_AREA_TOP) {
    pingBallY = PLAY_AREA_TOP;
    pingBallSpeedY = abs(pingBallSpeedY);  // Bounce down
  } else if (pingBallY + BALL_SIZE >= PLAY_AREA_BOTTOM) {
    pingBallY = PLAY_AREA_BOTTOM - BALL_SIZE;
    pingBallSpeedY = -abs(pingBallSpeedY);  // Bounce up
  }
  
  // Ball collision with left paddle (LIFE) - IMPROVED DETECTION
  if (pingBallX <= PLAY_AREA_LEFT + PADDLE_WIDTH && pingBallX >= PLAY_AREA_LEFT) {
    if (pingBallY + BALL_SIZE >= leftPaddleY && pingBallY <= leftPaddleY + PADDLE_HEIGHT) {
      pingBallX = PLAY_AREA_LEFT + PADDLE_WIDTH;  // Move ball to right of paddle
      pingBallSpeedX = abs(pingBallSpeedX);  // Always bounce right
      
      // Add spin based on hit position - more controlled
      float hitPosition = (pingBallY + BALL_SIZE/2) - (leftPaddleY + PADDLE_HEIGHT/2);
      float spinFactor = hitPosition / (PADDLE_HEIGHT/2) * 0.3;
      pingBallSpeedY += spinFactor;
      
      // Speed up based on difficulty
      float speedIncrease = 1.05;
      if (currentMode == HARD_MODE) speedIncrease = 1.08;
      
      pingBallSpeedX *= speedIncrease;
      pingBallSpeedY = constrain(pingBallSpeedY, -3.0, 3.0);
    }
  }
  
  // Ball collision with right paddle (Player) - IMPROVED DETECTION
  if (pingBallX + BALL_SIZE >= PLAY_AREA_RIGHT - PADDLE_WIDTH && pingBallX <= PLAY_AREA_RIGHT) {
    if (pingBallY + BALL_SIZE >= rightPaddleY && pingBallY <= rightPaddleY + PADDLE_HEIGHT) {
      pingBallX = PLAY_AREA_RIGHT - PADDLE_WIDTH - BALL_SIZE;  // Move ball to left of paddle
      pingBallSpeedX = -abs(pingBallSpeedX);  // Always bounce left
      
      // Add spin based on hit position
      float hitPosition = (pingBallY + BALL_SIZE/2) - (rightPaddleY + PADDLE_HEIGHT/2);
      float spinFactor = hitPosition / (PADDLE_HEIGHT/2) * 0.3;
      pingBallSpeedY += spinFactor;
      
      // Speed up based on difficulty
      float speedIncrease = 1.05;
      if (currentMode == HARD_MODE) speedIncrease = 1.08;
      
      pingBallSpeedX *= speedIncrease;
      pingBallSpeedY = constrain(pingBallSpeedY, -3.0, 3.0);
    }
  }
  
  // Score detection - FIXED GOAL DETECTION
  // Player scores when ball goes past LEFT side (LIFE misses)
  if (pingBallX < 0) {
    pingPlayerScore++;
    score = pingPlayerScore; // Update game score for high score tracking
    resetBall();
    checkPingPongGameOver();
    return;  // Exit early to prevent double scoring
  }
  
  // LIFE scores when ball goes past RIGHT side (Player misses)
  if (pingBallX > SCREEN_WIDTH) {
    pingLifeScore++;
    score = pingPlayerScore; // Keep player score for tracking
    resetBall();
    checkPingPongGameOver();
    return;  // Exit early to prevent double scoring
  }
}

void checkPingPongGameOver() {
  int winningScore = 5;
  
  if (pingLifeScore >= winningScore || pingPlayerScore >= winningScore) {
    // Update high score if player wins
    if (pingPlayerScore > pingLifeScore) {
      int* currentHighScore = getCurrentHighScorePtr();
      if (score > *currentHighScore) {
        *currentHighScore = score;
        saveHighScores();
      }
    }
    gameState = GAME_OVER;
  }
}

void drawPingPong() {
  u8g2.clearBuffer();
  
  // Draw border
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  // Draw scores with labels
  u8g2.setFont(u8g2_font_7x13_tf);
  char scoreStr[20];
  
  // LIFE score (left)
  sprintf(scoreStr, "LIFE: %d", pingLifeScore);
  u8g2.drawStr(10, 10, scoreStr);
  
  // Player score (right)
  sprintf(scoreStr, "YOU: %d", pingPlayerScore);
  int scoreWidth = u8g2.getStrWidth(scoreStr);
  u8g2.drawStr(SCREEN_WIDTH - scoreWidth - 10, 10, scoreStr);
  
  // High score Removed as per request
  
  // Draw center line
  for (int y = PLAY_AREA_TOP; y < PLAY_AREA_BOTTOM; y += 4) {
    u8g2.drawPixel(SCREEN_WIDTH / 2, y);
  }
  
  // Draw play area boundaries (visual guide)
  u8g2.drawLine(PLAY_AREA_LEFT, PLAY_AREA_TOP, PLAY_AREA_LEFT, PLAY_AREA_BOTTOM);
  u8g2.drawLine(PLAY_AREA_RIGHT, PLAY_AREA_TOP, PLAY_AREA_RIGHT, PLAY_AREA_BOTTOM);
  
  // Draw paddles
  u8g2.drawBox(PLAY_AREA_LEFT, leftPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT);  // LIFE paddle
  u8g2.drawBox(PLAY_AREA_RIGHT - PADDLE_WIDTH, rightPaddleY, PADDLE_WIDTH, PADDLE_HEIGHT);  // Player paddle
  
  // Draw ball
  u8g2.drawBox((int)pingBallX, (int)pingBallY, BALL_SIZE, BALL_SIZE);
  
  // Draw difficulty indicator
  u8g2.setFont(u8g2_font_5x7_tf);
  switch(currentMode) {
    case EASY_MODE: u8g2.drawStr(SCREEN_WIDTH/2 - 10, 72, "EASY"); break;
    case NORMAL_MODE: u8g2.drawStr(SCREEN_WIDTH/2 - 15, 72, "NORMAL"); break;
    case HARD_MODE: u8g2.drawStr(SCREEN_WIDTH/2 - 10, 72, "HARD"); break;
  }
}

// ============ EEPROM AND HIGH SCORE FUNCTIONS ============

void loadHighScores() {
  int magicValue;
  EEPROM.get(EEPROM_MAGIC_ADDR, magicValue);
  
  if (magicValue == EEPROM_MAGIC_VALUE) {
    EEPROM.get(EEPROM_EASY_ADDR, highScoreEasy);
    EEPROM.get(EEPROM_NORMAL_ADDR, highScoreNormal);
    EEPROM.get(EEPROM_HARD_ADDR, highScoreHard);

    EEPROM.get(EEPROM_FLAPPY_EASY_ADDR, flappyHighScoreEasy);
    EEPROM.get(EEPROM_FLAPPY_NORMAL_ADDR, flappyHighScoreNormal);
    EEPROM.get(EEPROM_FLAPPY_HARD_ADDR, flappyHighScoreHard);

    EEPROM.get(EEPROM_PING_EASY_ADDR, pingPongHighScoreEasy);
    EEPROM.get(EEPROM_PING_NORMAL_ADDR, pingPongHighScoreNormal);
    EEPROM.get(EEPROM_PING_HARD_ADDR, pingPongHighScoreHard);

    if (highScoreEasy < 0 || highScoreEasy > 99999) highScoreEasy = 0;
    if (highScoreNormal < 0 || highScoreNormal > 99999) highScoreNormal = 0;
    if (highScoreHard < 0 || highScoreHard > 99999) highScoreHard = 0;
    if (flappyHighScoreEasy < 0 || flappyHighScoreEasy > 99999) flappyHighScoreEasy = 0;
    if (flappyHighScoreNormal < 0 || flappyHighScoreNormal > 99999) flappyHighScoreNormal = 0;
    if (flappyHighScoreHard < 0 || flappyHighScoreHard > 99999) flappyHighScoreHard = 0;
    if (pingPongHighScoreEasy < 0 || pingPongHighScoreEasy > 99999) pingPongHighScoreEasy = 0;
    if (pingPongHighScoreNormal < 0 || pingPongHighScoreNormal > 99999) pingPongHighScoreNormal = 0;
    if (pingPongHighScoreHard < 0 || pingPongHighScoreHard > 99999) pingPongHighScoreHard = 0;
    Serial.println("High scores loaded from EEPROM");
  } else {
    highScoreEasy = 0;
    highScoreNormal = 0;
    highScoreHard = 0;
    flappyHighScoreEasy = 0;
    flappyHighScoreNormal = 0;
    flappyHighScoreHard = 0;
    pingPongHighScoreEasy = 0;
    pingPongHighScoreNormal = 0;
    pingPongHighScoreHard = 0;
    
    EEPROM.put(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_VALUE);
    EEPROM.put(EEPROM_EASY_ADDR, highScoreEasy);
    EEPROM.put(EEPROM_NORMAL_ADDR, highScoreNormal);
    EEPROM.put(EEPROM_HARD_ADDR, highScoreHard);

    EEPROM.put(EEPROM_FLAPPY_EASY_ADDR, flappyHighScoreEasy);
    EEPROM.put(EEPROM_FLAPPY_NORMAL_ADDR, flappyHighScoreNormal);
    EEPROM.put(EEPROM_FLAPPY_HARD_ADDR, flappyHighScoreHard);
    
    EEPROM.put(EEPROM_PING_EASY_ADDR, pingPongHighScoreEasy);
    EEPROM.put(EEPROM_PING_NORMAL_ADDR, pingPongHighScoreNormal);
    EEPROM.put(EEPROM_PING_HARD_ADDR, pingPongHighScoreHard);
    EEPROM.commit();
    
    Serial.println("EEPROM initialized with default high scores");
  }
}

void saveHighScores() {
  EEPROM.put(EEPROM_EASY_ADDR, highScoreEasy);
  EEPROM.put(EEPROM_NORMAL_ADDR, highScoreNormal);
  EEPROM.put(EEPROM_HARD_ADDR, highScoreHard);

  EEPROM.put(EEPROM_FLAPPY_EASY_ADDR, flappyHighScoreEasy);
  EEPROM.put(EEPROM_FLAPPY_NORMAL_ADDR, flappyHighScoreNormal);
  EEPROM.put(EEPROM_FLAPPY_HARD_ADDR, flappyHighScoreHard);

  EEPROM.put(EEPROM_PING_EASY_ADDR, pingPongHighScoreEasy);
  EEPROM.put(EEPROM_PING_NORMAL_ADDR, pingPongHighScoreNormal);
  EEPROM.put(EEPROM_PING_HARD_ADDR, pingPongHighScoreHard);
  EEPROM.commit();
  
  Serial.println("High scores saved to EEPROM");
}

int getCurrentHighScore() {
  if (currentGame == GAME_FLAPPY) {
    switch(currentMode) {
      case EASY_MODE: return flappyHighScoreEasy;
      case NORMAL_MODE: return flappyHighScoreNormal;
      case HARD_MODE: return flappyHighScoreHard;
      default: return 0;
    }
  } else if (currentGame == GAME_PING_PONG) {
    switch(currentMode) {
      case EASY_MODE: return pingPongHighScoreEasy;
      case NORMAL_MODE: return pingPongHighScoreNormal;
      case HARD_MODE: return pingPongHighScoreHard;
      default: return 0;
    }
  }
  
  switch(currentMode) {
    case EASY_MODE: return highScoreEasy;
    case NORMAL_MODE: return highScoreNormal;
    case HARD_MODE: return highScoreHard;
    default: return 0;
  }
}

int* getCurrentHighScorePtr() {
  if (currentGame == GAME_FLAPPY) {
    switch(currentMode) {
      case EASY_MODE: return &flappyHighScoreEasy;
      case NORMAL_MODE: return &flappyHighScoreNormal;
      case HARD_MODE: return &flappyHighScoreHard;
      default: return &flappyHighScoreNormal;
    }
  } else if (currentGame == GAME_PING_PONG) {
    switch(currentMode) {
      case EASY_MODE: return &pingPongHighScoreEasy;
      case NORMAL_MODE: return &pingPongHighScoreNormal;
      case HARD_MODE: return &pingPongHighScoreHard;
      default: return &pingPongHighScoreNormal;
    }
  }
  
  switch(currentMode) {
    case EASY_MODE: return &highScoreEasy;
    case NORMAL_MODE: return &highScoreNormal;
    case HARD_MODE: return &highScoreHard;
    default: return &highScoreNormal;
  }
}

// ============ CALCULATOR FUNCTIONS ============

void resetCalculator() {
  calcInput = "0";
  firstNum = 0;
  calcOperation = 0;
  newEntry = true;
  calcCursorX = 0;
  calcCursorY = 0;
}

void handleCalculatorInput() {
  unsigned long now = millis();
  
  // Handle joystick navigation for grid (Uses own analog reading for grid)
  if (now - lastCalcMove > MOVE_DELAY) {
    int x = analogRead(VRX_PIN);
    int y = analogRead(VRY_PIN);
    bool moved = false;
    
    // X Axis (Left/Right)
    if (x < (CENTER_X - THRESHOLD_X)) { 
      calcCursorX--; 
      if (calcCursorX < 0) calcCursorX = 3; 
      moved = true;
    } 
    else if (x > (CENTER_X + THRESHOLD_X)) { 
      calcCursorX++; 
      if (calcCursorX > 3) calcCursorX = 0;
      moved = true;
    }
    
    // Y Axis (Up/Down)
    if (y < (CENTER_Y - THRESHOLD_Y)) { 
      calcCursorY--; 
      if (calcCursorY < 0) calcCursorY = 3;
      moved = true;
    } 
    else if (y > (CENTER_Y + THRESHOLD_Y)) { 
      calcCursorY++; 
      if (calcCursorY > 3) calcCursorY = 0;
      moved = true;
    }
    
    if (moved) lastCalcMove = now;
  }
  
  // FIX: Use the global debounced button flags instead of raw digitalRead
  // This prevents "automatic writing" and ghost inputs
  if (buttonPressed || actionPressed) {
    // Determine which character was selected
    char key = calcKeys[calcCursorY][calcCursorX];
    
    // NUMBER KEYS (0-9)
    if (key >= '0' && key <= '9') {
      if (newEntry) {
        calcInput = "";
        newEntry = false;
      }
      if (calcInput.length() < 10) {
        if (calcInput == "0") calcInput = String(key);
        else calcInput += key;
      }
    }
    // CLEAR KEY (C)
    else if (key == 'C') {
      calcInput = "0";
      firstNum = 0;
      calcOperation = 0;
      newEntry = true;
    }
    // EQUALS KEY (=)
    else if (key == '=') {
      performCalculation();
      calcOperation = 0;
      newEntry = true;
    }
    // OPERATORS (+ - * /)
    else { 
      if (calcOperation != 0) {
        performCalculation();
      }
      firstNum = calcInput.toFloat();
      calcOperation = key;
      newEntry = true;
    }
      
    // Consume the button presses
    buttonPressed = false;
    actionPressed = false;
    delay(DEBOUNCE_DELAY);
  }
}

void performCalculation() {
  float secondNum = calcInput.toFloat();
  float result = 0;
  
  switch (calcOperation) {
    case '+': result = firstNum + secondNum; break;
    case '-': result = firstNum - secondNum; break;
    case '*': result = firstNum * secondNum; break;
    case '/': 
      if (secondNum != 0) result = firstNum / secondNum; 
      else result = 0; 
      break;
    default: return; 
  }
  
  // Remove decimal point if number is whole
  if (result == (int)result) {
    calcInput = String((int)result);
  } else {
    calcInput = String(result, 2);
  }
  
  firstNum = result;
}

void drawCalculator() {
  u8g2.clearBuffer();
  
  // Draw border
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  // Uplifted display and grid to fit on screen
  // Display area moved to top (y=2 to y=17)
  u8g2.drawFrame(2, 2, 124, 15);
  
  // Draw active operator
  if (calcOperation != 0) {
    u8g2.setFont(u8g2_font_6x10_tf);
    char opStr[2] = {calcOperation, '\0'};
    u8g2.drawStr(6, 13, opStr);
  }

  // Draw numbers (right aligned)
  u8g2.setFont(u8g2_font_9x15_tf); 
  int textWidth = u8g2.getStrWidth(calcInput.c_str());
  int xPos = 122 - textWidth; 
  if (xPos < 20) xPos = 20;    
  u8g2.drawStr(xPos, 14, calcInput.c_str());

  // Draw keypad grid (shifted up to y=19)
  u8g2.setFont(u8g2_font_6x10_tf);
  
  int startX = 14;
  int startY = 19;
  int btnW = 24;
  int btnH = 9;
  int gap = 1;
  
  for (int row = 0; row < 4; row++) {
    for (int col = 0; col < 4; col++) {
      int x = startX + (col * (btnW + gap));
      int y = startY + (row * (btnH + gap));
      
      // Draw button
      if (row == calcCursorY && col == calcCursorX) {
        u8g2.drawBox(x, y, btnW, btnH);
        u8g2.setDrawColor(0);
      } else {
        u8g2.drawFrame(x, y, btnW, btnH);
        u8g2.setDrawColor(1);
      }
      
      // Draw character
      char label[2] = {calcKeys[row][col], '\0'};
      int charW = u8g2.getStrWidth(label);
      u8g2.drawStr(x + (btnW - charW)/2, y + 8, label);
      
      u8g2.setDrawColor(1);
    }
  }
  
  u8g2.sendBuffer();
}

// ============ DATE FUNCTIONS ============

bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int getDaysInMonth(int month, int year) {
  if (month == 2) return isLeapYear(year) ? 29 : 28;
  if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
  return 31;
}

// Calculate Day of Week (0=Sunday, 1=Monday...)
int getDayOfWeek(int d, int m, int y) {
  if (m < 3) { m += 12; y -= 1; }
  int K = y % 100;
  int J = y / 100;
  int f = d + 13 * (m + 1) / 5 + K + K / 4 + J / 4 + 5 * J;
  // Adjusted Zeller's to match 0=Sunday, 1=Monday...
  return (f + 6) % 7; 
}

void handleDateInput() {
  unsigned long now = millis();
  if (now - lastCalendarMove < CALENDAR_MOVE_DELAY) return;

  int x = analogRead(VRX_PIN); // Pin 32
  int y = analogRead(VRY_PIN); // Pin 33
  
  bool moved = false;

  // Horizontal: Change Day
  if (x < (CENTER_X - THRESHOLD_X)) { // LEFT: Previous Day
    calDay--;
    moved = true;
  } else if (x > (CENTER_X + THRESHOLD_X)) { // RIGHT: Next Day
    calDay++;
    moved = true;
  }

  // Vertical: Change Month
  if (y < (CENTER_Y - THRESHOLD_Y)) { // UP: Previous Month
    calMonth--;
    moved = true;
  } else if (y > (CENTER_Y + THRESHOLD_Y)) { // DOWN: Next Month
    calMonth++;
    moved = true;
  }

  // Logic to handle date rollover
  if (moved) {
    if (calMonth > 12) { calMonth = 1; calYear++; }
    if (calMonth < 1) { calMonth = 12; calYear--; }

    int maxDays = getDaysInMonth(calMonth, calYear);
    if (calDay > maxDays) calDay = 1;
    if (calDay < 1) calDay = maxDays;

    lastCalendarMove = now;
  }
}

void drawDateScreen() {
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

  // 1. Calculate Day of Week
  int dow = getDayOfWeek(calDay, calMonth, calYear);
  const char* fullDayName;
  switch(dow) {
    case 0: fullDayName = "SUNDAY"; break;
    case 1: fullDayName = "MONDAY"; break;
    case 2: fullDayName = "TUESDAY"; break;
    case 3: fullDayName = "WEDNESDAY"; break;
    case 4: fullDayName = "THURSDAY"; break;
    case 5: fullDayName = "FRIDAY"; break;
    case 6: fullDayName = "SATURDAY"; break;
  }

  // 2. Display Day of Week (LARGE)
  // Use a large font for the Day Name
  u8g2.setFont(u8g2_font_7x13_tf);
  int dayWidth = u8g2.getStrWidth(fullDayName);
  u8g2.drawStr((SCREEN_WIDTH - dayWidth) / 2, 20, fullDayName);

  // 4. Draw Line
  u8g2.drawLine(20, 25, SCREEN_WIDTH - 20, 25);

  // 3. Display Date (Middle)
  char dateLine[30];
  sprintf(dateLine, "%02d %s %d", calDay, monthNames[calMonth], calYear);
  u8g2.setFont(u8g2_font_6x12_tf);
  int dateWidth = u8g2.getStrWidth(dateLine);
  u8g2.drawStr((SCREEN_WIDTH - dateWidth) / 2, 40, dateLine);
  
  // 5. Draw controls hint
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(15, 53, "Joystick: Change Date");
  u8g2.drawStr(30, 61, "HOME: Main Menu");
}

// ============ STOPWATCH FUNCTIONS ============

void handleStopwatchInput() {
  // ACTION button (pin 17) - START/STOP
  if (actionPressed) {
    if (!stopwatchRunning) {
      // Start or resume
      if (stopwatchPausedTime == 0) {
        // First start
        stopwatchStartTime = millis();
      } else {
        // Resume from paused state
        stopwatchStartTime = millis() - stopwatchPausedTime;
        stopwatchPausedTime = 0;
      }
      stopwatchRunning = true;
    } else {
      // Stop
      stopwatchRunning = false;
      stopwatchPausedTime = millis() - stopwatchStartTime;
    }
    actionPressed = false;
    delay(200);
  }
  
  // SW button (pin 25) - RESET
  if (buttonPressed) {
    // Reset to zero
    stopwatchStartTime = 0;
    stopwatchPausedTime = 0;
    stopwatchRunning = false;
    buttonPressed = false;
    delay(200);
  }
}

String formatTime(unsigned long milliseconds) {
  unsigned long totalSeconds = milliseconds / 1000;
  unsigned long hours = totalSeconds / 3600;
  unsigned long minutes = (totalSeconds % 3600) / 60;
  unsigned long seconds = totalSeconds % 60;
  unsigned long hundredths = (milliseconds % 1000) / 10;
  
  char buffer[15];
  if (hours > 0) {
    sprintf(buffer, "%02lu:%02lu:%02lu.%02lu", hours, minutes, seconds, hundredths);
  } else {
    sprintf(buffer, "%02lu:%02lu.%02lu", minutes, seconds, hundredths);
  }
  return String(buffer);
}

void drawStopwatch() {
  u8g2.clearBuffer();
  
  // Draw border
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  // Draw title
  u8g2.setFont(u8g2_font_7x13_tf);
  int titleWidth = u8g2.getStrWidth("STOPWATCH");
  u8g2.drawStr((SCREEN_WIDTH - titleWidth) / 2, 15, "STOPWATCH");
  
  // Draw current time (large font)
  unsigned long currentTime;
  if (stopwatchRunning) {
    currentTime = millis() - stopwatchStartTime;
  } else {
    currentTime = stopwatchPausedTime;
  }
  
  String timeStr = formatTime(currentTime);
  
  // Use larger font for time display
  u8g2.setFont(u8g2_font_logisoso18_tf);
  int timeWidth = u8g2.getStrWidth(timeStr.c_str());
  u8g2.drawStr((SCREEN_WIDTH - timeWidth) / 2, 45, timeStr.c_str());
  
  // Draw status
  u8g2.setFont(u8g2_font_6x10_tf);
  if (stopwatchRunning) {
    u8g2.drawStr(50, 60, "RUNNING");
  } else if (stopwatchPausedTime > 0) {
    u8g2.drawStr(50, 60, "PAUSED");
  } else {
    u8g2.drawStr(50, 60, "READY");
  }
  
  // Draw button labels
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.drawStr(15, 72, "ACTION: START/STOP");
  u8g2.drawStr(35, 82, "SW: RESET");
  
  u8g2.sendBuffer();
}

// ============ DRAWING FUNCTIONS ============

void drawGame() {
  u8g2.clearBuffer();
  
  switch(gameState) {
    case HOME_SCREEN:
      drawHomeScreen();
      break;
    case MAIN_MENU:
      drawMainMenu();
      break;
    case GAME_SELECTION:
      drawGameSelection();
      break;
    case MODE_SELECTION:
      drawModeSelection();
      break;
    case PLAYING:
      drawPlaying();
      break;
    case PAUSED:
      drawPauseMenu();
      break;
    case GAME_OVER:
      drawGameOver();
      break;
    case ESPNOW_MENU:
      drawESPNowMenu();
      break;
    case STOPWATCH:
      drawStopwatch();
      break;
    case CALENDAR:
      drawDateScreen();
      break;
    case CALCULATOR:
      drawCalculator();
      break;
  }
  
  u8g2.sendBuffer();
}

void drawMainMenu() {
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  u8g2.setFont(u8g2_font_7x13_tf);
  int titleWidth = u8g2.getStrWidth("MAIN MENU");
  u8g2.drawStr((SCREEN_WIDTH - titleWidth) / 2, 17, "MAIN MENU"); // Shifted down ~5px
  
  int iconX = SCREEN_WIDTH / 2;
  int iconY = 33; // Shifted down ~5px
  
  switch(currentMenuOption) {
    case MENU_GAMES: {
      u8g2.drawFrame(iconX - 12, iconY - 8, 24, 16);
      u8g2.drawBox(iconX - 10, iconY - 3, 8, 6);
      u8g2.drawDisc(iconX + 5, iconY - 3, 3);
      u8g2.drawDisc(iconX + 5, iconY + 3, 3);
      u8g2.setFont(u8g2_font_6x10_tf);
      int gamesWidth = u8g2.getStrWidth("GAMES");
      u8g2.drawStr((SCREEN_WIDTH - gamesWidth) / 2, 53, "GAMES"); // Shifted down
      break;
    }
      
    case MENU_ESPNOW: {
      u8g2.drawCircle(iconX, iconY, 8);
      u8g2.drawCircle(iconX, iconY, 4);
      u8g2.setFont(u8g2_font_6x10_tf);
      int espnowWidth = u8g2.getStrWidth("ESP-NOW");
      u8g2.drawStr((SCREEN_WIDTH - espnowWidth) / 2, 53, "ESP-NOW"); // Shifted down
      break;
    }
      
    case MENU_STOPWATCH: {
      u8g2.drawCircle(iconX, iconY, 8);
      u8g2.drawDisc(iconX, iconY, 2);
      u8g2.drawLine(iconX, iconY, iconX, iconY - 5);
      u8g2.drawLine(iconX, iconY, iconX + 5, iconY);
      u8g2.setFont(u8g2_font_6x10_tf);
      int stopwatchWidth = u8g2.getStrWidth("STOPWATCH");
      u8g2.drawStr((SCREEN_WIDTH - stopwatchWidth) / 2, 53, "STOPWATCH"); // Shifted down
      break;
    }
      
    case MENU_CALENDAR: {
      u8g2.drawFrame(iconX - 10, iconY - 8, 20, 16);
      u8g2.drawBox(iconX - 10, iconY - 8, 20, 4);
      u8g2.drawBox(iconX - 8, iconY, 4, 4);
      u8g2.drawBox(iconX - 2, iconY, 4, 4);
      u8g2.drawBox(iconX + 4, iconY, 4, 4);
      u8g2.setFont(u8g2_font_6x10_tf);
      int calendarWidth = u8g2.getStrWidth("DATE");
      u8g2.drawStr((SCREEN_WIDTH - calendarWidth) / 2, 53, "DATE"); // Shifted down
      break;
    }
      
    case MENU_CALCULATOR: {
      u8g2.drawFrame(iconX - 10, iconY - 8, 20, 16);
      u8g2.drawBox(iconX - 8, iconY - 6, 16, 6);
      u8g2.drawFrame(iconX - 8, iconY + 2, 4, 4);
      u8g2.drawFrame(iconX - 2, iconY + 2, 4, 4);
      u8g2.drawFrame(iconX + 4, iconY + 2, 4, 4);
      u8g2.setFont(u8g2_font_6x10_tf);
      int calculatorWidth = u8g2.getStrWidth("CALCULATOR");
      u8g2.drawStr((SCREEN_WIDTH - calculatorWidth) / 2, 53, "CALCULATOR"); // Shifted down
      break;
    }
  }
  
  u8g2.setFont(u8g2_font_unifont_t_symbols);
  u8g2.drawGlyph(10, 41, 0x2190); // Shifted down
  u8g2.drawGlyph(SCREEN_WIDTH - 20, 41, 0x2192); // Shifted down
}

void drawGameSelection() {
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  u8g2.setFont(u8g2_font_7x13_tf);
  int titleWidth = u8g2.getStrWidth("SELECT GAME");
  u8g2.drawStr((SCREEN_WIDTH - titleWidth) / 2, 14, "SELECT GAME");
  
  u8g2.setFont(u8g2_font_6x10_tf);

  const char* gameTitles[] = {"SNAKE", "FLAPPY BIRD", "PING PONG"};
  int yPositions[] = {30, 45, 60}; // Fixed positions
  
  for(int i = 0; i < 3; i++) {
    if(i == selectedGame) {
      // Draw selection box
       u8g2.drawBox(10, yPositions[i] - 9, 108, 12);
       u8g2.setDrawColor(0); // Invert text
    } else {
       u8g2.setDrawColor(1);
    }
    
    int strWidth = u8g2.getStrWidth(gameTitles[i]);
    u8g2.drawStr((SCREEN_WIDTH - strWidth) / 2, yPositions[i], gameTitles[i]);
    u8g2.setDrawColor(1); // Reset
  }
}

void drawModeSelection() {
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  u8g2.setFont(u8g2_font_7x13_tf);
  const char* gameName;
  if (currentGame == GAME_SNAKE) gameName = "SNAKE";
  else if (currentGame == GAME_FLAPPY) gameName = "FLAPPY BIRD";
  else gameName = "PING PONG";
  
  int gameWidth = u8g2.getStrWidth(gameName);
  u8g2.drawStr((SCREEN_WIDTH - gameWidth) / 2, 14, gameName);
  
  u8g2.setFont(u8g2_font_6x10_tf);
  
  // Easy mode 
  if (selectedMode == 0) {
    u8g2.drawBox(45, 18, 40, 12);
    u8g2.setDrawColor(0);
    int easyWidth = u8g2.getStrWidth("EASY");
    u8g2.drawStr((SCREEN_WIDTH - easyWidth) / 2, 27, "EASY");
    u8g2.setDrawColor(1);
  } else {
    int easyWidth = u8g2.getStrWidth("EASY");
    u8g2.drawStr((SCREEN_WIDTH - easyWidth) / 2, 27, "EASY");
  }
  
  // Normal mode
  if (selectedMode == 1) {
    u8g2.drawBox(40, 32, 50, 12);
    u8g2.setDrawColor(0);
    int normalWidth = u8g2.getStrWidth("NORMAL");
    u8g2.drawStr((SCREEN_WIDTH - normalWidth) / 2, 41, "NORMAL");
    u8g2.setDrawColor(1);
  } else {
    int normalWidth = u8g2.getStrWidth("NORMAL");
    u8g2.drawStr((SCREEN_WIDTH - normalWidth) / 2, 41, "NORMAL");
  }
  
  // Hard mode
  if (selectedMode == 2) {
    u8g2.drawBox(45, 46, 40, 12);
    u8g2.setDrawColor(0);
    int hardWidth = u8g2.getStrWidth("HARD");
    u8g2.drawStr((SCREEN_WIDTH - hardWidth) / 2, 55, "HARD");
    u8g2.setDrawColor(1);
  } else {
    int hardWidth = u8g2.getStrWidth("HARD");
    u8g2.drawStr((SCREEN_WIDTH - hardWidth) / 2, 55, "HARD");
  }
}

void drawPlaying() {
  if (currentGame == GAME_FLAPPY) {
    drawFlappyBird();
    return;
  } else if (currentGame == GAME_PING_PONG) {
    drawPingPong();
    return;
  }
  
  // Draw Snake
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  u8g2.setFont(u8g2_font_7x13_tf);
  char scoreStr[20];
  sprintf(scoreStr, "S:%d", score);
  u8g2.drawStr(5, 10, scoreStr);
  
  sprintf(scoreStr, "H:%d", getCurrentHighScore());
  int textWidth = u8g2.getStrWidth(scoreStr);
  u8g2.drawStr(SCREEN_WIDTH - textWidth - 5, 10, scoreStr);
  
  if (currentMode == NORMAL_MODE || currentMode == HARD_MODE) {
    u8g2.drawFrame(BORDER_LEFT, BORDER_TOP, 
                   BORDER_RIGHT - BORDER_LEFT + SNAKE_SIZE, 
                   BORDER_BOTTOM - BORDER_TOP + SNAKE_SIZE);
  }
  
  if (currentMode == HARD_MODE) {
    if (obstaclesCount > 0) {
      for (int i = 0; i < obstaclesCount; i++) {
        u8g2.drawBox(obstaclesX[i], obstaclesY[i], OBSTACLE_SIZE, OBSTACLE_SIZE);
        
        u8g2.setDrawColor(0);
        for (int offset = -1; offset <= 1; offset++) {
          u8g2.drawLine(obstaclesX[i] + 2, obstaclesY[i] + 2 + offset, 
                        obstaclesX[i] + OBSTACLE_SIZE - 3, obstaclesY[i] + OBSTACLE_SIZE - 3 + offset);
          u8g2.drawLine(obstaclesX[i] + OBSTACLE_SIZE - 3, obstaclesY[i] + 2 + offset, 
                        obstaclesX[i] + 2, obstaclesY[i] + OBSTACLE_SIZE - 3 + offset);
        }
        u8g2.setDrawColor(1);
      }
    }
  }
  
  for (int i = 0; i < snakeLength; i++) {
    if (i == 0) {
      u8g2.drawBox(snakeX[i], snakeY[i], SNAKE_SIZE, SNAKE_SIZE);
      u8g2.setDrawColor(0);
      if (dirX == 1) {
        u8g2.drawPixel(snakeX[i] + SNAKE_SIZE - 1, snakeY[i] + 1);
        u8g2.drawPixel(snakeX[i] + SNAKE_SIZE - 1, snakeY[i] + SNAKE_SIZE - 2);
      } else if (dirX == -1) {
        u8g2.drawPixel(snakeX[i], snakeY[i] + 1);
        u8g2.drawPixel(snakeX[i], snakeY[i] + SNAKE_SIZE - 2);
      } else if (dirY == -1) {
        u8g2.drawPixel(snakeX[i] + 1, snakeY[i]);
        u8g2.drawPixel(snakeX[i] + SNAKE_SIZE - 2, snakeY[i]);
      } else if (dirY == 1) {
        u8g2.drawPixel(snakeX[i] + 1, snakeY[i] + SNAKE_SIZE - 1);
        u8g2.drawPixel(snakeX[i] + SNAKE_SIZE - 2, snakeY[i] + SNAKE_SIZE - 1);
      }
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawBox(snakeX[i], snakeY[i], SNAKE_SIZE, SNAKE_SIZE);
      u8g2.setDrawColor(0);
      u8g2.drawPixel(snakeX[i] + 1, snakeY[i] + 1);
      u8g2.setDrawColor(1);
    }
  }
  
  u8g2.drawCircle(foodX + SNAKE_SIZE/2, foodY + SNAKE_SIZE/2, SNAKE_SIZE/2);
}

void drawPauseMenu() {
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  u8g2.setFont(u8g2_font_7x13_tf);
  int pausedWidth = u8g2.getStrWidth("PAUSED");
  u8g2.drawStr((SCREEN_WIDTH - pausedWidth) / 2, 20, "PAUSED");
  
  u8g2.setFont(u8g2_font_6x10_tf);
  
  // Resume option
  if (pauseSelection == 0) {
    u8g2.drawBox(40, 32, 50, 12);
    u8g2.setDrawColor(0);
    int resumeWidth = u8g2.getStrWidth("RESUME");
    u8g2.drawStr((SCREEN_WIDTH - resumeWidth) / 2, 41, "RESUME");
    u8g2.setDrawColor(1);
  } else {
    int resumeWidth = u8g2.getStrWidth("RESUME");
    u8g2.drawStr((SCREEN_WIDTH - resumeWidth) / 2, 41, "RESUME");
  }
  
  // Menu option
  if (pauseSelection == 1) {
    u8g2.drawBox(45, 48, 40, 12);
    u8g2.setDrawColor(0);
    int menuWidth = u8g2.getStrWidth("MENU");
    u8g2.drawStr((SCREEN_WIDTH - menuWidth) / 2, 57, "MENU");
    u8g2.setDrawColor(1);
  } else {
    int menuWidth = u8g2.getStrWidth("MENU");
    u8g2.drawStr((SCREEN_WIDTH - menuWidth) / 2, 57, "MENU");
  }
}

void drawGameOver() {
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  if (gameOverState == GAME_OVER_SHOW_SCORES) {
    drawGameOverScores();
  } else {
    u8g2.setFont(u8g2_font_10x20_tf);
    
    int gameOverWidth = u8g2.getStrWidth("GAME OVER");
    int gameOverX = (SCREEN_WIDTH - gameOverWidth) / 2;
    int gameOverY = 30;
    
    for (int i = 0; i < gameOverFadeValue; i++) {
      u8g2.drawStr(gameOverX, gameOverY, "GAME OVER");
    }
  }
}

void drawGameOverScores() {
  int currentHighScore = getCurrentHighScore();
  
  u8g2.setFont(u8g2_font_7x13_tf);
  
  char levelStr[20];
  switch(currentMode) {
    case EASY_MODE: strcpy(levelStr, "LEVEL: EASY"); break;
    case NORMAL_MODE: strcpy(levelStr, "LEVEL: NORMAL"); break;
    case HARD_MODE: strcpy(levelStr, "LEVEL: HARD"); break;
  }
  
  int levelWidth = u8g2.getStrWidth(levelStr);
  u8g2.drawStr((SCREEN_WIDTH - levelWidth) / 2, 18, levelStr);
  
  char scoreStr[20];
  sprintf(scoreStr, "SCORE: %d", score);
  int scoreWidth = u8g2.getStrWidth(scoreStr);
  u8g2.drawStr((SCREEN_WIDTH - scoreWidth) / 2, 36, scoreStr);
  
  char highScoreStr[20];
  sprintf(highScoreStr, "HIGH: %d", currentHighScore);
  int highScoreWidth = u8g2.getStrWidth(highScoreStr);
  u8g2.drawStr((SCREEN_WIDTH - highScoreWidth) / 2, 54, highScoreStr);
}

void drawESPNowMenu() {
  u8g2.clearBuffer();
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  
  // --- HEADER ---
  u8g2.setFont(u8g2_font_7x13_tf);
  int titleW = u8g2.getStrWidth("RC CONTROL");
  u8g2.drawStr((SCREEN_WIDTH - titleW) / 2, 15, "RC CONTROL");

  // --- CONNECTION STATUS ---
  u8g2.setFont(u8g2_font_6x10_tf);
  if (connectionStatus) {
    // Draw Filled Box for Connected
    u8g2.drawBox(10, 22, 108, 14);
    u8g2.setDrawColor(0); // Black text
    int strW = u8g2.getStrWidth("CONNECTED");
    u8g2.drawStr((SCREEN_WIDTH - strW)/2, 33, "CONNECTED");
    u8g2.setDrawColor(1); // Reset to White
  } else {
    // Draw Empty Frame for Disconnected
    u8g2.drawFrame(10, 22, 108, 14);
    int strW = u8g2.getStrWidth("DISCONNECTED");
    u8g2.drawStr((SCREEN_WIDTH - strW)/2, 33, "DISCONNECTED");
  }

  // --- VISUALIZER ---
  // Draw a crosshair and a dot representing joystick position
  int centerX = SCREEN_WIDTH / 2;
  int centerY = 50;
  
  u8g2.drawLine(centerX - 10, centerY, centerX + 10, centerY); // Horizontal
  u8g2.drawLine(centerX, centerY - 10, centerX, centerY + 10); // Vertical
  
  // Calculate Dot Position for visualization (simple scaling)
  int dotX = map(espNowData.x, 1000, -1000, -10, 10);
  int dotY = map(espNowData.y, -1000, 1000, -10, 10);
  
  u8g2.drawDisc(centerX + dotX, centerY + dotY, 2);

  // --- FOOTER ---
  u8g2.setFont(u8g2_font_4x6_tf);
  u8g2.drawStr(5, 62, "BACK: EXIT");
}
void handleHomeScreen() {
  // If button press detected, go to MAIN_MENU
  if (buttonPressed || actionPressed) {
    gameState = MAIN_MENU;
    buttonPressed = false;
    actionPressed = false;
    delay(DEBOUNCE_DELAY);
  }
}

void drawHomeScreen() {
  u8g2.clearBuffer();
  
  // Outer Frame
  u8g2.drawFrame(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  u8g2.drawFrame(2, 2, SCREEN_WIDTH-4, SCREEN_HEIGHT-4);
  
  // Tech corners
  u8g2.drawBox(0, 0, 10, 10);
  u8g2.drawBox(SCREEN_WIDTH-10, 0, 10, 10);
  u8g2.drawBox(0, SCREEN_HEIGHT-10, 10, 10);
  u8g2.drawBox(SCREEN_WIDTH-10, SCREEN_HEIGHT-10, 10, 10);
  
  // Invert corners for contrast
  u8g2.setDrawColor(0);
  u8g2.drawPixel(2,2); u8g2.drawPixel(SCREEN_WIDTH-3,2);
  u8g2.drawPixel(2,SCREEN_HEIGHT-3); u8g2.drawPixel(SCREEN_WIDTH-3,SCREEN_HEIGHT-3);
  u8g2.setDrawColor(1);

  // PROJECT MPS text
  u8g2.setFont(u8g2_font_10x20_tf);
  int w = u8g2.getStrWidth("PROJECT MPS");
  u8g2.drawStr((SCREEN_WIDTH - w)/2, 35, "PROJECT MPS");
  
  // Subtext
  u8g2.setFont(u8g2_font_5x7_tf);
  const char* sub = "SYSTEM ONLINE";
  int sw = u8g2.getStrWidth(sub);
  u8g2.drawStr((SCREEN_WIDTH - sw)/2, 48, sub);
  
  // Blinking prompt
  if ((millis() / 500) % 2 == 0) {
     const char* msg = "[ PRESS ENTER ]";
     int mw = u8g2.getStrWidth(msg);
     u8g2.drawStr((SCREEN_WIDTH - mw)/2, 60, msg);
  }
}