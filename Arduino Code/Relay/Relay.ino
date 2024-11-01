// AUTHOR: Chong Yi Yang
// STUDENT ID: 105297292
// COURSE: COS60011-Technology Design Project
/////
// Relay Code Overview:
// This code is designed to function as a relay node in a wireless communication network,
// utilizing the RF95 module to listen for messages. If a message is heard, it tries to
// decrypt and checks if the destination ID is the same as its own ID. If it is then it
// will capture the packet, if not it will re-broadcast the message and changing the source
// ID header field.
//
// Message Header Format:
//  SEQ       %4d   // Sequence number of the message
//  TYPE      %4d   // Message type (indicates the kind of message)
//  TAGID     %4d   // Identifier for the source node where the message is coming from
//  DESTID    %4d   // Identifier for the destination node where the message is going to
//  TTL       %4d   // Time-to-live value for the message, indicating its lifespan
//  RSSI      %4d   // Received Signal Strength Indicator, used for assessing signal quality
//  MSGLEN    %4d   // Length of the message sent
/////

#include <SPI.h>
#include <RH_RF95.h>

// Cryptographic libraries
#include <ChaCha.h>
#include <Curve25519.h>
#include <RNG.h>

// Singleton instance of radio driver
RH_RF95 rf95; // Create an instance of the RF95 radio driver

// Constants
int MESSAGE_LENGTH = 108;   // Maximum input message length (buffer size)
int FIELD_LENGTH = 35;      // Length of the fixed fields in the message
const double CSMATIME = 10; // Time to wait for channel to become idle

const int TAGID = 101;      // Unique identifier for the relay node
int replyID;                // ID of the responding node (changeable at runtime)
const int RECEIVERID = 100; // ID of the receiver node
int TTL = 5;                // Time to live for the messages
int thisRSSI = 0;           // RSSI value for signal strength

// Constant message types
const int PUBLIC_KEY_REQ = 50;   // Message type for public key request
const int PUBLIC_KEY_REPLY = 51; // Message type for public key reply
const int ENC_KEY = 52;          // Message type for encrypted key

// Initialize private and public keys
uint8_t privateKey[32] = {0};           // Array to store private key
uint8_t publicKey[32] = {0};            // Array to store public key
uint8_t transmitterPublicKey[32] = {0}; // Public key of the transmitter
uint8_t receiverPublicKey[32] = {0};    // Public key of the receiver
uint8_t encryptionKey[32] = {0};        // Array to store encryption key
uint8_t iv[12] = {0};                   // Initialization vector for encryption

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
  Serial.begin(9600);                       // Start serial communication at 9600 baud rate
  Serial.println(F("Starting Relay Node")); // Log startup message
  while (!Serial)
    ; // Wait for serial port to be available

  // Initialize the RF95 radio module
  while (!rf95.init())
  {
    // Log initialization failure
    delay(1000);
  }

  // Start random number generator for cryptographic functions
  RNG.begin("LoRa based messaging system (Relay node)");

  rf95.setFrequency(915.0);        // Set frequency to 915 MHz for LoRa
  rf95.setTxPower(23, false);      // Set transmission power
  rf95.setSignalBandwidth(500000); // Set signal bandwidth
  rf95.setSpreadingFactor(12);     // Set spreading factor for LoRa

  // Generate cryptographic keys
  generateKeys();

  // Begin key exchange with transmitter
  Serial.println("Initiating key exchange with transmitter");
  Serial.println("--------------------------------------------");

  // Receive encryption key from transmitter
  receivePublicKeyReq();
  sendPublicKeyRequest(false); // Send public key request to transmitter

  // Log transmitter's public key
  Serial.print(F("Transmitter Public Key: "));
  for (int i = 0; i < sizeof(publicKey); i++)
  {
    Serial.print(transmitterPublicKey[i]);
  }
  Serial.println();

  // Create a copy of the private key before passing it to dh2() function
  uint8_t privateKeyCopy[32];
  memcpy(privateKeyCopy, privateKey, 32); // Copy private key

  // Generate shared secret using transmitter's public key
  Curve25519::dh2(transmitterPublicKey, privateKey);
  Serial.print(F("Shared key: "));
  for (int i = 0; i < sizeof(transmitterPublicKey); i++)
  {
    Serial.print(transmitterPublicKey[i]);
  }
  Serial.println();

  // Start receiving the encryption key
  Serial.println(F("Receiving encryption key"));
  Serial.println("--------------------------------------------");
  receiveEncryptionKey();

  // Send encryption key to receiver
  bool hasReceiver = false; // Change this as per receiver's availability

  // If there is a receiver, proceed with key exchange
  if (hasReceiver)
  {
    Serial.println(F("Initiating key exchange with Receiver"));
    Serial.println(F("--------------------------------------------"));

    replyID = RECEIVERID;       // Set reply ID to receiver ID
    sendPublicKeyRequest(true); // Request public key from the receiver

    // Log receiver's public key
    Serial.print(F("Receiver Public Key: "));
    for (int i = 0; i < sizeof(publicKey); i++)
    {
      Serial.print(receiverPublicKey[i]);
    }
    Serial.println();

    // Receive public key request from receiver
    receivePublicKeyReq();

    // Generate shared secret using receiver's public key
    Curve25519::dh2(receiverPublicKey, privateKeyCopy);
    Serial.print(F("Shared secret key: "));
    for (int i = 0; i < sizeof(receiverPublicKey); i++)
    {
      Serial.print(receiverPublicKey[i]);
    }
    Serial.println();

    // Encrypt and send the encryption key to the receiver
    Serial.println(F("Sending encryption key"));
    Serial.println("--------------------------------------------");
    sendEncryptionKey();
  }

  Serial.println(F("System is ready")); // Log system readiness
}

