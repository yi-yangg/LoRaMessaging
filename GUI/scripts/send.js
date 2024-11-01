"use strict";

// Define null for transmitter port
var transmitterPort = null;

// Define reader in global scope so can be used in both serialRead and disconnectPort functions
var reader;

const updateTransmitterPort = () => updatePortButtons("Transmitter");
const deleteTransmitterPort = () => removePortButtons("Transmitter");

// On load function
window.onload = () => {
    const transmitterButton = document.getElementById("select-button");
    
    // Initialise connect button
    transmitterButton.onclick = async () => {
        const port = await connectPort();
        if (port) {
            transmitterPort = port;
            showContainer();
            updateTransmitterPort();
            await new Promise(resolve => setTimeout(resolve, 2000)); // Wait for setup on Arduino
            serialRead(); // Start reading

            // Send password
            document.getElementById("message-input").value = document.getElementById("password-info").value;
            serialWrite();
        }
        else if (transmitterPort) {
            disconnectPort();
        }
    }
    
    // Initialise other button functions
    const disconnectButton = document.getElementById("disconnect-button");
    const sendButton = document.getElementById("send-button");
    const clearButton = document.getElementById("clear-button");
    disconnectButton.onclick = disconnectPort;
    sendButton.onclick = serialWrite;
    clearButton.onclick = clearMessageFromTextArea;
}

// Disconnect port
async function disconnectPort() {
    // Stop reading
    await reader.cancel();
    await transmitterPort.close();
    transmitterPort = null;
    clearMessageFromTextArea();
    deleteTransmitterPort();
}

// Write messages to Arduino
async function serialWrite() {
    const message = document.getElementById("message-input").value.trim() + "\n"; // Format input messages so can be accepted by Arduino
    if (transmitterPort) {
        //console.log("test");
        const writer = transmitterPort.writable.getWriter();
        const encoder = new TextEncoder();
        await writer.write(encoder.encode(message));
        writer.releaseLock();
        document.getElementById("message-input").value = ""; // Clear input area after writing
    }
    else {
        alert('Transmitter port not set');
    }
}

// Read messages from Arduino
async function serialRead() {
    while (transmitterPort.readable) {
        reader = transmitterPort.readable.getReader();
        try {
            const decoder = new TextDecoder();
            while (true) {
                await new Promise(resolve => setTimeout(resolve, 1000));
                // Read from the serial port
                const { value, done } = await reader.read();
                if (done) {
                    // Stream has been closed
                    break;
                }
                // Decode and add message to serial monitor textarea
                const text = decoder.decode(value);
                if (text) {
                    //console.log(text);
                    addMessageToTextArea(text);
                }
            }
        } catch (error) {
            console.error("Error reading from serial port:", error);
        } finally {
            reader.releaseLock();
        }
    }
}

// Append messages read from Arduino to serial monitor textarea
function addMessageToTextArea(text) {
    var serialMonitor = document.getElementById("serial-output");
    serialMonitor.value += text;
    serialMonitor.scrollTop = serialMonitor.scrollHeight;
} 

// Clear messages printed on serial monitor textarea
function clearMessageFromTextArea() {
    var serialMonitor = document.getElementById("serial-output");
    serialMonitor.value = "";
    serialMonitor.scrollTop = serialMonitor.scrollHeight; // Scroll down to the current place where messages are output
}  