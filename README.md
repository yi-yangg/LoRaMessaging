# LoRa Messaging System

## Project Overview

This project is a **LoRa-based Messaging System** that enables secure, long-range communication using LoRa technology integrated with Arduino and a graphical user interface (GUI). It’s designed for reliable, secure communication in challenging environments, such as underground mining.

### Key Features

- **Secure Communication**: Leverages the Crypto.h library to secure communication between devices.
- **Long-Range Messaging**: Utilizes LoRa technology to enable long range communication with low power consumption.
- **User-friendly GUI**: A GUI built using PHP and Bootstrap to manage system settings and allow users to interact with the LoRa devices through a web-based GUI
- **Password Management**: Allows users to update their passwords securely through GUI with SQL database.

## Hardware Requirements

- **Arduino Uno**: Primary microcontroller for handling LoRa communication.
- **XC4392 LoRa™ Shield**: Enables long-range communication between devices.
- **Computer/Laptop**: Used to run the GUI and connect to the Arduino devices.

## Software Requirements

- **Arduino IDE**: For uploading code to the Arduino Uno.
- **Web Server**: Apache or any server capable of running PHP scripts and MySQL for database management.

## Installation and Setup

Follow these steps to set up the project on your computer:

### Prerequisites

1. **Arduino IDE**: Download and install the Arduino IDE from [Arduino's official website](https://www.arduino.cc/en/software).
  
2. **PHP & Web Server** (if running locally): Install a local server environment such as [XAMPP](https://www.apachefriends.org/download.html).

3. **Libraries**:
   - Install the **Crypto** library for Arduino encryption.
   - Install the **RadioHead** library for handling LoRa communication.
   
   In the Arduino IDE:
   - Go to **Sketch > Include Library > Manage Libraries...**
   - Search for `Crypto` and `RadioHead`, then click **Install** for each.

### Setting up the Project

1. **Clone the Repository**: Clone this repository to your local machine or download it as a ZIP file and extract it.

2. **Setting Up the Arduino**:
   - Open each Arduino code file from the repo in the Arduino IDE (Tag.ino, Relay.ino, Receiver.ino).
   - Connect your Arduino board via USB.
   - Select the correct board and port from Tools > Board/Port.
   - Upload the code to the Arduino board.
  
3. **Setting up the web GUI**:
   - Place the PHP files from the `GUI` directory into the `htdocs` folder of your server environment (e.g. `XAMPP/htdocs/GUI`)
   - Start Apache server through XAMPP or other server environment.
   - Open browser and navigate to `http://localhost/GUI` to access the GUI.
    
### Database Setup (if applicable)

1. **Create Database**:
   - Open phpMyAdmin (typically available at `http://localhost/phpmyadmin`)
   - Create a new database (e.g. `lora_project` or `lora_db`)

2. **Edit Configurations**:
   - In `settings.php`, set your database host, username, password and newly created database name.
   - Ensure all settings match your local SQL environment.

### Usage

1. **Access the GUI**:
   - Once navigated to `http://localhost/GUI`, register a new account and login with the created credentials.

2. **Send Secure Messages**:
   - After connecting and uploading the code files to the Arduino boards, select the respective port on the `Send` and `Receive` pages.

3. **Monitor Responses**:
   - View incoming messagess from the Arduino in the GUI's message panel.

## Security Measures:

### Password Management

- **Passowrd Hashing**: SHA-256 hashing is used to store user passwords securely.
- **Session Management**: ENsures only authorized users can access the GUI to send and receive messages from the Arduino board.

### Secure Communication Protocol

1. **Key Derivation Function (KDF)**: The system uses a simple KDF to generate a unique encryption key based on the logged-in user’s password. This key is derived using XOR and shifting operations, and shuffled a number of times to ensure its strength and randomness.

2. **ChaCha Encryption**: The ChaCha20 stream cipher is implemented to encrypt the entire message payload. ChaCha20 is selected for its efficiency on low-power devices and strong security guarantees.

3. **Curve25519 for Secure Key Exchange**: Curve25519 is used for secure, efficient key exchange between devices. This elliptic curve algorithm enables both devices to generate a shared secret securely, which is used to transmit the derived encryption key. This ensures that only authorized devices can decrypt the messages, even if an attacker intercepts the communication.

## Relay node implementation

The relay node in this project serves as an intermediary for secure message transmission between the transmitter and receiver, performing key management, packet handling, and relay functions as follows:
1. **Key Exchange Process**:
   - The relay node begins by performing a secure key exchange with the transmitter node using Curve25519 to obtain the derived encryption key based on the logged-in user's password.
   - This obtained encryption key enables the relay node to securely decrypt incoming packets from the transmitter.

2. **Packet Handling**:
   - The relay node receives encrypted packets from the transmitter. For each packet, it checks the destination ID:
     - **If the packet is intended for the relay node itself** (e.g. `destination_id == self_id`), it will capture and decrypt the packet, processing the message as the final destination.
     - **If the packet is not intended for the relay node** (e.g. `destination_id != self_id`), it decrypts the packet, modifies the source ID to its own ID and re-encrypts it. The modified packet is then re-broadcasted towards the intended destination.

## Future Improvements

- **Better Key Derivation Function**: Implement a stronger KDF by adding a salt onto the function to allow a more random encryption key for the same password.
- **Allow for multi-way communication**: The current network architecture only allows for 1 way communication (`Transmitter->Relay->Receiver`). Expanding to multi-way communication would allow for bidirectional or even full mesh communication (`Transmitter <-> Relay <-> Receiver`). This would make the architecture more scalable when adding more `Transmitter, Relay, or Receiver` nodes.

   
