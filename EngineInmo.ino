//RST          D9
//SDA(SS)      D10
//MOSI         D11
//MISO         D12
//SCK          D13

#include <SPI.h>
#include <MFRC522.h>
#include "pitches.h"

#define VALID 1
#define INVALID 2
#define NO_CARD 3

const bool DEBUG = true;
const int RST_PIN = 5; // Pin 9 para el reset del RC522
const int SS_PIN = 10; // Pin 10 para el SS (SDA) del RC522
const int speaker_pin = 9; // For the sound


MFRC522 mfrc522(SS_PIN, RST_PIN);   // Crear instancia del MFRC522

byte readUID[4]; // Almacena el tag leido
byte validKey1[4] = { 0x03, 0x01, 0xE2, 0xC7 };  // Ejemplo de clave valida
//D3 5B E3 C7

// Melodies definition: access, welcome and rejection
int access_melody[] = {NOTE_FS7, NOTE_DS7};
int access_noteDurations[] = {2, 2};
int fail_melody[] = {NOTE_DS7, NOTE_DS7, NOTE_DS7};
int fail_noteDurations[] = {8, 8, 8};


//========== Function to play the access granted or denied tunes ==========
void playTune(int Scan) {
  pinMode(speaker_pin, OUTPUT);
  if (Scan == 1) // A Good card Read
  {
    for (int i = 0; i < 2; i++)    //loop through the notes
    { // Good card read
      int access_noteDuration = 1000 / access_noteDurations[i];
      tone(speaker_pin, access_melody[i], access_noteDuration);
      int access_pauseBetweenNotes = access_noteDuration * 1.30;
      delay(access_pauseBetweenNotes);
      noTone(speaker_pin);
    }
  }
  else // A Bad card read
    for (int i = 0; i < 3; i++)    //loop through the notes
    {
      int fail_noteDuration = 1000 / fail_noteDurations[i];
      tone(speaker_pin, fail_melody[i], fail_noteDuration);
      int fail_pauseBetweenNotes = fail_noteDuration * 1.30;
      delay(fail_pauseBetweenNotes);
      noTone(speaker_pin);
    }
    delay(50);
    pinMode(speaker_pin, INPUT);
}

void printArray(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
  Serial.println("");
}

//Función para comparar dos vectores
bool isEqualArray(byte arrayA[], byte arrayB[], int length)
{
  for (int index = 0; index < length; index++)
  {
    if (DEBUG) {
      Serial.print(arrayA[index], HEX);
      Serial.print("-");
      Serial.println(arrayB[index], HEX);
    }
    if (arrayA[0] != arrayB[0])
      return false;
  }
  return true;
}

int getCardStatus() {
  // Detectar tarjeta
  if (mfrc522.PICC_IsNewCardPresent())
  {
    //Seleccionamos una tarjeta
    if (mfrc522.PICC_ReadCardSerial())
    {
      // Comparar ID con las claves validas
      if (isEqualArray(mfrc522.uid.uidByte, validKey1, 4)) {
        Serial.println("Tarjeta valida");
        return VALID;
      }
      else {
        Serial.println("Tarjeta invalida");
        if (DEBUG) {
          printArray(mfrc522.uid.uidByte, mfrc522.uid.size);
        }
        return INVALID;
      }
      // Finalizar lectura actual
      mfrc522.PICC_HaltA();
    }
  }
  return NO_CARD;
}

void startEngine() {
  Serial.println("Starting Engine is allowed");
}

void soundOK() {
  playTune(VALID);
}
void soundKO() {
  playTune(INVALID);
}

void setup() {
  Serial.begin(9600); // Iniciar serial
  SPI.begin();        // Iniciar SPI
  mfrc522.PCD_Init(); // Iniciar MFRC522
}


void loop() {
  int card = getCardStatus();
  if (VALID == card) {
    soundOK();
    startEngine();
  } else if (INVALID == card) {
    soundKO();
  }

  delay(250);

}
