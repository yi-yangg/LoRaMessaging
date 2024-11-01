// AUTHOR: Chong Yi Yang
// STUDENT ID: 105297292
// COURSE: COS60011-Technology Design Project
/////
//
// Tag Code: This program reads message from serial and transmits messages
// from the tag to a receiver via relays. Each message contains the following header fields:
//
//  SEQ       %4d   // Sequence number of the message
//  TYPE      %4d   // Message type (indicates the kind of message)
//  TAGID     %4d   // Identifier for the source node where the message is coming from
//  DESTID    %4d   // Identifier for the destination node where the message is going to
//  TTL       %4d   // Time-to-live value for the message, indicating its lifespan
//  RSSI      %4d   // Received Signal Strength Indicator, used for assessing signal quality
//  MSGLEN    %4d   // Length of the message sent
//
//
// This setup allows for efficient communication and key exchange in a LoRa-based network.
//
/////

#include <SPI.h>
#include <RH_RF95.h>

// Cryptographic libraries
#include <ChaCha.h>
#include <Curve25519.h>
#include <RNG.h>

// Singleton instance of radio driver
RH_RF95 rf95; // Instantiate RF95 LoRa driver

// Constants
const int MESSAGE_LENGTH = 108; // Length of the full message
const int FIELD_LENGTH = 35;    // Length of the header fields

// Constant message types
const int NORM_MESSAGE_TYPE = 0; // Normal message type
const int PUBLIC_KEY_REQ = 50;   // Public key request message type
const int PUBLIC_KEY_REPLY = 51; // Public key reply message type
const int ENC_KEY = 52;          // Encrypted key message type

// Message fields
int SEQ = 0;          // Sequence number of messages
int TYPE = 0;         // Type of message
const int TAGID = 99; // Transmitter ID (fixed value)
int DESTID = 100;     // Destination ID (Receiver ID)
int RELAYID = 101;    // Relay ID used as Destination ID during key exchange
int TTL = 5;          // Time-to-live for the message
int thisRSSI = 0;     // Received Signal Strength Indicator

// Encryption variables
uint8_t encryptionKey[32] = {0}; // Array to hold derived encryption key
uint8_t iv[12] = {0};            // Initialization vector for ChaCha encryption

// Function to derive a key from a password using HKDF
// void deriveKeyFromPassword(uint8_t* passwordBytes, int passwordLen) {
//   HKDF<SHA256> hkdf; // Instantiate HKDF with SHA256 hashing
//   hkdf.clear();
//   Serial.println(passwordLen);
//   hkdf.setKey(passwordBytes, passwordLen); // Set the key using password bytes
//   hkdf.extract(encryptionKey, 32); // Extract the derived key
//   // Print derived key for debugging
//   Serial.print(F("Derived Key: "));
//   for (int i = 0; i < 32; i++) {
//       Serial.print(encryptionKey[i]);
//       Serial.print(" ");
//   }
//   Serial.println();
// }

void deriveKeyFromPassword(uint8_t *passwordBytes, size_t passwordLen)
{
  // Initialize the derived key with 0s
  for (int i = 0; i < 32; i++)
  {
    encryptionKey[i] = 0;
  }

  // Perform XOR folding to reduce the input to a 32-byte key
  for (int i = 0; i < 32; i++)
  {
    encryptionKey[i] ^= passwordBytes[i % passwordLen];                   // XOR each byte and fold into the derived key
    encryptionKey[i] = (encryptionKey[i] << 1) | (encryptionKey[i] >> 7); // Bit rotate left
  }

  int noOfRounds = 10;

  for (int roundNo = 1; roundNo <= 10; roundNo++)
  {
    for (int i = 0; i < 32; i++)
    {
      encryptionKey[i] ^= encryptionKey[(i + roundNo) % 32];                // XOR each byte and fold into the derived key
      encryptionKey[i] = (encryptionKey[i] << 1) | (encryptionKey[i] >> 7); // Bit rotate left
    }
  }

  Serial.print(F("Derived Key: "));
  for (int i = 0; i < 32; i++)
  {
    Serial.print(encryptionKey[i]);
    Serial.print(" ");
  }
  Serial.println();
}

