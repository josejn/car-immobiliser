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

const int TRACE = 0;
const int DEBUG = 1;
const int ERROR_ = 2;
const int WARN = 3;
const int INFO = 4;
const int LOG_LEVEL = TRACE;

const int RST_PIN = 5; // Pin 9 para el reset del RC522
const int SS_PIN = 10; // Pin 10 para el SS (SDA) del RC522
const int speaker_pin = 9; // For the sound

int LOCKED = 0;
int UNLOCKED_LEARNING = 1;
int UNLOCKED = 2;
int UNLOCKED_WAITING = 0;
int LEARNING_TIME = 6000;

int OLD_STATUS = -1;
int MAIN_STATUS = LOCKED;
int TIMER = 0;
unsigned char data[16] = {'O', 'G', 'T', ' ', 'K', 'L', 'O', 'R', ' ', 'J', 'O', 'S', 'E', '_', 'J', 'N'};

MFRC522::MIFARE_Key key;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Crear instancia del MFRC522

byte readUID[4]; // Almacena el tag leido
byte validKey1[4] = { 0x03, 0x01, 0xE2, 0xC7 };  // Ejemplo de clave valida
//D3 5B E3 C7

// Melodies definition: access, welcome and rejection
int access_melody[] = {NOTE_FS7, NOTE_DS7};
int access_noteDurations[] = {2, 2};
int fail_melody[] = {NOTE_DS7, NOTE_DS7, NOTE_DS7};
int fail_noteDurations[] = {8, 8, 8};

//***************** Logging function
void logger(int mode, String msg) {
  if (mode >= LOG_LEVEL) {

    switch (mode) {
      case TRACE:
        Serial.print("[TRACE] ");
        break;
      case DEBUG:
        Serial.print("[DEBUG] ");
        break;
      case ERROR_:
        Serial.print("[ERROR] ");
        break;
      case WARN:
        Serial.print("[WARN] ");
        break;
      default:
        Serial.print("[INFO] ");
        break;
    }
    Serial.print(millis());
    Serial.print(" - ");
    Serial.println(msg);
  } else {

  }
}

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

void printArray(byte *buffer, byte bufferSize, int level) {
  String logLine = "";
  for (byte i = 0; i < bufferSize; i++) {
    logLine += (buffer[i] < 0x10 ? " 0" : " ");
    logLine += String(buffer[i], HEX);
  }
  logger(level, logLine);
}

void printArrayAsString(byte *buffer, byte bufferSize, int level) {
  String logLine = "";
  for (byte i = 0; i < bufferSize; i++) {
    logLine += char(buffer[i]);
  }
  logger(level, logLine);
}


//Función para comparar dos vectores
bool isEqualArray(byte arrayA[], byte arrayB[], int length)
{
  for (int index = 0; index < length; index++)
  {
    if (LOG_LEVEL <= TRACE) {
      String logLine = "";
      logLine += String(arrayA[index], HEX);
      logLine += "-";
      logLine += (arrayB[index], HEX);
      logger(TRACE, logLine);
    }
    if (arrayA[index] != arrayB[index])
      return false;
  }
  return true;
}

bool rfidAuthenticate(byte trailerBlock) {
  MFRC522::StatusCode status;
  if (!mfrc522.PICC_IsNewCardPresent())
  {
    return false;
  }
  //Seleccionamos una tarjeta
  if (!mfrc522.PICC_ReadCardSerial())
  {
    logger(DEBUG, "Card is not ready");
    return false;
  }

  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    String logLine =  "Card authentication failed ";
    logLine = logLine.concat(mfrc522.GetStatusCodeName(status));
    logger(DEBUG, logLine);
    return false;
  }
  logger(INFO, "Card authenticated");
  return true;
}

void rfidCloseSession() {
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
}

void rfidWriteData(byte trailerBlock, byte sector, byte blockAddr, byte data[]) {
  MFRC522::StatusCode status;
   if (LOG_LEVEL <= DEBUG) {
    String logLine = "Writing data into Card: ";
    logLine += blockAddr;
    logger(DEBUG, logLine);
    printArray((byte*)data, 16 , DEBUG);
    printArrayAsString((byte*)data, 16 , DEBUG);
  }
  // Write data to the block

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, (byte*)data, 16);
  if (status != MFRC522::STATUS_OK) {
    String logLine =  "Card WRITE failed: ";
    logLine = logLine.concat(mfrc522.GetStatusCodeName(status));
    logger(DEBUG, logLine);
  }
}

