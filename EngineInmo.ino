//RST          D5
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
#define CLONE 3
#define CONTACT 4

#define STARTER_PIN 3
#define ENGINE_PIN 4

#define ACC_PIN 2
#define RED_LED 6
#define GREEN_LED 7

#define RST_PIN 5 // Pin para el reset del RC522
#define SS_PIN 10 // Pin  para el SS (SDA) del RC522
#define speaker_pin 9 // For the sound

#define ACC_OFF 1
#define ACC_ON 0


const int TRACE = 0;
const int DEBUG = 1;
const int ERROR_ = 2;
const int WARN = 3;
const int INFO = 4;
const int LOG_LEVEL = DEBUG;

int INIT = 0;
int LOCKED = 1;
int UNLOCKED_LEARNING = 2;
int UNLOCKED = 3;
int UNLOCKED_WAITING = 4;
unsigned long LEARNING_TIME = 10000;//milliseconds
unsigned long ENGINE_TIME =  120000;//milliseconds
unsigned long IDLE_TIME = 300000;//milliseconds

bool RED_STATUS = false;
bool GREEN_STATUS = false;

int OLD_STATUS = -1;
int MAIN_STATUS = INIT;
int TIMER = 0;
unsigned char data[16] = {'J', 'o', 's', 'e', '_', 'J', 'N', ' ', 'V', 'C', '3', '8', '4', '8', '3', '*'};
byte VOID_ID[4] = {0, 0, 0, 0};

MFRC522::MIFARE_Key key;

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Crear instancia del MFRC522

byte readUID[4]; // Almacena el tag leido
byte validKey1[4] = { 0x05, 0x99, 0x48, 0xB5 };  // Ejemplo de clave valida
//D3 5B E3 C7

// Melodies definition: access, welcome and rejection
int access_melody[] = {NOTE_FS7, NOTE_DS7};
int access_noteDurations[] = {2, 2};

int fail_melody[] = {NOTE_DS7, NOTE_DS7, NOTE_DS7};
int fail_noteDurations[] = {8, 8, 8};

int clone_melody[] = {NOTE_B6, NOTE_DS7, NOTE_FS7};
int clone_noteDurations[] = {8, 8, 8};

int contact_melody[] = {NOTE_DS7, NOTE_FS7};
int contact_noteDurations[] = {8, 8};

//========== Logging function ==========
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
  else if (Scan == 2) { // A Bad card read
    for (int i = 0; i < 3; i++)    //loop through the notes
    {
      int fail_noteDuration = 1000 / fail_noteDurations[i];
      tone(speaker_pin, fail_melody[i], fail_noteDuration);
      int fail_pauseBetweenNotes = fail_noteDuration * 1.30;
      delay(fail_pauseBetweenNotes);
      noTone(speaker_pin);
    }
  }
  else if (Scan == 3) { // A Bad card read
    for (int i = 0; i < 3; i++)    //loop through the notes
    {
      int clone_noteDuration = 1000 / clone_noteDurations[i];
      tone(speaker_pin, clone_melody[i], clone_noteDuration);
      int clone_pauseBetweenNotes = clone_noteDuration * 1.30;
      delay(clone_pauseBetweenNotes);
      noTone(speaker_pin);
    }
  }
  else if (Scan == 4) { // A Contact and ok card
    for (int i = 0; i < 2; i++)    //loop through the notes
    {
      int contact_noteDuration = 1000 / contact_noteDurations[i];
      tone(speaker_pin, contact_melody[i], contact_noteDuration);
      int contact_pauseBetweenNotes = contact_noteDuration * 1.30;
      delay(contact_pauseBetweenNotes);
      noTone(speaker_pin);
    }
  }
  delay(50);
  pinMode(speaker_pin, INPUT);
}

//========== Array Utils ==========
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

//========== CARD Utils ==========
bool prepareCard() {
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
  return true;
}

bool rfidAuthenticate(byte trailerBlock) {
  MFRC522::StatusCode status;
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    String logLine =  "Card authentication failed ";
    logLine = logLine.concat(mfrc522.GetStatusCodeName(status));
    logger(WARN, logLine);
    return false;
  }
  logger(DEBUG, "Card authenticated");
  return true;
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

byte* rfidReadData(byte blockAddr) {
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

byte* getCardId() {
  logger(DEBUG, "Reading id!");
  printArray(mfrc522.uid.uidByte, 4, DEBUG);
  return mfrc522.uid.uidByte;
}

boolean isValidCardById() {
  byte* myid = getCardId();
  if (isEqualArray(myid, validKey1, 4)) {
    logger(INFO, "Valid card by ID");
    return true;
  }
  logger(INFO, "Invalid card by ID");
  return false;
}

bool isValidCardByData() {
  if (rfidAuthenticate(7)) {
    byte* mydata = rfidReadData(4);
    if (isEqualArray(mydata, data, 16)) {
      logger(INFO, "Valid card by DATA");
      return true;
    }
    logger(INFO, "Invalid card by DATA");
    return false;
  }
  logger(DEBUG, "No authentication");
  return false;
}

int waitForCard() {
  int card = NO_CARD;
  if (prepareCard()) {
    if (isValidCardById()) {
      card = VALID;
    }
    else if (isValidCardByData()) {
      card = VALID;
    }
    else {
      card = INVALID;
    }
  }
  else {
    card = NO_CARD;
  }
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return card;
}

//========== Security Car API ==========
void startEngine() {
  logger(INFO, "Starting Engine is allowed");
  pinMode(ENGINE_PIN, OUTPUT);
  digitalWrite(ENGINE_PIN, HIGH);
  pinMode(STARTER_PIN, OUTPUT);
  digitalWrite(STARTER_PIN, HIGH);

}

void blink_red(){
  RED_STATUS = !RED_STATUS;
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, RED_STATUS);
  }
  