// Initialize private and public keys
uint8_t privateKey[32] = {0};        // Private key
uint8_t publicKey[32] = {0};         // Public key
uint8_t receiverPublicKey[32] = {0}; // Receiver's public key

// Function to generate public and private keys using Curve25519
void generateKeys()
{
  Curve25519::dh1(publicKey, privateKey); // Generate keys

  // Print the public key
  Serial.print(F("Public Key: "));
  for (int i = 0; i < sizeof(publicKey); i++)
  {
    Serial.print(publicKey[i]);
  }
  Serial.println();

  // Print the private key
  Serial.print(F("Private Key: "));
  for (int i = 0; i < sizeof(privateKey); i++)
  {
    Serial.print(privateKey[i]);
  }
  Serial.println();
}

// Function to send the encrypted encryption key to the receiver
void sendEncryptionKey()
{
  ChaCha chacha2;              // Instantiate ChaCha for encryption
  uint8_t buf[MESSAGE_LENGTH]; // Buffer for the message

  chacha2.setKey(receiverPublicKey, sizeof(receiverPublicKey));           // Set receiver's public key as the key
  chacha2.setIV(iv, sizeof(iv));                                          // Set initialization vector
  uint8_t encryptedEncKey[32] = {0};                                      // Buffer for the encrypted encryption key
  chacha2.encrypt(encryptedEncKey, encryptionKey, sizeof(encryptionKey)); // Encrypt the encryption key

  // Print the encrypted encryption key for debugging
  Serial.print("Encrypted encryption key: ");
  for (int i = 0; i < sizeof(encryptedEncKey); i++)
  {
    Serial.print(encryptedEncKey[i]);
  }
  Serial.println();

  // Safe formatting initial message with fields
  snprintf(buf, FIELD_LENGTH + 1, "%4d %4d %4d %4d %4d %4d %4d ", -1, ENC_KEY, TAGID, RELAYID, TTL, thisRSSI, sizeof(encryptedEncKey));
  memcpy(buf + FIELD_LENGTH, encryptedEncKey, 32); // Append encrypted key to the buffer

  sendMessage(buf, MESSAGE_LENGTH); // Send the message
}

// Setup function runs once at startup
void setup()
{
  // Initialise LoRa transceiver
  Serial.begin(9600); // Start serial communication at 9600 baud
  Serial.println(F("Starting Transmitter Node"));

  while (!Serial)
    Serial.println(F("Waiting for serial port")); // Wait for serial port to be available.

  // Initialize the RF95 module and check for errors
  while (!rf95.init())
  {
    Serial.println(F("Initialisation of LoRa receiver failed"));
    delay(1000); // Delay and try again if initialization fails
  }

  // Setup RNG for random number generation
  RNG.begin("LoRa based messaging system (Transmitter)");

  Serial.println(F("Waiting for password..."));

  // Get password from web and convert to bytes array
  String passwordString = "";

  // Loop until a password is received from serial
  while (passwordString.length() == 0)
  {
    if (Serial.available() > 0)
    {
      passwordString = Serial.readStringUntil("\n"); // Read password until newline
    }
  }

  int len = passwordString.length();

  Serial.print(F("Password: "));
  Serial.println(passwordString); // You can print the whole string

  // Derive key from password
  uint8_t passwordBytes[len];

  // Convert password string to byte array
  for (int i = 0; i < len; i++)
  {
    passwordBytes[i] = passwordString[i];
  }
  Serial.println();

  // Derive encryption key from password using HKDF
  deriveKeyFromPassword(passwordBytes, len);

  rf95.setFrequency(915.0);        // Set the frequency for the RF95 module
  rf95.setTxPower(5, false);       // Set transmission power
  rf95.setSignalBandwidth(500000); // Set signal bandwidth
  rf95.setSpreadingFactor(12);     // Set spreading factor for LoRa

  // DFKE using Curve25519 (Derive Key From Key Exchange)
  generateKeys(); // Generate public/private keys

  Serial.println(F("Initiating key exchange"));
  Serial.println(F("--------------------------------------------"));

  // Send public key to relay node
  sendPublicKeyRequest();

  // Print the public key based on the destination
  if (RELAYID != DESTID)
    Serial.print(F("Relay Public Key: "));
  else
    Serial.print(F("Receiver Public Key: "));

  for (int i = 0; i < sizeof(publicKey); i++)
  {
    Serial.print(receiverPublicKey[i]);
  }
  Serial.println();

  // Receive public key from relay node
  receivePublicKeyReq();

  // Get shared secret using receiver's public key
  Curve25519::dh2(receiverPublicKey, privateKey); // Derive shared secret
  Serial.print(F("Shared secret key: "));
  for (int i = 0; i < sizeof(receiverPublicKey); i++)
  {
    Serial.print(receiverPublicKey[i]);
  }
  Serial.println();

  Serial.println(F("Sending encryption key"));
  Serial.println("--------------------------------------------");

  // Encrypt encryption key and send to relay node
  sendEncryptionKey();

  Serial.println(F("System is ready")); // Indicate system readiness
}

