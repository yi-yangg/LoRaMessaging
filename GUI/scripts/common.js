"use strict";


// Connect port on click function
async function connectPort() {
    // Get span element from ID
    const portInfoSpan = document.getElementById("port-info");
    try {
        // Request user to select port
        const port = await navigator.serial.requestPort();
        await port.open({ baudRate: 9600 });
        
        // Change span text and text color
        portInfoSpan.textContent = "Connected";
        portInfoSpan.classList.remove("text-danger");
        portInfoSpan.classList.add("text-success");

        // Set port variable to which device port it is
        return port;
    }

    catch(error) {
        // If no port selected or duplicate port selected, error
        console.error(error);
        return removePort("No port selected or port is unavailable");

    }
}

// Change port status after disconnecting
function removePort(content) {
    const portInfoSpan = document.getElementById("port-info");
    portInfoSpan.textContent = "No port selected";
    portInfoSpan.classList.remove("text-success");
    portInfoSpan.classList.add("text-danger");
    showToast(content);

    return null;
}

// Show notifications
function showToast(content) {
    const toast = document.getElementById("liveToast");
    const toastText = document.getElementById("toast-text");

    toastText.textContent = content;

    const toastBootstrap = bootstrap.Toast.getOrCreateInstance(toast);
    toastBootstrap.show();
}

// Show function panel after connecting to a port
function showContainer() {
    const monitor = document.getElementById("monitor-container");
    monitor.classList.remove("d-none");
}

// Hide function panel after connecting to a port
function removeContainer() {
    const monitor = document.getElementById("monitor-container");
    monitor.classList.add("d-none");
}

// Remove connect button and show disconnect button
function updatePortButtons(port) {
    const selectButton = document.getElementById("select-button");
    const disconnectButton = document.getElementById("disconnect-button");

    // Update select button to update port button
    /*
    selectButton.classList.add("btn-success");
    selectButton.classList.remove("btn-primary");
    selectButton.textContent = "Update " + port + " Port";
    */
   
    selectButton.classList.add("d-none");
    disconnectButton.classList.remove("d-none");
}

// Remove disconnect button and show connect button
function removePortButtons(port) {
    removePort("Port disconnected");
    const selectButton = document.getElementById("select-button");
    const disconnectButton = document.getElementById("disconnect-button");

    // Update port buttons to be back to normal
    /*
    selectButton.classList.remove("btn-success");
    selectButton.classList.add("btn-primary");
    selectButton.textContent = "Select " + port + " Port";
    */

    selectButton.classList.remove("d-none");
    disconnectButton.classList.add("d-none");

    removeContainer();
}