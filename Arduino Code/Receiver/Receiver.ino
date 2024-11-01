// AUTHOR: Chong Yi Yang
// STUDENT ID: 105297292
// COURSE: COS60011-Technology Design Project
/////
//
// Receiver code. Listens for messages, decrypting them and receives the message
// Message format is
//
//  SEQ       %4d   // Sequence number of the message
//  TYPE      %4d   // Message type (indicates the kind of message)
//  TAGID     %4d   // Identifier for the source node where the message is coming from
//  DESTID    %4d   // Identifier for the destination node where the message is going to
//  TTL       %4d   // Time-to-live value for the message, indicating its lifespan
//  RSSI      %4d   // Received Signal Strength Indicator, used for assessing signal quality
//  MSGLEN    %4d   // Length of the message sent
//
/////

#include <SPI.h>
#include <RH_RF95.h>

// Cryptographic libraries
#include <ChaCha.h>
#include <Curve25519.h>
#include <RNG.h>

// Singleton instance of radio driver
RH_RF95 rf95;

// Constants
int MESSAGE_LENGTH = 108;   // Maximum input message length is MESSAGE_LENGTH - 1 (due to '\0' at the end)
int FIELD_LENGTH = 35;      // Field length for message formatting
const double CSMATIME = 10; // Time to wait for the channel to become idle

// Node and message identifiers
const int TAGID = 100;  // Unique identifier for this TAG
const int DESTID = 99;  // Destination ID for the messages
const int RELAYID = 99; // Relay ID for this node
int TTL = 5;            // Time To Live for messages
int thisRSSI = 0;       // RSSI value for the node's signal strength

// Constant message types
const int PUBLIC_KEY_REQ = 50;   // Request for public key
const int PUBLIC_KEY_REPLY = 51; // Reply containing public key
const int ENC_KEY = 52;          // Encrypted key message type

// Initialize private and public keys
uint8_t privateKey[32] = {0};           // Storage for private key
uint8_t publicKey[32] = {0};            // Storage for public key
uint8_t transmitterPublicKey[32] = {0}; // Public key of the transmitter
uint8_t encryptionKey[32] = {0};        // Storage for the encryption key
uint8_t iv[12] = {0};                   // Initialization vector for encryption/decryption

// Function to generate public and private keys
void generateKeys()
{
  // Generate public and private keys using Curve25519
  Curve25519::dh1(publicKey, privateKey);

  // Print the generated public key
  Serial.print(F("Public Key: "));
  for (int i = 0; i < sizeof(publicKey); i++)
  {
    Serial.print(publicKey[i]);
  }
  Serial.println();

  // Print the generated private key
  Serial.print(F("Private Key: "));
  for (int i = 0; i < sizeof(privateKey); i++)
  {
    Serial.print(privateKey[i]);
  }
  Serial.println();
}

void setup()
{
  Serial.begin(9600);                          // Start serial communication at 9600 baud rate
  Serial.println(F("Starting Receiver Node")); // Print startup message
  while (!Serial)
    ; // Wait for the serial port to become available

  // Initialize LoRa receiver and retry if it fails
  while (!rf95.init())
  {
    // Serial.println("Initialisation of LoRa receiver failed");
    delay(1000); // Wait for a second before retrying
  }

  RNG.begin("LoRa based messaging system (Receiver)"); // Initialize random number generator

  // Configure the RF95 module
  rf95.setFrequency(915.0);        // Set frequency to 915 MHz
  rf95.setTxPower(23, false);      // Set transmit power
  rf95.setSignalBandwidth(500000); // Set signal bandwidth
  rf95.setSpreadingFactor(12);     // Set spreading factor for LoRa

  // Generate keys for secure communication
  generateKeys();

  Serial.println("Initiating key exchange"); // Print key exchange initiation message
  Serial.println("--------------------------------------------");

  // Start the key exchange process
  receivePublicKeyReq();  // Receive public key requests
  sendPublicKeyRequest(); // Send a public key request

  // Print the public key based on whether it's a relay or a destination
  if (RELAYID != DESTID)
    Serial.print(F("Relay Public Key: "));
  else
    Serial.print(F("Transmitter Public Key: "));

  // Print the received public key
  for (int i = 0; i < sizeof(publicKey); i++)
  {
    Serial.print(transmitterPublicKey[i]);
  }
  Serial.println();

  // Compute the shared key using the transmitter's public key
  Curve25519::dh2(transmitterPublicKey, privateKey);

  // Print the shared key
  Serial.print(F("Shared key: "));
  for (int i = 0; i < sizeof(transmitterPublicKey); i++)
  {
    Serial.print(transmitterPublicKey[i]);
  }
  Serial.println();

  Serial.println(F("Receiving encryption key")); // Notify of encryption key reception
  Serial.println("--------------------------------------------");
  receiveEncryptionKey(); // Receive the encryption key

  Serial.println(F("System is ready")); // Notify that the system is ready for operation
}

