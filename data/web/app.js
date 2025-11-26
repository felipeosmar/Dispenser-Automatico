/**
 * ESP32 Dispenser - Web Interface JavaScript
 */

let isConnected = false;
let healthData = null;

document.addEventListener('DOMContentLoaded', function () {
    console.log('ESP32 Dispenser Interface Loaded');
    checkConnection();
    setInterval(checkConnection, 2000);
    updateSystemInfo();
    setInterval(updateSystemInfo, 5000);
});

async function checkConnection() {
    try {
        const response = await fetch('/api/status', {
            method: 'GET',
            cache: 'no-cache'
        });
        if (response.ok) {
            healthData = await response.json();
            if (!isConnected) {
                isConnected = true;
                updateConnectionStatus(true);
            }
        } else {
            if (isConnected) {
                isConnected = false;
                updateConnectionStatus(false);
            }
        }
    } catch (error) {
        if (isConnected) {
            isConnected = false;
            updateConnectionStatus(false);
        }
    }
}

function updateConnectionStatus(connected) {
    const statusDot = document.getElementById('connection-status');
    const statusText = document.getElementById('status-text');

    if (connected) {
        if (statusDot) statusDot.classList.add('connected');
        if (statusText) statusText.textContent = 'Conectado';
    } else {
        if (statusDot) statusDot.classList.remove('connected');
        if (statusText) statusText.textContent = 'Desconectado';
    }
}

async function updateSystemInfo() {
    if (!healthData) return;

    const uptimeElement = document.getElementById('uptime');
    if (uptimeElement && healthData.uptime) {
        uptimeElement.textContent = healthData.uptime.formatted || 'N/A';
    }

    const wifiElement = document.getElementById('wifi-status');
    if (wifiElement && healthData.wifi) {
        const wifiStatus = healthData.wifi.connected ?
            `${healthData.wifi.ssid} (${healthData.wifi.signal_strength})` :
            'Desconectado';
        wifiElement.textContent = wifiStatus;
    }
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

console.log('ESP32 Dispenser Ready');
