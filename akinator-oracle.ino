/*
  ====================================================================
  AKINATOR ON NODEMCU  (101-character edition)
  ====================================================================
  Hardware:
    - NodeMCU (ESP8266)
    - 6x single-color LEDs (2 red, 2 green, 2 blue) -> shows game status
        Both RED anodes   -> D5 (GPIO14)
        Both GREEN anodes -> D6 (GPIO12)
        Both BLUE anodes  -> D7 (GPIO13)
        All 6 cathodes -> breadboard GND rail -> resistor -> NodeMCU GND
    - Ultrasonic sensors: NOT used in this build.

  How it works:
    1. NodeMCU creates its OWN WiFi network (an access point) - no router/
       internet needed. Network name: "Akinator-NodeMCU".
    2. Connect your phone/laptop to that network, then open
       http://192.168.4.1 (fixed address, also printed in Serial Monitor).
    3. First question is always "Is your character an idiot?" (easter egg -
       see below). After that, normal questions begin.
    4. After each answer, the ESP8266 re-scores all 101 characters and
       picks the single most "informative" next question (the one that
       splits the remaining candidates most evenly) - same strategy the
       real Akinator uses.
    5. When one character clearly stands out (or questions run low), it
       guesses. You confirm right/wrong on the page.
    6. RGB LED:
         BLUE (blinking)  = nobody connected to the WiFi yet
         BLUE (solid)     = game in progress / a question is being asked
         GREEN            = correct guess
         RED              = wrong guess / gave up

  EASTER EGGS:
    - First question is always "Is your character an idiot?"
        - Answer YES -> shows "Are you Likhith MS?" (both buttons say "Yes")
        - Answer NO  -> normal game proceeds as usual
    - If you spam the "No" button 5+ times within 3 seconds at any point,
      it also jumps to the "Are you Likhith MS?" screen.
  ====================================================================
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ---------------- WIFI ACCESS POINT SETTINGS ----------------
const char* AP_SSID = "Akinator-NodeMCU";
const char* AP_PASS = "akinator123"; // must be 8+ characters, or use "" for an open network

// ---------------- RGB LED PINS ----------------
#define PIN_RED   D5
#define PIN_GREEN D6
#define PIN_BLUE  D7

ESP8266WebServer server(80);

// ---- WiFi connection blink state ----
unsigned long lastBlinkTime = 0;
bool blinkState = false;
const unsigned long BLINK_INTERVAL_MS = 400;

// ---- No-spam detector state ----
#define NO_SPAM_WINDOW_MS 3000
#define NO_SPAM_THRESHOLD 5
unsigned long noClickTimes[16];
int noClickHead = 0;
int noClickCount = 0;

void recordNoClick() {
  unsigned long now = millis();
  noClickTimes[noClickHead] = now;
  noClickHead = (noClickHead + 1) % 16;
  if (noClickCount < 16) noClickCount++;
}

bool checkNoSpam() {
  unsigned long now = millis();
  int recent = 0;
  for (int i = 0; i < noClickCount; i++) {
    if (now - noClickTimes[i] <= NO_SPAM_WINDOW_MS) recent++;
  }
  return recent >= NO_SPAM_THRESHOLD;
}

// ==== AUTO-GENERATED ATTRIBUTE BITS ====
#define B_REAL_PERSON (1ULL << 0)
#define B_MALE (1ULL << 1)
#define B_FEMALE (1ULL << 2)
#define B_ANIMAL (1ULL << 3)
#define B_ROBOT (1ULL << 4)
#define B_CHILD (1ULL << 5)
#define B_INDIAN_SHOW (1ULL << 6)
#define B_JAPANESE_ANIME (1ULL << 7)
#define B_AMERICAN_COMIC (1ULL << 8)
#define B_VIDEO_GAME (1ULL << 9)
#define B_MYTHOLOGICAL (1ULL << 10)
#define B_SUPERPOWERS (1ULL << 11)
#define B_CAN_FLY (1ULL << 12)
#define B_STRONG_FIGHTER (1ULL << 13)
#define B_SWORD (1ULL << 14)
#define B_NINJA (1ULL << 15)
#define B_PIRATE (1ULL << 16)
#define B_MASK_HELMET (1ULL << 17)
#define B_GLASSES (1ULL << 18)
#define B_BLUE_THEMED (1ULL << 19)
#define B_GADGETS_TECH (1ULL << 20)
#define B_DEMON_SUPER (1ULL << 21)
#define B_VILLAIN (1ULL << 22)
#define B_COMIC_RELIEF (1ULL << 23)
#define B_SPORTSMAN (1ULL << 24)
#define B_SCIENTIST_LEADER (1ULL << 25)
#define B_SHONEN_PROTAGONIST (1ULL << 26)
#define B_SHINIGAMI (1ULL << 27)
#define B_ALCHEMIST (1ULL << 28)
#define B_TITAN_SHIFTER (1ULL << 29)
#define B_HERO_SOCIETY (1ULL << 30)
#define B_ASSASSIN_HUNTER (1ULL << 31)
#define B_ISEKAI (1ULL << 32)
#define B_SPY_FAMILY (1ULL << 33)
#define B_DEMON_SLAYER_CORPS (1ULL << 34)
#define B_PRO_HERO_STUDENT (1ULL << 35)
#define B_GENIUS_DETECTIVE (1ULL << 36)
#define B_DEVIL_FRUIT (1ULL << 37)
#define B_HOKAGE (1ULL << 38)
#define B_AKATSUKI (1ULL << 39)
#define B_CRICKETER (1ULL << 40)
#define B_BATSMAN (1ULL << 41)
#define B_BOWLER (1ULL << 42)
#define B_ALLROUNDER (1ULL << 43)
#define B_CAPTAIN (1ULL << 44)
#define B_WICKETKEEPER (1ULL << 45)
#define B_RETIRED_CRICKETER (1ULL << 46)
#define B_INDIAN_NATIONAL (1ULL << 47)
#define B_PAKISTANI_NATIONAL (1ULL << 48)
#define B_AUSTRALIAN_NATIONAL (1ULL << 49)
#define B_WEST_INDIAN_NATIONAL (1ULL << 50)
#define B_SOUTH_AFRICAN_NATIONAL (1ULL << 51)
#define B_ENGLISH_NATIONAL (1ULL << 52)
#define B_SRI_LANKAN_NATIONAL (1ULL << 53)
#define NUM_BITS 54

struct Question {
  const char* text;
  uint64_t bit;
};

// ==== AUTO-GENERATED QUESTIONS (one per bit) ====
Question QUESTIONS[NUM_BITS] = {
  {"Is your character a real person (not fictional)?", B_REAL_PERSON},
  {"Is your character male?", B_MALE},
  {"Is your character female?", B_FEMALE},
  {"Is your character a non-human creature, animal, or monster?", B_ANIMAL},
  {"Is your character a robot?", B_ROBOT},
  {"Is your character a child?", B_CHILD},
  {"Is your character from an Indian cartoon/show?", B_INDIAN_SHOW},
  {"Is your character from a Japanese anime or manga?", B_JAPANESE_ANIME},
  {"Is your character from an American comic/superhero franchise?", B_AMERICAN_COMIC},
  {"Is your character from a video game?", B_VIDEO_GAME},
  {"Is your character from mythology?", B_MYTHOLOGICAL},
  {"Does your character have superpowers?", B_SUPERPOWERS},
  {"Can your character fly?", B_CAN_FLY},
  {"Is your character known for being a very strong fighter?", B_STRONG_FIGHTER},
  {"Does your character fight with a sword or blade?", B_SWORD},
  {"Is your character a ninja?", B_NINJA},
  {"Is your character a pirate?", B_PIRATE},
  {"Does your character wear a mask or helmet?", B_MASK_HELMET},
  {"Does your character wear glasses?", B_GLASSES},
  {"Is your character mostly blue colored?", B_BLUE_THEMED},
  {"Does your character rely on gadgets or technology?", B_GADGETS_TECH},
  {"Is your character a demon or supernatural creature?", B_DEMON_SUPER},
  {"Is your character a villain or antagonist?", B_VILLAIN},
  {"Is your character mainly known for being funny / comic relief?", B_COMIC_RELIEF},
  {"Is your character known for sports?", B_SPORTSMAN},
  {"Is your character known as a scientist or leader?", B_SCIENTIST_LEADER},
  {"Is your character the main hero of a shonen anime?", B_SHONEN_PROTAGONIST},
  {"Is your character a soul reaper / death god type being?", B_SHINIGAMI},
  {"Is your character an alchemist?", B_ALCHEMIST},
  {"Can your character transform into a giant titan?", B_TITAN_SHIFTER},
  {"Does your character belong to an official hero organization?", B_HERO_SOCIETY},
  {"Is your character an assassin or a professional hunter?", B_ASSASSIN_HUNTER},
  {"Was your character transported into another world?", B_ISEKAI},
  {"Is your character part of a spy or secret-agent family?", B_SPY_FAMILY},
  {"Is your character part of a demon-slaying organization?", B_DEMON_SLAYER_CORPS},
  {"Is your character a student training to become a hero?", B_PRO_HERO_STUDENT},
  {"Is your character a genius detective or strategist?", B_GENIUS_DETECTIVE},
  {"Has your character eaten a special fruit that gave them powers?", B_DEVIL_FRUIT},
  {"Has your character held the title of Hokage (village leader)?", B_HOKAGE},
  {"Is your character part of a secretive villain organization?", B_AKATSUKI},
  {"Is your character a cricketer?", B_CRICKETER},
  {"Is your character primarily known as a batsman?", B_BATSMAN},
  {"Is your character primarily known as a bowler?", B_BOWLER},
  {"Is your character known as an all-rounder?", B_ALLROUNDER},
  {"Has your character captained their national team?", B_CAPTAIN},
  {"Is your character a wicketkeeper?", B_WICKETKEEPER},
  {"Has your character retired from professional cricket?", B_RETIRED_CRICKETER},
  {"Does your character play/represent India?", B_INDIAN_NATIONAL},
  {"Does your character play/represent Pakistan?", B_PAKISTANI_NATIONAL},
  {"Does your character play/represent Australia?", B_AUSTRALIAN_NATIONAL},
  {"Does your character play/represent the West Indies?", B_WEST_INDIAN_NATIONAL},
  {"Does your character play/represent South Africa?", B_SOUTH_AFRICAN_NATIONAL},
  {"Does your character play/represent England?", B_ENGLISH_NATIONAL},
  {"Does your character play/represent Sri Lanka?", B_SRI_LANKAN_NATIONAL},
};

struct Character {
  const char* name;
  uint64_t attrs;
};

// ==== AUTO-GENERATED CHARACTERS (101 total, all attribute-fingerprints verified unique) ====
Character CHARACTERS[] = {
  {"Chhota Bheem", B_MALE | B_CHILD | B_INDIAN_SHOW | B_STRONG_FIGHTER},
  {"Raju", B_MALE | B_CHILD | B_INDIAN_SHOW},
  {"Jaggu", B_ANIMAL | B_INDIAN_SHOW},
  {"Kalia", B_MALE | B_INDIAN_SHOW | B_VILLAIN},
  {"Dholu", B_MALE | B_CHILD | B_INDIAN_SHOW | B_COMIC_RELIEF},
  {"Bholu", B_MALE | B_CHILD | B_INDIAN_SHOW | B_GLASSES | B_COMIC_RELIEF},
  {"Motu", B_MALE | B_INDIAN_SHOW | B_COMIC_RELIEF},
  {"Patlu", B_MALE | B_INDIAN_SHOW | B_GLASSES | B_COMIC_RELIEF},
  {"Dr. Jhatka", B_MALE | B_INDIAN_SHOW | B_VILLAIN | B_SCIENTIST_LEADER},
  {"Ghasitaram", B_MALE | B_INDIAN_SHOW | B_VILLAIN | B_COMIC_RELIEF},
  {"Chutki", B_FEMALE | B_CHILD | B_INDIAN_SHOW},
  {"Doraemon", B_ROBOT | B_JAPANESE_ANIME | B_BLUE_THEMED | B_GADGETS_TECH},
  {"Nobita", B_MALE | B_CHILD | B_JAPANESE_ANIME | B_GLASSES},
  {"Shizuka", B_FEMALE | B_CHILD | B_JAPANESE_ANIME},
  {"Gian", B_MALE | B_CHILD | B_JAPANESE_ANIME | B_STRONG_FIGHTER | B_VILLAIN},
  {"Suneo", B_MALE | B_CHILD | B_JAPANESE_ANIME | B_GLASSES | B_COMIC_RELIEF},
  {"Shinchan", B_MALE | B_CHILD | B_JAPANESE_ANIME | B_COMIC_RELIEF},
  {"Kazama", B_MALE | B_CHILD | B_JAPANESE_ANIME},
  {"Himawari", B_FEMALE | B_CHILD | B_JAPANESE_ANIME | B_COMIC_RELIEF},
  {"Ninja Hattori", B_MALE | B_JAPANESE_ANIME | B_NINJA},
  {"Kenichi", B_MALE | B_CHILD | B_JAPANESE_ANIME | B_STRONG_FIGHTER},
  {"Naruto", B_MALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_NINJA | B_SHONEN_PROTAGONIST | B_HOKAGE},
  {"Sasuke", B_MALE | B_JAPANESE_ANIME | B_SWORD | B_NINJA | B_VILLAIN | B_AKATSUKI},
  {"Goku", B_MALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_CAN_FLY | B_STRONG_FIGHTER | B_SHONEN_PROTAGONIST},
  {"Vegeta", B_MALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_CAN_FLY | B_STRONG_FIGHTER | B_VILLAIN},
  {"Luffy", B_MALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_PIRATE | B_SHONEN_PROTAGONIST | B_DEVIL_FRUIT},
  {"Zoro", B_MALE | B_JAPANESE_ANIME | B_STRONG_FIGHTER | B_SWORD | B_PIRATE},
  {"Tanjiro", B_MALE | B_JAPANESE_ANIME | B_SWORD | B_SHONEN_PROTAGONIST | B_DEMON_SLAYER_CORPS},
  {"Nezuko", B_FEMALE | B_JAPANESE_ANIME | B_DEMON_SUPER | B_DEMON_SLAYER_CORPS},
  {"Spider Man", B_MALE | B_AMERICAN_COMIC | B_SUPERPOWERS | B_MASK_HELMET},
  {"Iron Man", B_MALE | B_AMERICAN_COMIC | B_CAN_FLY | B_MASK_HELMET | B_GADGETS_TECH},
  {"Batman", B_MALE | B_AMERICAN_COMIC | B_MASK_HELMET | B_GADGETS_TECH | B_GENIUS_DETECTIVE},
  {"Superman", B_MALE | B_AMERICAN_COMIC | B_SUPERPOWERS | B_CAN_FLY},
  {"Hulk", B_MALE | B_AMERICAN_COMIC | B_SUPERPOWERS | B_STRONG_FIGHTER},
  {"Captain America", B_MALE | B_AMERICAN_COMIC | B_STRONG_FIGHTER | B_MASK_HELMET},
  {"Steve", B_MALE | B_VIDEO_GAME},
  {"Creeper", B_ANIMAL | B_VIDEO_GAME | B_VILLAIN},
  {"Mario", B_MALE | B_VIDEO_GAME | B_SUPERPOWERS},
  {"Sonic", B_ANIMAL | B_VIDEO_GAME | B_BLUE_THEMED},
  {"APJ Abdul Kalam", B_REAL_PERSON | B_MALE | B_SCIENTIST_LEADER},
  {"MS Dhoni", B_REAL_PERSON | B_MALE | B_CRICKETER | B_CAPTAIN | B_WICKETKEEPER | B_INDIAN_NATIONAL},
  {"Hanuman", B_MALE | B_MYTHOLOGICAL | B_SUPERPOWERS | B_CAN_FLY | B_STRONG_FIGHTER},
  {"Krishna", B_MALE | B_MYTHOLOGICAL | B_SUPERPOWERS | B_CAN_FLY},
  {"Ichigo Kurosaki", B_MALE | B_JAPANESE_ANIME | B_SWORD | B_SHONEN_PROTAGONIST | B_SHINIGAMI},
  {"Rukia Kuchiki", B_FEMALE | B_JAPANESE_ANIME | B_SWORD | B_SHINIGAMI},
  {"Edward Elric", B_MALE | B_JAPANESE_ANIME | B_SHONEN_PROTAGONIST | B_ALCHEMIST},
  {"Alphonse Elric", B_MALE | B_ROBOT | B_JAPANESE_ANIME | B_ALCHEMIST},
  {"Eren Yeager", B_MALE | B_JAPANESE_ANIME | B_SHONEN_PROTAGONIST | B_TITAN_SHIFTER},
  {"Mikasa Ackerman", B_FEMALE | B_CHILD | B_JAPANESE_ANIME | B_STRONG_FIGHTER | B_SWORD},
  {"Levi Ackerman", B_MALE | B_JAPANESE_ANIME | B_STRONG_FIGHTER | B_SWORD},
  {"Light Yagami", B_MALE | B_JAPANESE_ANIME | B_VILLAIN | B_GENIUS_DETECTIVE},
  {"L Lawliet", B_MALE | B_JAPANESE_ANIME | B_GENIUS_DETECTIVE},
  {"Natsu Dragneel", B_MALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_SHONEN_PROTAGONIST},
  {"Erza Scarlet", B_FEMALE | B_JAPANESE_ANIME | B_STRONG_FIGHTER | B_SWORD},
  {"Saitama", B_MALE | B_JAPANESE_ANIME | B_STRONG_FIGHTER | B_COMIC_RELIEF | B_HERO_SOCIETY},
  {"Genos", B_MALE | B_ROBOT | B_JAPANESE_ANIME | B_HERO_SOCIETY},
  {"All Might", B_MALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_STRONG_FIGHTER | B_HERO_SOCIETY},
  {"Izuku Midoriya", B_MALE | B_JAPANESE_ANIME | B_SHONEN_PROTAGONIST | B_PRO_HERO_STUDENT},
  {"Katsuki Bakugo", B_MALE | B_JAPANESE_ANIME | B_PRO_HERO_STUDENT},
  {"Shoto Todoroki", B_MALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_PRO_HERO_STUDENT},
  {"Killua Zoldyck", B_MALE | B_CHILD | B_JAPANESE_ANIME | B_ASSASSIN_HUNTER},
  {"Gon Freecss", B_MALE | B_CHILD | B_JAPANESE_ANIME | B_SHONEN_PROTAGONIST | B_ASSASSIN_HUNTER},
  {"Meliodas", B_MALE | B_JAPANESE_ANIME | B_STRONG_FIGHTER | B_DEMON_SUPER},
  {"Rimuru Tempest", B_MALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_ISEKAI},
  {"Tanya Degurechaff", B_FEMALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_ISEKAI},
  {"Anya Forger", B_FEMALE | B_CHILD | B_JAPANESE_ANIME | B_SPY_FAMILY},
  {"Loid Forger", B_MALE | B_JAPANESE_ANIME | B_GADGETS_TECH | B_SPY_FAMILY},
  {"Yor Forger", B_FEMALE | B_JAPANESE_ANIME | B_ASSASSIN_HUNTER | B_SPY_FAMILY},
  {"Kakashi Hatake", B_MALE | B_JAPANESE_ANIME | B_NINJA | B_MASK_HELMET},
  {"Itachi Uchiha", B_MALE | B_JAPANESE_ANIME | B_NINJA | B_AKATSUKI},
  {"Bulma", B_FEMALE | B_JAPANESE_ANIME | B_GADGETS_TECH | B_SCIENTIST_LEADER},
  {"Piccolo", B_MALE | B_JAPANESE_ANIME | B_CAN_FLY | B_STRONG_FIGHTER | B_DEMON_SUPER},
  {"Frieza", B_MALE | B_JAPANESE_ANIME | B_CAN_FLY | B_STRONG_FIGHTER | B_VILLAIN},
  {"Cell", B_MALE | B_ROBOT | B_JAPANESE_ANIME | B_CAN_FLY | B_STRONG_FIGHTER | B_VILLAIN},
  {"Boruto Uzumaki", B_MALE | B_CHILD | B_JAPANESE_ANIME | B_NINJA},
  {"Sakura Haruno", B_FEMALE | B_JAPANESE_ANIME | B_STRONG_FIGHTER | B_NINJA},
  {"Hinata Hyuga", B_FEMALE | B_JAPANESE_ANIME | B_SUPERPOWERS | B_NINJA},
  {"Nami", B_FEMALE | B_JAPANESE_ANIME | B_PIRATE},
  {"Sanji", B_MALE | B_JAPANESE_ANIME | B_STRONG_FIGHTER | B_PIRATE},
  {"Shanks", B_MALE | B_JAPANESE_ANIME | B_SWORD | B_PIRATE},
  {"Virat Kohli", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_CAPTAIN | B_INDIAN_NATIONAL},
  {"Sachin Tendulkar", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_RETIRED_CRICKETER | B_INDIAN_NATIONAL},
  {"Rohit Sharma", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_INDIAN_NATIONAL},
  {"Kapil Dev", B_REAL_PERSON | B_MALE | B_CRICKETER | B_ALLROUNDER | B_CAPTAIN | B_RETIRED_CRICKETER | B_INDIAN_NATIONAL},
  {"Sourav Ganguly", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_CAPTAIN | B_RETIRED_CRICKETER | B_INDIAN_NATIONAL},
  {"Rahul Dravid", B_REAL_PERSON | B_MALE | B_GLASSES | B_CRICKETER | B_BATSMAN | B_INDIAN_NATIONAL},
  {"Yuvraj Singh", B_REAL_PERSON | B_MALE | B_CRICKETER | B_ALLROUNDER | B_RETIRED_CRICKETER | B_INDIAN_NATIONAL},
  {"Jasprit Bumrah", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BOWLER | B_INDIAN_NATIONAL},
  {"Hardik Pandya", B_REAL_PERSON | B_MALE | B_CRICKETER | B_ALLROUNDER | B_CAPTAIN | B_INDIAN_NATIONAL},
  {"Ravindra Jadeja", B_REAL_PERSON | B_MALE | B_CRICKETER | B_ALLROUNDER | B_INDIAN_NATIONAL},
  {"KL Rahul", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_WICKETKEEPER | B_INDIAN_NATIONAL},
  {"Shane Warne", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BOWLER | B_RETIRED_CRICKETER | B_AUSTRALIAN_NATIONAL},
  {"Ricky Ponting", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_CAPTAIN | B_RETIRED_CRICKETER | B_AUSTRALIAN_NATIONAL},
  {"Steve Smith", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_CAPTAIN | B_AUSTRALIAN_NATIONAL},
  {"Brian Lara", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_CAPTAIN | B_RETIRED_CRICKETER | B_WEST_INDIAN_NATIONAL},
  {"Chris Gayle", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_RETIRED_CRICKETER | B_WEST_INDIAN_NATIONAL},
  {"AB de Villiers", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_WICKETKEEPER | B_RETIRED_CRICKETER | B_SOUTH_AFRICAN_NATIONAL},
  {"Wasim Akram", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BOWLER | B_CAPTAIN | B_RETIRED_CRICKETER | B_PAKISTANI_NATIONAL},
  {"Babar Azam", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BATSMAN | B_CAPTAIN | B_PAKISTANI_NATIONAL},
  {"Ben Stokes", B_REAL_PERSON | B_MALE | B_CRICKETER | B_ALLROUNDER | B_CAPTAIN | B_ENGLISH_NATIONAL},
  {"Muttiah Muralitharan", B_REAL_PERSON | B_MALE | B_CRICKETER | B_BOWLER | B_RETIRED_CRICKETER | B_SRI_LANKAN_NATIONAL},
};

const int NUM_CHARACTERS = sizeof(CHARACTERS) / sizeof(Character);

// ====================================================================
// GAME STATE
// ====================================================================
bool candidateActive[200];
bool questionAsked[NUM_BITS];
int  questionsAskedCount = 0;
int  currentQuestionIndex = -1;
int  guessIndex = -1;
bool waitingForGuessConfirm = false;
int  guessesTried = 0;
const int MAX_QUESTIONS = 15;
const int MAX_GUESSES = 3;
bool gameOver = false;
bool gameWon = false;

// ---- Easter egg state ----
bool askedIdiotQuestion = false; // has the first "idiot" question been asked yet?
bool likhithMode = false;        // are we showing the "Are you Likhith MS?" screen?

bool lastColorR = 0, lastColorG = 0, lastColorB = 1;

void setLED(bool r, bool g, bool b) {
  digitalWrite(PIN_RED, r ? HIGH : LOW);
  digitalWrite(PIN_GREEN, g ? HIGH : LOW);
  digitalWrite(PIN_BLUE, b ? HIGH : LOW);
}

void setGameLED(bool r, bool g, bool b) {
  lastColorR = r; lastColorG = g; lastColorB = b;
  setLED(r, g, b);
}

void resetGame() {
  for (int i = 0; i < NUM_CHARACTERS; i++) candidateActive[i] = true;
  for (int i = 0; i < NUM_BITS; i++) questionAsked[i] = false;
  questionsAskedCount = 0;
  guessIndex = -1;
  waitingForGuessConfirm = false;
  guessesTried = 0;
  gameOver = false;
  gameWon = false;
  askedIdiotQuestion = false;
  likhithMode = false;
  setGameLED(0, 0, 1); // blue = thinking
}

int countActiveCandidates() {
  int c = 0;
  for (int i = 0; i < NUM_CHARACTERS; i++) if (candidateActive[i]) c++;
  return c;
}

int pickBestQuestion() {
  int bestQ = -1;
  int bestScore = 999999;
  for (int q = 0; q < NUM_BITS; q++) {
    if (questionAsked[q]) continue;
    int yes = 0, no = 0;
    for (int c = 0; c < NUM_CHARACTERS; c++) {
      if (!candidateActive[c]) continue;
      if (CHARACTERS[c].attrs & QUESTIONS[q].bit) yes++; else no++;
    }
    if (yes == 0 || no == 0) continue;
    int score = abs(yes - no);
    if (score < bestScore) {
      bestScore = score;
      bestQ = q;
    }
  }
  return bestQ;
}

int pickBestCandidate() {
  for (int c = 0; c < NUM_CHARACTERS; c++) {
    if (candidateActive[c]) return c;
  }
  return -1;
}

void applyAnswer(int qIndex, bool yes) {
  uint64_t bit = QUESTIONS[qIndex].bit;
  for (int c = 0; c < NUM_CHARACTERS; c++) {
    if (!candidateActive[c]) continue;
    bool hasBit = (CHARACTERS[c].attrs & bit) != 0;
    if (hasBit != yes) candidateActive[c] = false;
  }
  questionAsked[qIndex] = true;
  questionsAskedCount++;
}

void advanceGame() {
  if (gameOver || likhithMode) return;

  int activeCount = countActiveCandidates();

  if (activeCount == 1 || questionsAskedCount >= MAX_QUESTIONS || pickBestQuestion() == -1) {
    guessIndex = pickBestCandidate();
    if (guessIndex == -1) {
      gameOver = true;
      setGameLED(1, 0, 0);
      return;
    }
    waitingForGuessConfirm = true;
    setGameLED(0, 0, 1);
    return;
  }

  currentQuestionIndex = pickBestQuestion();
  waitingForGuessConfirm = false;
  setGameLED(0, 0, 1);
}

// ====================================================================
// WEB PAGE
// ====================================================================
String htmlPage() {
  String html = R"HTML(
<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width, initial-scale=1">
<title>The Oracle</title>
<style>
  :root{
    --bg:#0b0a14; --bg2:#141225; --ink:#e7e3f5; --ink-dim:#8b86a8;
    --violet:#b48cff; --violet-dim:#5d4d99; --gold:#f0c05a; --green:#5fd98a; --red:#ff6b6b;
  }
  *{box-sizing:border-box}
  body{
    margin:0; min-height:100vh; background:
      radial-gradient(ellipse at 50% -10%, var(--bg2), var(--bg) 60%);
    color:var(--ink); font-family:Georgia,'Palatino Linotype',serif;
    display:flex; flex-direction:column; align-items:center; justify-content:center;
    padding:32px 20px; text-align:center;
  }
  .eyebrow{
    font-family:'Courier New',monospace; letter-spacing:.3em; text-transform:uppercase;
    font-size:.7rem; color:var(--violet-dim); margin-bottom:6px;
  }
  h1{
    font-weight:400; font-size:1.9rem; letter-spacing:.04em; margin:0 0 34px;
    color:var(--ink);
  }
  h1 em{color:var(--violet); font-style:normal}

  .orb-wrap{position:relative; width:170px; height:170px; margin-bottom:30px}
  .orb{
    width:170px; height:170px; border-radius:50%;
    background:radial-gradient(circle at 35% 30%, var(--violet), var(--violet-dim) 65%, transparent 100%);
    box-shadow:0 0 60px 8px rgba(180,140,255,.35), inset 0 0 30px rgba(255,255,255,.15);
    animation:pulse 2.6s ease-in-out infinite;
  }
  .orb.guess{
    background:radial-gradient(circle at 35% 30%, var(--gold), #7a5a1e 65%, transparent 100%);
    box-shadow:0 0 60px 10px rgba(240,192,90,.4), inset 0 0 30px rgba(255,255,255,.2);
    animation:pulse 1.4s ease-in-out infinite;
  }
  .orb.won{
    background:radial-gradient(circle at 35% 30%, var(--green), #1f6b42 65%, transparent 100%);
    box-shadow:0 0 70px 14px rgba(95,217,138,.45), inset 0 0 30px rgba(255,255,255,.2);
    animation:none;
  }
  .orb.lost{
    background:radial-gradient(circle at 35% 30%, var(--red), #7a2323 65%, transparent 100%);
    box-shadow:0 0 60px 10px rgba(255,107,107,.4), inset 0 0 30px rgba(255,255,255,.15);
    animation:none;
  }
  .orb.likhith{
    background:radial-gradient(circle at 35% 30%, #ff5ec4, #7a1e5e 65%, transparent 100%);
    box-shadow:0 0 70px 14px rgba(255,94,196,.45), inset 0 0 30px rgba(255,255,255,.2);
    animation:pulse 0.6s ease-in-out infinite;
  }
  @keyframes pulse{
    0%,100%{transform:scale(1); opacity:1}
    50%{transform:scale(1.06); opacity:.85}
  }

  #box{
    max-width:420px; font-size:1.3rem; line-height:1.5; min-height:2.6em;
    margin-bottom:26px;
  }
  #box .name{color:var(--gold); font-style:italic}

  .dots{display:flex; gap:6px; margin-bottom:30px; min-height:8px}
  .dot{width:8px; height:8px; border-radius:50%; background:#2a2745}
  .dot.filled{background:var(--violet)}

  #controls{display:flex; gap:14px; flex-wrap:wrap; justify-content:center}
  button{
    font-family:'Courier New',monospace; letter-spacing:.05em; text-transform:uppercase;
    font-size:.85rem; padding:14px 30px; border:1px solid transparent; border-radius:999px;
    cursor:pointer; transition:transform .12s ease, box-shadow .12s ease;
    background:var(--bg2); color:var(--ink);
  }
  button:active{transform:scale(.96)}
  .yes{border-color:var(--green); color:var(--green)}
  .yes:hover{box-shadow:0 0 16px rgba(95,217,138,.35)}
  .no{border-color:var(--red); color:var(--red)}
  .no:hover{box-shadow:0 0 16px rgba(255,107,107,.35)}
  .restart{border-color:var(--violet); color:var(--violet)}
  .restart:hover{box-shadow:0 0 16px rgba(180,140,255,.35)}
</style></head><body>
<div class="eyebrow">NodeMCU &middot; character oracle</div>
<h1>Think of a character. <em>I will find it.</em></h1>
<div class="orb-wrap"><div class="orb" id="orb"></div></div>
<div id="box">Waking up&hellip;</div>
<div class="dots" id="dots"></div>
<div id="controls"></div>
<script>
async function refresh(){
  let r = await fetch('/state'); let d = await r.json();
  let box = document.getElementById('box');
  let ctr = document.getElementById('controls');
  let orb = document.getElementById('orb');
  let dots = document.getElementById('dots');
  ctr.innerHTML = '';
  dots.innerHTML = '';

  if(d.mode === 'question' || d.mode === 'guess'){
    let n = d.maxq || 15;
    for(let i=0;i<n;i++){
      let s = document.createElement('div');
      s.className = 'dot' + (i < d.q ? ' filled' : '');
      dots.appendChild(s);
    }
  }

  if(d.mode === 'question'){
    orb.className = 'orb';
    box.innerText = d.text;
    ctr.innerHTML = '<button class="yes" onclick="answer(1)">Yes</button><button class="no" onclick="answer(0)">No</button>';
  } else if(d.mode === 'guess'){
    orb.className = 'orb guess';
    box.innerHTML = 'Is it&hellip; <span class="name">' + d.text + '</span>?';
    ctr.innerHTML = '<button class="yes" onclick="confirmGuess(1)">Correct</button><button class="no" onclick="confirmGuess(0)">Not quite</button>';
  } else if(d.mode === 'won'){
    orb.className = 'orb won';
    box.innerHTML = 'It was <span class="name">' + d.text + '</span> all along.';
    ctr.innerHTML = '<button class="restart" onclick="restart()">Play again</button>';
  } else if(d.mode === 'lost'){
    orb.className = 'orb lost';
    box.innerText = 'You have stumped the oracle.';
    ctr.innerHTML = '<button class="restart" onclick="restart()">Play again</button>';
  } else if(d.mode === 'likhith'){
    orb.className = 'orb likhith';
    box.innerText = 'Are you Likhith MS?';
    ctr.innerHTML = '<button class="yes" onclick="likhithAnswer()">Yes</button><button class="yes" onclick="likhithAnswer()">Yes</button>';
  }
}
async function answer(v){ await fetch('/answer?val=' + v); refresh(); }
async function confirmGuess(v){ await fetch('/confirm?val=' + v); refresh(); }
async function restart(){ await fetch('/restart'); refresh(); }
async function likhithAnswer(){ await fetch('/likhith_ack'); refresh(); }
refresh();
setInterval(refresh, 1500);
</script></body></html>
)HTML";
  return html;
}

// ====================================================================
// WEB HANDLERS
// ====================================================================
void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleState() {
  String json;
  if (likhithMode) {
    json = "{\"mode\":\"likhith\"}";
  } else if (gameOver) {
    if (gameWon) {
      json = String("{\"mode\":\"won\",\"text\":\"") + CHARACTERS[guessIndex].name + "\"}";
    } else {
      json = "{\"mode\":\"lost\"}";
    }
  } else if (!askedIdiotQuestion) {
    json = String("{\"mode\":\"question\",\"text\":\"Is your character an idiot?\",\"q\":0,\"maxq\":") + MAX_QUESTIONS + "}";
  } else if (waitingForGuessConfirm) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Guess:");

    lcd.setCursor(0,1);
    lcd.print(CHARACTERS[guessIndex].name);

    json = String("{\"mode\":\"guess\",\"text\":\"") + CHARACTERS[guessIndex].name + "\",\"q\":" + questionsAskedCount + ",\"maxq\":" + MAX_QUESTIONS + "}";
  } else {
    json = String("{\"mode\":\"question\",\"text\":\"") + QUESTIONS[currentQuestionIndex].text + "\",\"q\":" + questionsAskedCount + ",\"maxq\":" + MAX_QUESTIONS + "}";
  }
  server.send(200, "application/json", json);
}

void handleAnswer() {
  String val = server.arg("val");
  bool yes = (val == "1");

  // Track "No" clicks for the spam easter egg, regardless of game phase.
  if (!yes) {
    recordNoClick();
    if (checkNoSpam()) {
      likhithMode = true;
      server.send(200, "text/plain", "ok");
      return;
    }
  }

  // Special-case the very first question (the idiot question).
  if (!askedIdiotQuestion && !gameOver && !likhithMode) {
    askedIdiotQuestion = true;
    if (yes) {
      likhithMode = true;
      server.send(200, "text/plain", "ok");
      return;
    }
    // "No" -> proceed into the normal game
    advanceGame();
    server.send(200, "text/plain", "ok");
    return;
  }

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Thinking...");

  if (!waitingForGuessConfirm && !gameOver && !likhithMode) {
    applyAnswer(currentQuestionIndex, yes);
    advanceGame();
  }
  server.send(200, "text/plain", "ok");
}

void handleConfirm() {
  String val = server.arg("val");
  if (val == "1") {
    gameOver = true;
    gameWon = true;
    setGameLED(0, 1, 0);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(CHARACTERS[guessIndex].name);

    lcd.setCursor(0,1);
    lcd.print("Correct!");
  } else {
    candidateActive[guessIndex] = false;
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(CHARACTERS[guessIndex].name);

    lcd.setCursor(0,1);
    lcd.print("Wrong!");
    guessesTried++;
    if (guessesTried >= MAX_GUESSES) {
      gameOver = true;
      setGameLED(1, 0, 0);
    } else {
      advanceGame();
    }
  }
  server.send(200, "text/plain", "ok");
}

void handleLikhithAck() {
  // Whichever "Yes" button they tap, just restart the game fresh.
  resetGame();
  advanceGame();
  server.send(200, "text/plain", "ok");
}

void handleRestart() {
  resetGame();
  advanceGame();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("New Game");

  lcd.setCursor(0,1);
  lcd.print(WiFi.softAPIP());
  server.send(200, "text/plain", "ok");
}

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Akinator Ready");

  lcd.setCursor(0,1);
  lcd.print(WiFi.softAPIP());

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE, OUTPUT);
  setLED(0, 0, 0);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASS);
  delay(200);
  Serial.println();
  Serial.print("WiFi network created: ");
  Serial.println(AP_SSID);
  Serial.println("Connect your phone/laptop to that network, then open:");
  Serial.print("http://");
  Serial.println(WiFi.softAPIP());

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Connect to:");

  lcd.setCursor(0,1);
  lcd.print(WiFi.softAPIP());

  resetGame();
  advanceGame();

  server.on("/", handleRoot);
  server.on("/state", handleState);
  server.on("/answer", handleAnswer);
  server.on("/confirm", handleConfirm);
  server.on("/restart", handleRestart);
  server.on("/likhith_ack", handleLikhithAck);
  server.begin();
}

int lastStationCount = -1;

void loop() {
  int stationCount = WiFi.softAPgetStationNum();

  if (stationCount == 0) {
    unsigned long now = millis();
    if (now - lastBlinkTime >= BLINK_INTERVAL_MS) {
      lastBlinkTime = now;
      blinkState = !blinkState;
      setLED(0, 0, blinkState);
    }
  } else if (lastStationCount == 0 || lastStationCount == -1) {
    setLED(lastColorR, lastColorG, lastColorB);
  }

  server.handleClient();
  lastStationCount = stationCount;
}