// Loop function runs repeatedly after setup
void loop()
{
  if (Serial.available() > 0)
  {                      // Check if there is incoming serial data
    normalMessageSend(); // Send a normal message
    delay(3000);         // Wait for the specified transmission interval
  }
}

// Function to send a public key request
void sendPublicKeyRequest()
{
  uint8_t publicKeyBuf[MESSAGE_LENGTH];  // Buffer for incoming public key reply
  uint8_t bufLen = sizeof(publicKeyBuf); // Length of the buffer for incoming message

  const long req_interval = 3000; // Request every 3 seconds
  long previousMillis = millis(); // Store the last time a request was sent

  // Constantly loop to check send public key request and listen for public key reply
  while (true)
  {

    long currentMillis = millis(); // Get the current time
    // Check if it's time to send the next request
    if (currentMillis - previousMillis >= req_interval)
    {
      previousMillis = currentMillis; // Update the last request time
      uint8_t buf[FIELD_LENGTH];      // Buffer for sending the request
      // Safe formatting initial message with fields
      snprintf(buf, FIELD_LENGTH + 1, "%4d %4d %4d %4d %4d %4d %4d ", -1, PUBLIC_KEY_REQ, TAGID, RELAYID, TTL, thisRSSI, 0);
      sendMessage(buf, FIELD_LENGTH); // Send the public key request
    }
    // Listen for all signals (looking for reply)
    if (rf95.available() && rf95.recv(publicKeyBuf, &bufLen))
    {
      int seq, type, tagID, destID, ttl, rssi, msgLen; // Variables to store message fields
      // Read the incoming message fields
      int itemsRead = sscanf(publicKeyBuf, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);
      // Check if the message is a valid public key reply
      if (type == PUBLIC_KEY_REPLY && destID == TAGID && itemsRead == 7)
      {
        // Get public key from the buffer
        memcpy(receiverPublicKey, publicKeyBuf + FIELD_LENGTH, sizeof(receiverPublicKey)); // Copy the public key
        break;                                                                             // Exit the loop after receiving the public key
      }
    }
  }
}

// Function to receive a public key request
void receivePublicKeyReq()
{
  while (true)
  {
    uint8_t publicKeyBuf[MESSAGE_LENGTH]; // Buffer for incoming messages
    uint8_t len = sizeof(publicKeyBuf);   // Length of the buffer
    // Check if a message is available and read it
    if (rf95.available() && rf95.recv(publicKeyBuf, &len))
    {
      int seq, type, tagID, destID, ttl, rssi, msgLen; // Variables to store message fields
      // Extract subfields and look for public key request
      int itemsRead = sscanf(publicKeyBuf, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);

      // If the message is sent to itself
      if (itemsRead == 7 && destID == TAGID && type == PUBLIC_KEY_REQ)
      {
        sendPublicKey(); // Respond with the public key
        break;           // Exit the loop after sending the public key
      }
    }
  }
}

