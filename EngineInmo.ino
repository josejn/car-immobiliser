//RST          D9
//SDA(SS)      D10
//MOSI         D11
//MISO         D12
//SCK          D13

#include <SPI.h>
#include <MFRC522.h>

#define VALID 1
#define INVALID 2
#define NO_CARD 3

const bool DEBUG = true;
const int RST_PIN = 5; // Pin 9 para el reset del RC522
const int SS_PIN = 10; // Pin 10 para el SS (SDA) del RC522
const int speakerOut = 9; // For the sound


MFRC522 mfrc522(SS_PIN, RST_PIN);   // Crear instancia del MFRC522

byte readUID[4]; // Almacena el tag leido
byte validKey1[4] = { 0x03, 0x01, 0xE2, 0xC7 };  // Ejemplo de clave valida
//D3 5B E3 C7

//****************************
//****** FOR MUSIC ***********
#define  c     3830    // 261 Hz
#define  d     3400    // 294 Hz
#define  e     3038    // 329 Hz
#define  f     2864    // 349 Hz
#define  g     2550    // 392 Hz
#define  a     2272    // 440 Hz
#define  b     2028    // 493 Hz
#define  C     1912    // 523 Hz 
#define  R     0
// MELODY and TIMING  =======================================
//  melody[] is an array of notes, accompanied by beats[],
//  which sets each note's relative length (higher #, longer note)
int melody[] = {  C,  b,  g,  C,  b,   e,  R,  C,  c,  g, a, C };
int beats[]  = { 16, 16, 16,  8,  8,  16, 32, 16, 16, 16, 8, 8 };
int MAX_COUNT = sizeof(melody) / 2; // Melody length, for looping.

// Set overall tempo
long tempo = 10000;
// Set length of pause between notes
int pause = 1000;
// Loop variable to increase Rest length
int rest_count = 100; //<-BLETCHEROUS HACK; See NOTES

// Initialize core variables
int tone_ = 0;
int beat = 0;
long duration  = 0;



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
      if (isEqualArray(mfrc522.uid.uidByte, validKey1, 4)){
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
  Serial.println("Playing ok tone");
  // PLAY TONE  ==============================================
  // Pulse the speaker to play a tone for a particular duration
  long elapsed_time = 0;
  if (tone_ > 0) { // if this isn't a Rest beat, while the tone has
    //  played less long than 'duration', pulse speaker HIGH and LOW
    while (elapsed_time < duration) {

      digitalWrite(speakerOut,HIGH);
      delayMicroseconds(tone_ / 2);

      // DOWN
      digitalWrite(speakerOut, LOW);
      delayMicroseconds(tone_ / 2);

      // Keep track of how long we pulsed
      elapsed_time += (tone_);
    }
  }
  else { // Rest beat; loop times delay
    for (int j = 0; j < rest_count; j++) { // See NOTE on rest_count
      delayMicroseconds(duration);  
    }                                
  }                                
}

void soundKO() {
  Serial.println("Playing ko! tone");
}

void setup() {
  Serial.begin(9600); // Iniciar serial
  SPI.begin();        // Iniciar SPI
  mfrc522.PCD_Init(); // Iniciar MFRC522
}


void loop() {
  int card = getCardStatus();
  if (VALID == card){
    soundOK();
    startEngine();  
  } else if (INVALID == card){
    soundKO();  
  }
  
  delay(250);

}