// Function to send the encryption key to the receiver
void sendEncryptionKey()
{
  ChaCha chacha2;              // Create ChaCha instance
  uint8_t buf[MESSAGE_LENGTH]; // Buffer for message

  // Set key and IV for ChaCha encryption
  chacha2.setKey(receiverPublicKey, sizeof(receiverPublicKey));
  chacha2.setIV(iv, sizeof(iv));

  uint8_t encryptedEncKey[32] = {0}; // Buffer for encrypted encryption key
  // Encrypt the encryption key
  chacha2.encrypt(encryptedEncKey, encryptionKey, sizeof(encryptionKey));

  // Log encrypted encryption key
  Serial.print("Encrypted encryption key: ");
  for (int i = 0; i < sizeof(encryptedEncKey); i++)
  {
    Serial.print(encryptedEncKey[i]);
  }
  Serial.println();

  // Format the message with the relevant fields
  snprintf(buf, sizeof(buf), "%4d %4d %4d %4d %4d %4d %4d ", -1, ENC_KEY, TAGID, replyID, TTL, thisRSSI, sizeof(encryptedEncKey));
  memcpy(buf + FIELD_LENGTH, encryptedEncKey, 32); // Append encrypted key to the message

  // Send the formatted message
  sendMessage(buf, MESSAGE_LENGTH);
}

// Function to receive the encryption key from the transmitter
void receiveEncryptionKey()
{
  while (true)
  {                                    // Loop until a valid key is received
    uint8_t encKeyBuf[MESSAGE_LENGTH]; // Buffer for received message
    uint8_t len = sizeof(encKeyBuf);   // Buffer length
    if (rf95.available() && rf95.recv(encKeyBuf, &len))
    { // Check for received message
      int seq, type, tagID, destID, ttl, rssi, msgLen;
      // Extract subfields and look for ENC_KEY
      int itemsRead = sscanf(encKeyBuf, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);

      // Validate received message
      if (itemsRead == 7 && destID == TAGID && type == ENC_KEY)
      {
        memcpy(encryptionKey, encKeyBuf + FIELD_LENGTH, sizeof(encryptionKey)); // Copy the encryption key

        // Log the encrypted encryption key
        Serial.print(F("Encrypted encryption key: "));
        for (int i = 0; i < sizeof(encryptionKey); i++)
        {
          Serial.print(encryptionKey[i]);
        }
        Serial.println();

        // Decrypt the encryption key
        ChaCha chacha2;
        chacha2.setKey(transmitterPublicKey, sizeof(transmitterPublicKey));
        chacha2.setIV(iv, sizeof(iv));
        chacha2.decrypt(encryptionKey, encryptionKey, sizeof(encryptionKey)); // Decrypt the key

        // Log the decrypted encryption key
        Serial.print(F("Encryption key: "));
        for (int i = 0; i < sizeof(encryptionKey); i++)
        {
          Serial.print(encryptionKey[i]);
        }
        Serial.println();
        break; // Exit the loop after successful reception
      }
    }
  }
}