// Function to send the public key in response to a request
void sendPublicKey()
{
  uint8_t buf[MESSAGE_LENGTH]; // Buffer for the outgoing message
  // Safe formatting initial message with fields
  snprintf(buf, FIELD_LENGTH + 1, "%4d %4d %4d %4d %4d %4d %4d ", -1, PUBLIC_KEY_REPLY, TAGID, RELAYID, TTL, thisRSSI, sizeof(publicKey));

  // Copy the public key to the buffer
  memcpy(buf + FIELD_LENGTH, publicKey, 32); // Append the public key to the message
  sendMessage(buf, FIELD_LENGTH + 32);       // Send the public key message
}

// Function to send a message via RF
void sendMessage(uint8_t *buf, int bufLen)
{

  rf95.setModeIdle(); // Set the RF module to idle mode to prevent message loss
  // Wait for the channel to be idle before sending
  while (rf95.isChannelActive())
  {
    delay(10); // Wait for the channel to go idle
    // Serial.println(F("Tag node looping on isChannelActive()")); //DEBUG
  }
  rf95.send(buf, bufLen); // Send the message
  rf95.waitPacketSent();  // Wait until the packet is sent
}

// Function to encrypt a message using ChaCha encryption
void encryptMessage(uint8_t *message, int msgLen)
{
  ChaCha chacha;                                       // Create a ChaCha instance
  chacha.clear();                                      // Clear any previous state
  chacha.setKey(encryptionKey, sizeof(encryptionKey)); // Set the encryption key
  chacha.setIV(iv, sizeof(iv));                        // Set the initialization vector
  chacha.encrypt(message, message, msgLen);            // Encrypt the message in place
}

// Function to read a message from serial input
void getMessageFromSerial(uint8_t *message, int *msgLen)
{
  // Read input message from web UI
  while (Serial.available() > 0)
  {
    // Ensure there is space in the message buffer
    if (*msgLen < MESSAGE_LENGTH - 1)
    {
      message[(*msgLen)++] = Serial.read(); // Read a byte from serial input
      delay(10);                            // Ensure all input is read
    }
    else
      break; // Exit if the message buffer is full
  }
}

// Function to send a normal message
void normalMessageSend()
{
  uint8_t buf[MESSAGE_LENGTH]; // Buffer for the outgoing message
  // Increment sequence number for each message
  SEQ++;

  int msgLen = 0;                                 // Length of the message
  uint8_t message[MESSAGE_LENGTH - FIELD_LENGTH]; // Buffer for the message content

  getMessageFromSerial(message, &msgLen); // Read message from serial input

  // Format the message with fields
  snprintf(buf, FIELD_LENGTH + 1, "%4d %4d %4d %4d %4d %4d %4d ", SEQ, NORM_MESSAGE_TYPE, TAGID, DESTID, TTL, thisRSSI, msgLen);
  // Copy the message content to the buffer
  memcpy(buf + FIELD_LENGTH, message, msgLen);
  Serial.println(F("Sending message"));                           // Log sending message
  Serial.println("--------------------------------------------"); // Separator for readability
  Serial.print(F("Original Message: "));                          // Print original message
  for (int i = 0; i < FIELD_LENGTH + msgLen; i++)
  {
    Serial.print((char)buf[i]); // Print each byte of the message
  }
  Serial.println();

  Serial.println(F("Encrypting message...")); // Log encryption process

  encryptMessage(buf, FIELD_LENGTH + msgLen); // Encrypt the message

  Serial.print("Encrypted message: "); // Print encrypted message
  for (int i = 0; i < FIELD_LENGTH + msgLen; i++)
  {
    Serial.print((char)buf[i]); // Print each byte of the encrypted message
  }
  Serial.println();

  sendMessage(buf, FIELD_LENGTH + msgLen); // Send the encrypted message
}