byte* rfidReadData(byte trailerBlock, byte blockAddr) {
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);

  logger(INFO, "Reading Card");

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    String logLine =  "Card READ failed: ";
    logLine = logLine.concat(mfrc522.GetStatusCodeName(status));
    logger(DEBUG, logLine);
  }

  if (LOG_LEVEL <= DEBUG) {
    printArray(buffer, 16, DEBUG);
    printArrayAsString(buffer, 16, DEBUG);
  }
  return buffer;
}

int getCardStatus() {

  // Detectar tarjeta
  if (mfrc522.PICC_IsNewCardPresent())
  {
    //Seleccionamos una tarjeta
    if (mfrc522.PICC_ReadCardSerial())
    {
      unsigned char *data2 = rfidReadData(7, 4);
      printArrayAsString(data2, 16, DEBUG);
      // Comparar ID con las claves validas
      if (isEqualArray(mfrc522.uid.uidByte, validKey1, 4)) {
        logger(INFO, "Valid Card detected by ID");
        return VALID;
      } else 
      if (isEqualArray(data2, data, 16)) {
        logger(INFO, "Valid Card detected by DATA");
        return VALID;
      }
      else {
        if (LOG_LEVEL <= DEBUG) {
          printArray(mfrc522.uid.uidByte, mfrc522.uid.size, DEBUG);
        }
        logger(WARN, "INVALID Card detected!");
        return INVALID;
      }
      // Finalizar lectura actual
      mfrc522.PICC_HaltA();
    }
  }
  return NO_CARD;
}

void startEngine() {
  logger(INFO, "Starting Engine is allowed");
}

void soundOK() {
  playTune(VALID);
}

void soundKO() {
  playTune(INVALID);
}

void pairCard() {
  delay(250);
  if (rfidAuthenticate(7)) {
    rfidWriteData(7, 1, 4, data);
    unsigned char *data2 = rfidReadData(7, 4);
    rfidCloseSession();
  } else {
    logger(WARN, "Error authenticating RFIDd");
  }
}

void on_locked() {
  if (OLD_STATUS != LOCKED) {
    OLD_STATUS = MAIN_STATUS;
    logger(INFO, "New Status is LOCKED");
  }
  int card;
  //Espero hasta tener tarjeta
  while ((card = getCardStatus()) == NO_CARD) {
    delay(250);
  }

  //Si es valida, salto al nuevo estado
  if (card == VALID) {
    MAIN_STATUS = UNLOCKED_LEARNING;
  } else if (INVALID == card) {
    soundKO();
  }
}

void on_unlocked_learning() {
  if (OLD_STATUS != UNLOCKED_LEARNING) {
    OLD_STATUS = MAIN_STATUS;
    logger(INFO, "New Status is UNLOCKED_LEARNING");

    //Do stuff here
    soundOK();
    startEngine();
    int card;
    int timer = millis() + LEARNING_TIME;

    while (true) {
      pairCard();

      if (timer <= millis()) {
        break;
      }
      delay(250);
    }
    MAIN_STATUS = UNLOCKED;
  }

}

void on_unlocked() {
  if (OLD_STATUS != UNLOCKED) {
    OLD_STATUS = MAIN_STATUS;

    logger(INFO, "New Status is UNLOCKED");
    //Do stuff here
  }
}

void on_unlocked_waiting() {
  if (OLD_STATUS != UNLOCKED_WAITING) {
    OLD_STATUS = MAIN_STATUS;
    logger(INFO, "New Status is UNLOCKED_WAITING");
    //Do stuff here
  }

}

void setup() {
  Serial.begin(9600); // Iniciar serial
  SPI.begin();        // Iniciar SPI
  mfrc522.PCD_Init(); // Iniciar MFRC522

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}


void loop() {


  if (MAIN_STATUS == LOCKED) {
    on_locked();
  }

  else if (MAIN_STATUS == UNLOCKED_LEARNING) {
    on_unlocked_learning();
  }

  else if (MAIN_STATUS == UNLOCKED) {
    on_unlocked();
  }

  else if (MAIN_STATUS == UNLOCKED_WAITING) {
    on_unlocked_waiting();
  }


  //  unsigned char data[16] = {'J', 'o', 's', 'e', '_', 'J', 'n', ' ', 'I', 'N', 'M', 'O', '_', 'B', 'M', 'W'};

  //  if (rfidAuthenticate(7)) {
  //    rfidWriteData(7, 1, 4, data);
  //    unsigned char *data2 = rfidReadData(7, 4);
  //    rfidCloseSession();
  //  } else {
  //    Serial.println("Error authenticating RFID");
  //  }
}
