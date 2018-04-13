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

// ======= State machine ======= //
enum MusicState { STATE_PAUSING, STATE_PLAYING };
MusicState musicState = STATE_PAUSING;


// ======= Button ======= //
#define PLAY_BUTTON_PIN  3
#define NEXT_BUTTON_PIN  2
#define PREV_BUTTON_PIN  4
#define DEBOUNCE_MS 200

void setup() {
  Serial.begin(115200);
  audio.initHardware();
  audio.setVolume(volume);

  pinMode(PLAY_BUTTON_PIN, INPUT_PULLUP);

  manager.addListener(EVENT_PLAY_PAUSE, playPauseCallback);
  manager.addListener(EVENT_NEXT, nextCallback);
  manager.addListener(EVENT_PREV, prevCallback);
}

void loop() {
  // Handle events in the queue
  manager.processEvent();

  // Check for events
  checkForButton(PLAY_BUTTON_PIN, EVENT_PLAY_PAUSE);
  checkForButton(NEXT_BUTTON_PIN, EVENT_NEXT);
  checkForButton(PREV_BUTTON_PIN, EVENT_PREV);
}

void checkForButton(int buttonPin, EventManager::EventType event) {
  if (digitalRead(buttonPin) == HIGH) {
    delay(DEBOUNCE_MS);
    if (digitalRead(buttonPin) == HIGH) {
      manager.queueEvent(event, 0);
    }
  }
}

void playPauseCallback(int event, int param) {
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
}

void nextCallback(int event, int param) {
  Serial.println("next");
  audio.next();
}

void prevCallback() {
  Serial.println("prev");
  audio.previous();
}