// Function to receive the encryption key
void receiveEncryptionKey()
{
  while (true)
  {
    uint8_t encKeyBuf[MESSAGE_LENGTH]; // Buffer for the encrypted key
    uint8_t len = sizeof(encKeyBuf);   // Length of the buffer
    // Check if a message is available
    if (rf95.available() && rf95.recv(encKeyBuf, &len))
    {
      int seq, type, tagID, destID, ttl, rssi, msgLen; // Variables for message fields
      // Extract subfields and look for an encrypted key request
      int itemsRead = sscanf(encKeyBuf, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);

      // Check if we read the expected number of items and if the destination ID matches
      if (itemsRead == 7 && destID == TAGID && type == ENC_KEY)
      {
        // Copy the encrypted key from the buffer
        memcpy(encryptionKey, encKeyBuf + FIELD_LENGTH, sizeof(encryptionKey));

        Serial.print(F("Encrypted encryption key: ")); // Notify of the encrypted key
        for (int i = 0; i < sizeof(encryptionKey); i++)
        {
          Serial.print(encryptionKey[i]); // Print encrypted encryption key
        }
        Serial.println();

        Serial.println(F("Decrypting encryption key...")); // Notify of decryption process
        ChaCha chacha2;                                    // Create a ChaCha decryption object

        chacha2.setKey(transmitterPublicKey, sizeof(transmitterPublicKey)); // Set the decryption key
        chacha2.setIV(iv, sizeof(iv));                                      // Set the initialization vector

        chacha2.decrypt(encryptionKey, encryptionKey, sizeof(encryptionKey)); // Decrypt the encryption key

        Serial.print(F("Plain text Encryption key: ")); // Notify of the plain text encryption key
        // Print the decrypted encryption key
        for (int i = 0; i < sizeof(encryptionKey); i++)
        {
          Serial.print(encryptionKey[i]);
        }
        Serial.println();
        break; // Exit the loop after successfully receiving and decrypting the key
      }
    }
  }
}

// Function to send a public key request
void sendPublicKeyRequest()
{
  uint8_t publicKeyBuf[MESSAGE_LENGTH];  // Buffer for the public key message
  uint8_t bufLen = sizeof(publicKeyBuf); // Length of the buffer

  const long req_interval = 3000; // Request every 3 seconds
  long previousMillis = millis(); // Store the last time a request was sent

  // Constantly loop to check and send public key request, and listen for reply
  while (true)
  {
    long currentMillis = millis(); // Get the current time
    // Check if the request interval has elapsed
    if (currentMillis - previousMillis >= req_interval)
    {
      previousMillis = currentMillis; // Update the previousMillis
      uint8_t buf[FIELD_LENGTH];      // Buffer for the request message
      // Safe formatting of the initial message with fields
      snprintf(buf, sizeof(buf), "%4d %4d %4d %4d %4d %4d %4d ", -1, PUBLIC_KEY_REQ, TAGID, RELAYID, TTL, thisRSSI, 0);
      sendMessage(buf, FIELD_LENGTH); // Send the formatted request message
    }
    // Listen for all signals (looking for reply)
    if (rf95.available() && rf95.recv(publicKeyBuf, &bufLen))
    {
      int seq, type, tagID, destID, ttl, rssi, msgLen; // Variables for message fields
      int itemsRead = sscanf(publicKeyBuf, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);
      // Check if the message is a public key reply
      if (type == PUBLIC_KEY_REPLY && destID == TAGID && itemsRead == 7)
      {
        // Get public key from the buffer
        memcpy(transmitterPublicKey, publicKeyBuf + FIELD_LENGTH, sizeof(transmitterPublicKey)); // Copy the public key
        break;                                                                                   // Exit the loop after receiving the public key
      }
    }
  }
}

// Function to receive public key requests
void receivePublicKeyReq()
{
  while (true)
  {
    uint8_t publicKeyBuf[MESSAGE_LENGTH]; // Buffer to store the received public key request message
    uint8_t len = sizeof(publicKeyBuf);   // Length of the buffer
    if (rf95.available() && rf95.recv(publicKeyBuf, &len))
    {                                                  // Check if a message is available and receive it
      int seq, type, tagID, destID, ttl, rssi, msgLen; // Variables for message fields
      // Extract subfields from the received message
      int itemsRead = sscanf(publicKeyBuf, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);
      // Check if the message is valid and is a public key request
      if (itemsRead == 7 && destID == TAGID && type == PUBLIC_KEY_REQ)
      {
        sendPublicKey(); // Send the public key in response
        break;           // Exit the loop after sending the key
      }
    }
  }
}