void blink_green(){
  GREEN_STATUS = !GREEN_STATUS;
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, GREEN_STATUS);
  }
  
void on_red(){
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, HIGH);
  }
  
void on_green(){
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, HIGH);
  }
  
void off_red(){
  pinMode(RED_LED, OUTPUT);
  digitalWrite(RED_LED, LOW);
  }
  
void off_green(){
  pinMode(GREEN_LED, OUTPUT);
  digitalWrite(GREEN_LED, LOW);
  }

void stopEngine() {
  logger(INFO, "Stopping Engine");
  pinMode(ENGINE_PIN, OUTPUT);
  digitalWrite(ENGINE_PIN, LOW);
  pinMode(STARTER_PIN, OUTPUT);
  digitalWrite(STARTER_PIN, LOW);

}

void soundOK() {
  playTune(VALID);
}

void soundKO() {
  playTune(INVALID);
}

void soundCloned() {
  playTune(CLONE);
}

void soundOK_and_Contact(){
  playTune(CONTACT);
}

//========== Events ==========
void on_init() {
  unsigned long timer = millis() + ENGINE_TIME;
  int card = NO_CARD;

 
  if (OLD_STATUS != INIT) {
    OLD_STATUS = MAIN_STATUS;
    logger(INFO, "New Status is INIT");
  }

  //Espero hasta tener tarjeta
  while (card == NO_CARD) {
    blink_red();
    card = waitForCard();
    if (timer <= millis()) {
      MAIN_STATUS = LOCKED;
      return;
    }
    delay(250);
  }
  //Si es valida, salto al nuevo estado
  if (card == VALID) {
    MAIN_STATUS = UNLOCKED_LEARNING;
  } else {
    soundKO();
  }
}

void on_locked() {
  if (OLD_STATUS != LOCKED) {
    OLD_STATUS = MAIN_STATUS;
    logger(INFO, "New Status is LOCKED");
    logger(WARN, "Time Expired! Locking engine");
    stopEngine();
    off_green();
    on_red();
  }
  int card = waitForCard();
  //Si es valida, salto al nuevo estado
  if (card == VALID) {
    MAIN_STATUS = UNLOCKED_LEARNING;
  } else if (card == INVALID) {
    soundKO();
  }
}

void on_unlocked_learning() {
  if (OLD_STATUS != UNLOCKED_LEARNING) {
    OLD_STATUS = MAIN_STATUS;
    logger(INFO, "New Status is UNLOCKED_LEARNING");
    off_red();
    startEngine();
    soundOK();

    int card;
    unsigned long timer = millis() + LEARNING_TIME;

    while (true) {
      blink_green();
      if (prepareCard()) {
        if (!isValidCardByData()) {
          logger(INFO, "Cloning Card!");
          rfidWriteData(7, 1, 4, data);
          unsigned char *data2 = rfidReadData(4);

          if (isEqualArray(data, data2, 16)) {
            logger(INFO, "Cloning OK!");
            soundCloned();
          } else {
            logger(WARN, "Cloning KO!");
            soundKO();
          }

          timer = millis() + LEARNING_TIME;
          logger(DEBUG, "Learning Time extended!");

        } else {
          logger(INFO, "Already Paired!");
          timer = millis() + LEARNING_TIME;
          logger(DEBUG, "Learning Time extended!");
          soundOK();
        }
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
      }
      if (timer <= millis()) {
        break;
      }
      delay(250);
    }
    MAIN_STATUS = UNLOCKED_WAITING;
  }

}

void on_unlocked() {
  soundOK_and_Contact();
  if (OLD_STATUS != UNLOCKED) {
    OLD_STATUS = MAIN_STATUS;
    off_red();
    on_green();
    logger(INFO, "New Status is UNLOCKED");
  }
  bool acc = ACC_ON;
  while (acc == ACC_ON) {
    acc = digitalRead(ACC_PIN);
    delay(250);
  }
  MAIN_STATUS = UNLOCKED_WAITING;

}

void on_unlocked_waiting() {
  if (OLD_STATUS != UNLOCKED_WAITING) {
    OLD_STATUS = MAIN_STATUS;
    logger(INFO, "New Status is UNLOCKED_WAITING");
    on_green();
    unsigned long timer = millis() + IDLE_TIME;
    bool acc = ACC_OFF;
    while (acc == ACC_OFF) {
      blink_red();
      acc = digitalRead(ACC_PIN);

      if (timer <= millis()) {
        logger(WARN, "Max IDLE time! Locking engine");
        MAIN_STATUS = LOCKED;
        soundKO();
        return;
      }

      delay(250);
    }
    MAIN_STATUS = UNLOCKED;
  }
}

//========== MAIN PROGRAM ==========
void setup() {
  Serial.begin(9600); // Iniciar serial
  SPI.begin();        // Iniciar SPI
  mfrc522.PCD_Init(); // Iniciar MFRC522
  pinMode(ACC_PIN, INPUT);

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

void loop() {
  if (MAIN_STATUS == INIT) {
    on_init();
  }

  else if (MAIN_STATUS == LOCKED) {
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
}
