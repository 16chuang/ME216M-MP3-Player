#include <AudioDevice.h>
#include <EventManager.h>

// ======== MP3 module ========= //
const int PIN_MP3_TX = 6;
const int PIN_MP3_RX = 7;

AudioDevice audio(PIN_MP3_TX, PIN_MP3_RX, mp3a);
int volume = 255;


// ======= Event manager ======= //
EventManager manager;

// ========== Events =========== //
#define EVENT_PLAY_PAUSE EventManager::kEventUser0
#define EVENT_NEXT       EventManager::kEventUser1
#define EVENT_PREV       EventManager::kEventUser2
#define EVENT_VOLUME     EventManager::kEventUser3

// ======= State machine ======= //
enum MusicState { STATE_PAUSING, STATE_PLAYING };
MusicState musicState = STATE_PAUSING;


// ========== Buttons ========== //
#define PLAY_BUTTON_PIN  3
#define NEXT_BUTTON_PIN  2
#define PREV_BUTTON_PIN  4
#define DEBOUNCE_MS 170

// ======== Linear pot ========= //
#define LINEAR_POT_PIN   A2
int prevPotReading = 0;

void setup() {
  Serial.begin(115200);

  pinMode(PLAY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(NEXT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PREV_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LINEAR_POT_PIN, INPUT);

  prevPotReading = analogRead(LINEAR_POT_PIN);
  audio.initHardware();
  audio.setVolume(volumeOf(prevPotReading));

  manager.addListener(EVENT_PLAY_PAUSE, musicPlaybackCallback);
  manager.addListener(EVENT_NEXT, musicPlaybackCallback);
  manager.addListener(EVENT_PREV, musicPlaybackCallback);
  manager.addListener(EVENT_VOLUME, volumeCallback);
}

void loop() {
  // Handle events in the queue
  manager.processEvent();

  // Check for events
  checkForButton(PLAY_BUTTON_PIN, EVENT_PLAY_PAUSE);
  checkForButton(NEXT_BUTTON_PIN, EVENT_NEXT);
  checkForButton(PREV_BUTTON_PIN, EVENT_PREV);
  checkForPot();
}


// Event checkers

void checkForButton(int buttonPin, EventManager::EventType event) {
  if (digitalRead(buttonPin) == HIGH) {
    delay(DEBOUNCE_MS);
    if (digitalRead(buttonPin) == HIGH) {
      manager.queueEvent(event, 0);
    }
  }
}

void checkForPot() {
  int currReading = analogRead(LINEAR_POT_PIN);
  if (prevPotReading != currReading) {
    manager.queueEvent(EVENT_VOLUME, volumeOf(currReading));
    prevPotReading = currReading;
  }
}


// Event callbacks

void musicPlaybackCallback(int event, int param) {
  if (event == EVENT_PLAY_PAUSE) {
    switch (musicState) {
      case STATE_PLAYING:
        Serial.println("pausing");
        musicState = STATE_PAUSING;
        audio.pause();
        break;
        
      case STATE_PAUSING:
        Serial.println("playing");
        musicState = STATE_PLAYING;
        audio.play();
        break;
    }
  } else if (event == EVENT_NEXT) {
    Serial.println("next");
    audio.next();
  } else if (event == EVENT_PREV) {
    Serial.println("prev");
    audio.previous();
  }
}

void volumeCallback(int event, int volume) {
  audio.setVolume(volume);
}


// Misc helpers

int volumeOf(int analogReading) {
  return map(analogReading, 1, 1023, 0, 255);
}

