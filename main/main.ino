#include <AudioDevice.h>
#include <EventManager.h>
#include <SPI.h>
#include <MFRC522.h>

// ======== MP3 module ========= //
const int PIN_MP3_TX = 7;
const int PIN_MP3_RX = 8;

AudioDevice audio(PIN_MP3_TX, PIN_MP3_RX, mp3a);


// ======= Event manager ======= //
EventManager manager;

// ========== Events =========== //
#define EVENT_PLAY_PAUSE EventManager::kEventUser0
#define EVENT_NEXT       EventManager::kEventUser1
#define EVENT_PREV       EventManager::kEventUser2
#define EVENT_VOLUME     EventManager::kEventUser3
#define EVENT_RFID       EventManager::kEventUser3

#define RFID_IN  0
#define RFID_OUT 1

// ======= State machine ======= //
enum MusicState { STATE_PAUSING, STATE_PLAYING };
MusicState musicState = STATE_PAUSING;

enum LeafState { STATE_LEAF, STATE_NO_LEAF };
LeafState leafState = STATE_NO_LEAF;


// ========== Buttons ========== //
#define PLAY_BUTTON_PIN  3
#define NEXT_BUTTON_PIN  2
#define PREV_BUTTON_PIN  4
#define DEBOUNCE_MS 170

// ======== Linear pot ========= //
#define LINEAR_POT_PIN   A2
int prevPotReading = 0;

// =========== RFID ============ //
constexpr uint8_t RST_PIN = 9;
constexpr uint8_t SS_PIN = 10;

MFRC522 mfrc522(SS_PIN, RST_PIN);
int currRFID;

// =========== Songs =========== //
#define PLAYLIST_0_ID   760
#define PLAYLIST_1_ID   795
#define PLAYLIST_2_ID   720
#define FEED_ME_SONG    7
#define NUM_SONGS       3 // per playlist

int playlistSongs[3][NUM_SONGS] = {
  {3, 1, 5},
  {11, 9, 13},
  {15, 17, 19}
};

int songIndex = 0;
int playlistIndex = -1;


void setup() {
  Serial.begin(115200);

  pinMode(PLAY_BUTTON_PIN, INPUT_PULLUP);
  pinMode(NEXT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PREV_BUTTON_PIN, INPUT_PULLUP);
  pinMode(LINEAR_POT_PIN, INPUT);

  // Volume
  prevPotReading = analogRead(LINEAR_POT_PIN);
  audio.initHardware();
  audio.setVolume(volumeOf(prevPotReading));

  // RFID
  SPI.begin();
  mfrc522.PCD_Init();

  manager.addListener(EVENT_PLAY_PAUSE, musicPlaybackCallback);
  manager.addListener(EVENT_NEXT, musicPlaybackCallback);
  manager.addListener(EVENT_PREV, musicPlaybackCallback);
  manager.addListener(EVENT_VOLUME, volumeCallback);
  manager.addListener(EVENT_RFID, rfidCallback);
}

void loop() {
  // Handle events in the queue
  manager.processEvent();

  // Check for events
  checkForLeaf();
  checkForButton(PLAY_BUTTON_PIN, EVENT_PLAY_PAUSE);
  checkForButton(NEXT_BUTTON_PIN, EVENT_NEXT);
  checkForButton(PREV_BUTTON_PIN, EVENT_PREV);
  checkForPot();
}


// Event checkers

void checkForLeaf() {
//   
  if (leafState == STATE_NO_LEAF && mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    currRFID = 0;
    for (int i = 0; i < 10; i++) currRFID += mfrc522.uid.uidByte[i];
    manager.queueEvent(EVENT_RFID, RFID_IN);
    
  } else if (leafState == STATE_LEAF) {
    // For some mysterious reason, we need to call these twice in order for them to register
    // the correct boolean value...
    mfrc522.PICC_IsNewCardPresent();
    mfrc522.PICC_ReadCardSerial();
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      manager.queueEvent(EVENT_RFID, RFID_OUT);
    }
  }
}

void checkForButton(int buttonPin, EventManager::EventType event) {
  if (digitalRead(buttonPin) == LOW) {
    delay(DEBOUNCE_MS);
    if (digitalRead(buttonPin) == LOW) {
      Serial.println(buttonPin);
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
  switch (leafState) {
    case STATE_LEAF:
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
        songIndex = (songIndex + 1) % NUM_SONGS;
        audio.setTrack(playlistSongs[playlistIndex][songIndex]);
        Serial.print("song "); Serial.println(playlistSongs[playlistIndex][songIndex]);
      } else if (event == EVENT_PREV) {
        Serial.println("prev");
        songIndex = (songIndex - 1 + NUM_SONGS) % NUM_SONGS;
        audio.setTrack(playlistSongs[playlistIndex][songIndex]);
        Serial.print("song "); Serial.println(playlistSongs[playlistIndex][songIndex]);
      }

      break;

    case STATE_NO_LEAF:
      feedMe();
      break;
  }
}

void volumeCallback(int event, int volume) {
  audio.setVolume(volume);
}

void rfidCallback(int event, int rfid) {
  if (leafState == STATE_LEAF && rfid == RFID_OUT) { // had a leaf, but no more
      Serial.println("no leaf");
      leafState = STATE_NO_LEAF;
      audio.pause();
  } else if (leafState == STATE_NO_LEAF && rfid == RFID_IN) { // didn't have leaf, but just got one
      leafState = STATE_LEAF;
      songIndex = 0;
      
      switch (currRFID) {
        case PLAYLIST_0_ID:
          Serial.println("playlist 0 leaf inserted");
          playlistIndex = 0;
          break;
        case PLAYLIST_1_ID:
          Serial.println("playlist 1 leaf inserted");
          playlistIndex = 1;
          break;
        case PLAYLIST_2_ID:
          Serial.println("playlist 2 leaf inserted");
          playlistIndex = 2;
          break;
        default:
          Serial.print("unknown leaf inserted ");
          Serial.println(currRFID);
      }

      audio.setTrack(playlistSongs[playlistIndex][songIndex]);
      Serial.print("song "); Serial.println(playlistSongs[playlistIndex][songIndex]);
  }
}


// Misc helpers

int volumeOf(int analogReading) {
  return map(analogReading, 1, 680, 0, 255);
}

// Blocking
void feedMe() {
  Serial.println("feed me");
  audio.setTrack(FEED_ME_SONG);
  audio.play();
  delay(1000);
  audio.pause();
}