void sendPublicKeyRequest(bool isFromReceiver)
{
  uint8_t publicKeyBuf[MESSAGE_LENGTH];  // Buffer to store received public key messages
  uint8_t bufLen = sizeof(publicKeyBuf); // Length of the buffer

  const long req_interval = 3000; // Request interval set to 3 seconds
  long previousMillis = millis(); // Store the last time a request was sent

  // Loop continuously to send a public key request and listen for replies
  while (true)
  {
    long currentMillis = millis(); // Get the current time
    // Check if the interval for sending a request has elapsed
    if (currentMillis - previousMillis >= req_interval)
    {
      previousMillis = currentMillis; // Update the last request time
      uint8_t buf[FIELD_LENGTH];      // Buffer to format the request message
      // Format the request message with fields
      snprintf(buf, sizeof(buf), "%4d %4d %4d %4d %4d %4d %4d ", -1, PUBLIC_KEY_REQ, TAGID, replyID, TTL, thisRSSI, 0);
      sendMessage(buf, FIELD_LENGTH); // Send the formatted request message
    }
    // Listen for incoming signals, checking for a reply
    if (rf95.available() && rf95.recv(publicKeyBuf, &bufLen))
    {
      int seq, type, tagID, destID, ttl, rssi, msgLen; // Variables to store extracted fields
      // Parse the received message
      int itemsRead = sscanf(publicKeyBuf, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);
      // Check if the message is a valid public key reply with matching identifiers
      if (type == PUBLIC_KEY_REPLY && destID == TAGID && tagID == replyID && itemsRead == 7)
      {
        // Get the public key from the received buffer
        if (isFromReceiver)
          memcpy(receiverPublicKey, publicKeyBuf + FIELD_LENGTH, sizeof(transmitterPublicKey)); // Copy to receiver's key
        else
          memcpy(transmitterPublicKey, publicKeyBuf + FIELD_LENGTH, sizeof(transmitterPublicKey)); // Copy to transmitter's key

        break; // Exit the loop once the key is received
      }
    }
  }
}

void receivePublicKeyReq()
{
  while (true)
  {
    uint8_t publicKeyBuf[MESSAGE_LENGTH]; // Buffer to store incoming public key requests
    uint8_t len = sizeof(publicKeyBuf);   // Length of the buffer
    // Check if there is an incoming message
    if (rf95.available() && rf95.recv(publicKeyBuf, &len))
    {
      int seq, type, tagID, destID, ttl, rssi, msgLen; // Variables to store extracted fields
      // Parse the received message for public key requests
      int itemsRead = sscanf(publicKeyBuf, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);

      // Check if the message is a valid public key request for this node
      if (itemsRead == 7 && destID == TAGID && type == PUBLIC_KEY_REQ)
      {
        replyID = tagID; // Set the reply ID to the sender's tag ID
        sendPublicKey(); // Send the public key in response
        break;           // Exit the loop after sending the key
      }
    }
  }
}

void sendPublicKey()
{
  uint8_t buf[MESSAGE_LENGTH]; // Buffer to format the public key message
  // Format the message with fields
  snprintf(buf, FIELD_LENGTH + 1, "%4d %4d %4d %4d %4d %4d %4d ", -1, PUBLIC_KEY_REPLY, TAGID, replyID, TTL, thisRSSI, sizeof(publicKey));
  memcpy(buf + FIELD_LENGTH, publicKey, 32); // Append the public key to the message
  sendMessage(buf, FIELD_LENGTH + 32);       // Send the message with the public key
}

void sendMessage(uint8_t *buf, int bufLen)
{
  rf95.setModeIdle(); // Set the RF95 module to idle mode to avoid message loss

  // Wait for the channel to be idle before sending
  while (rf95.isChannelActive())
  {
    delay(CSMATIME);                                         // Wait for a brief moment to check if the channel is idle
    Serial.println("Tag node looping on isChannelActive()"); // Debug message
  }
  rf95.send(buf, bufLen); // Send the buffer
  rf95.waitPacketSent();  // Wait until the packet has been sent
}

void loop()
{
  uint8_t buf[MESSAGE_LENGTH]; // Buffer for receiving messages
  uint8_t len = sizeof(buf);   // Length of the buffer
  // Check if there are available messages
  if (rf95.available())
  {
    // Attempt to receive a message
    if (rf95.recv(buf, &len))
    {
      receiveMessage(buf); // Process the received message
    }
    else
    {
      Serial.println("recv failed"); // Debug message for failed reception
    }
  }
}

