"use strict";

// Define null for receiver port
var receiverPort = null;

// Define reader in global scope so can be used in both serialRead and disconnectPort functions
var reader;

const updateReceiverPort = () => updatePortButtons("Receiver");
const deleteReceiverPort = () => removePortButtons("Receiver");

window.onload = () => {
    const receiverButton = document.getElementById("select-button");

    // Initialise connect button
    receiverButton.onclick = async () => {
        const port = await connectPort();
        if (port) {
            receiverPort = port;
            showContainer();
            updateReceiverPort();
            await new Promise(resolve => setTimeout(resolve, 2000)); // Wait for setup on Arduino
            serialRead(); // Start reading
        }
        else if (receiverPort) {
            disconnectPort();
        }
    }
    
    // Initialise other button functions
    const disconnectButton = document.getElementById("disconnect-button");
    const clearButton = document.getElementById("clear-button");
    disconnectButton.onclick = disconnectPort;
    clearButton.onclick = clearMessageFromTextArea;
}

// Disconnect port
async function disconnectPort() {
    // Stop reading
    await reader.cancel();
    await receiverPort.close();
    receiverPort = null;
    clearMessageFromTextArea();
    deleteReceiverPort();
}

// Read messages from Arduino
async function serialRead() {
    while (receiverPort.readable) {
        reader = receiverPort.readable.getReader();
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