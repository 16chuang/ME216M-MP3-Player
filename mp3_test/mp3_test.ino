/*
 * Audio Device example. 
 * 
 * Demonstrates the use of the AudioDevice class for communicating with mp3 players and FM radio. 
 * 
 * Open serial console and send character to trigger the different methods:
 * p: play
 * s: pause
 * ]: next track
 * [: previous track
 * 
 * 1-9: select track 1-9 
 * +: increase volume
 * -: decrease volume
 * 
 * Alex Olwal 2017
 * 
 */

#include <AudioDevice.h>

const int PIN_MP3_TX = 7;
const int PIN_MP3_RX = 8;

AudioDevice audio(PIN_MP3_TX, PIN_MP3_RX, mp3a);

int volume = 255;

void setup() {
  Serial.begin(115200);
  audio.initHardware();
  audio.setVolume(volume);
}

void loop() {
//  serialEvent();
}

void serialEvent() {

  char c = (char)Serial.read();

  switch (c)
  {
    case 'p': audio.play(); Serial.println("play"); break;
    case 's': audio.pause(); Serial.println("pause"); break;
    case ']': audio.next(); Serial.println("next"); break;
    case '[': audio.previous(); Serial.println("prev"); break;

    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      audio.setTrack(c - '0'); Serial.println(c); break;

    case '+':
      volume = min(volume + 10, 255); 
      audio.setVolume(volume);
      Serial.println(volume);
      break;

    case '-':
      volume = max(volume - 10, 0); 
      audio.setVolume(volume);
      Serial.println(volume);
      break;


  }
}