void encryptMessage(uint8_t *message, int msgLen)
{
  ChaCha chacha;                                       // Create a ChaCha encryption object
  chacha.clear();                                      // Clear any previous settings
  chacha.setKey(encryptionKey, sizeof(encryptionKey)); // Set the encryption key
  chacha.setIV(iv, sizeof(iv));                        // Set the initialization vector
  chacha.encrypt(message, message, msgLen);            // Encrypt the message in-place
}

void rebroadcastMessage(uint8_t *decryptedBuf, uint8_t msgLen)
{
  Serial.println("Message not intended for this node"); // Debug message
  Serial.println("Replacing source ID...");             // Indicate ID replacement process

  // Position where the TAGID is located in the buffer
  int tagIDPositionInBuf = 10;
  char tagIDStr[5];                    // String buffer to hold the TAGID
  snprintf(tagIDStr, 5, "%4d", TAGID); // Convert TAGID int to string

  // Replace the old source ID in the buffer with the new TAGID
  memcpy(decryptedBuf + tagIDPositionInBuf, tagIDStr, 4);

  Serial.println(F("Sending message")); // Debug message before sending
  Serial.println("--------------------------------------------");
  Serial.print(F("Original Message: ")); // Print the original message for debugging
  for (int i = 0; i < FIELD_LENGTH + msgLen; i++)
  {
    Serial.print((char)decryptedBuf[i]); // Output each character of the original message
  }
  Serial.println();

  Serial.println(F("Encrypting message..."));          // Indicate the start of the encryption process
  encryptMessage(decryptedBuf, FIELD_LENGTH + msgLen); // Encrypt the modified buffer
  Serial.print("Encrypted message: ");                 // Print the encrypted message for debugging
  for (int i = 0; i < FIELD_LENGTH + msgLen; i++)
  {
    Serial.print((char)decryptedBuf[i]); // Output each character of the encrypted message
  }
  Serial.println();

  sendMessage(decryptedBuf, FIELD_LENGTH + msgLen); // Send the rebroadcasted message
}

void receiveMessage(uint8_t *buf)
{
  uint8_t payload[MESSAGE_LENGTH];      // Buffer to unpack the received message
  memcpy(payload, buf, MESSAGE_LENGTH); // Copy the received buffer to payload

  decryptMessage(payload, MESSAGE_LENGTH);         // Decrypt the received message
  int seq, type, tagID, destID, ttl, rssi, msgLen; // Variables for extracted fields
  // Parse the decrypted message
  int itemsRead = sscanf(payload, "%4d %4d %4d %4d %4d %4d %4d ", &seq, &type, &tagID, &destID, &ttl, &rssi, &msgLen);
  // Check if the message format is valid
  if (itemsRead < 7)
  {
    Serial.println("Message format invalid."); // Debug message for invalid format
    return;                                    // Exit if format is invalid
  }

  // Check if the message type is valid
  if (type != 0)
  {
    Serial.println("Unknown message type"); // Debug message for unknown type
    return;                                 // Exit for unknown message type
  }

  uint8_t message[msgLen];                         // Buffer for the actual message
  memcpy(message, payload + FIELD_LENGTH, msgLen); // Extract the message from the payload

  // Display the received message for debugging
  Serial.print("Original message : ");
  for (int i = 0; i < FIELD_LENGTH + msgLen; i++)
  {
    Serial.print((char)buf[i]); // Print the entire original message
  }
  Serial.println();

  // Print the message fields for debugging
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
  Serial.print(rf95.lastRssi());
  Serial.print(" Message Length ");
  Serial.print(msgLen);
  Serial.print(" Actual Message: ");
  for (int i = 0; i < msgLen; i++)
  {
    Serial.print((char)message[i]); // Print the actual message
  }
  Serial.println();

  // If message is not for itself (destination ID not the same as its ID), then re-broadcast
  if (destID != TAGID)
  {
    rebroadcastMessage(payload, msgLen);
    return;
  }
}

void decryptMessage(uint8_t *message, int msgLen)
{
  ChaCha chacha;                                       // Create a ChaCha decryption object
  chacha.clear();                                      // Clear any previous settings
  chacha.setKey(encryptionKey, sizeof(encryptionKey)); // Set the decryption key
  chacha.setIV(iv, sizeof(iv));                        // Set the initialization vector
  chacha.decrypt(message, message, msgLen);            // Decrypt the message in-place
}