// Function to send the public key in response to a request
void sendPublicKey()
{
  uint8_t buf[MESSAGE_LENGTH]; // Buffer to prepare the public key message
  // Safely format the initial message with fields
  snprintf(buf, FIELD_LENGTH + 1, "%4d %4d %4d %4d %4d %4d %4d ", -1, PUBLIC_KEY_REPLY, TAGID, RELAYID, TTL, thisRSSI, sizeof(publicKey));

  memcpy(buf + FIELD_LENGTH, publicKey, 32); // Copy the public key into the buffer after the fields
  sendMessage(buf, FIELD_LENGTH + 32);       // Send the complete message
}

// Function to send messages using the RF95 module
void sendMessage(uint8_t *buf, int bufLen)
{
  rf95.setModeIdle(); // Set RF95 module to idle mode to prevent message loss

  // Check if the channel is idle; if not, wait until it goes idle
  while (rf95.isChannelActive())
  {
    delay(CSMATIME);                                         // Wait for the channel to become idle by checking frequently
    Serial.println("Tag node looping on isChannelActive()"); // Debugging output
  }

  rf95.send(buf, bufLen); // Send the message using RF95
  rf95.waitPacketSent();  // Wait for the packet to be sent
}

// Main loop where the program runs continuously
void loop()
{
  uint8_t buf[MESSAGE_LENGTH]; // Buffer for received messages
  uint8_t len = sizeof(buf);   // Length of the buffer
  if (rf95.available())
  { // Check if a message is available
    // If a message is received, process it
    if (rf95.recv(buf, &len))
    {
      receiveMessage(buf); // Handle the received message
    }
    else
    {
      Serial.println("recv failed"); // Log an error if the reception fails
    }
  }
}

// Function to process and display the received message
void receiveMessage(uint8_t *buf)
{
  uint8_t payload[MESSAGE_LENGTH];      // Buffer for the payload
  memcpy(payload, buf, MESSAGE_LENGTH); // Copy received message into payload buffer

  decryptMessage(payload, MESSAGE_LENGTH);         // Decrypt the received message
  int seq, type, tagID, destID, ttl, rssi, msgLen; // Variables for message fields
  // Extract subfields from the decrypted payload
  int itemsRead = sscanf(payload, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);
  // Check if the message was correctly parsed
  if (itemsRead < 7)
  {
    Serial.println("Error"); // Log error if parsing fails
    return;                  // Exit the function
  }

  // Check for message type and destination ID
  if (type != 0 || destID != TAGID)
    return; // If not a valid message, exit

  uint8_t message[msgLen];                         // Buffer for the actual message content
  memcpy(message, payload + FIELD_LENGTH, msgLen); // Extract message content from the payload

  // Debug output to display the received message details
  Serial.println("Message received");
  Serial.println("--------------------------------------------");
  Serial.print("Original message: ");
  for (int i = 0; i < FIELD_LENGTH + msgLen; i++)
  {
    Serial.print((char)buf[i]); // Print the original message as characters
  }
  Serial.println();

  // Print message details
  Serial.print("Seq ");
  Serial.print(seq);
  Serial.print(" Type ");
  Serial.print(type);
  Serial.print(" Source ID ");
  Serial.print(tagID);
  Serial.print(" Destination ID ");
  Serial.print(destID);
  Serial.print(" TTL ");
  Serial.print(ttl);
  Serial.print(" RSSI ");
  Serial.print(rssi);
  Serial.print(" RSSI ");
  Serial.print(rf95.lastRssi()); // Log the last RSSI value from the RF95
  Serial.print(" Message Length ");
  Serial.println(msgLen);

  // Print the actual received message content
  Serial.print("Actual message: ");
  for (int i = 0; i < msgLen; i++)
  {
    Serial.print((char)message[i]); // Print each character of the message
  }
  Serial.println();
}

// Function to decrypt the received message
void decryptMessage(uint8_t *message, int msgLen)
{
  ChaCha chacha;                                       // Create a ChaCha cipher object
  chacha.clear();                                      // Clear any previous state
  chacha.setKey(encryptionKey, sizeof(encryptionKey)); // Set the encryption key
  chacha.setIV(iv, sizeof(iv));                        // Set the initialization vector

  chacha.decrypt(message, message, msgLen); // Decrypt the message in place
}