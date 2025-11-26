// WiFi Management Functions
document.addEventListener('DOMContentLoaded', function() {
    loadWiFiStatus();
    setInterval(loadWiFiStatus, 10000);
});

async function loadWiFiStatus() {
    try {
        const response = await fetch('/api/wifi/status');
        const data = await response.json();

        document.getElementById('wifi-connected').textContent =
            data.connected ? 'Conectado' : 'Desconectado';
        document.getElementById('wifi-ssid').textContent = data.ssid || '-';
        document.getElementById('wifi-ip').textContent = data.ip || '-';
        document.getElementById('wifi-rssi').textContent =
            data.rssi ? `${data.rssi} dBm` : '-';
    } catch (error) {
        console.error('Error loading WiFi status:', error);
    }
}

async function scanWiFi() {
    const networksList = document.getElementById('networks-list');
    networksList.innerHTML = '<p>🔍 Escaneando redes...</p>';

    let retries = 0;
    const maxRetries = 5;

    while (retries < maxRetries) {
        try {
            const response = await fetch('/api/wifi/scan');
            const data = await response.json();

            // If scan is in progress, wait and retry
            if (response.status === 202 || data.status === 'scanning') {
                retries++;
                networksList.innerHTML = `<p>🔍 Escaneando redes... (${retries}/${maxRetries})</p>`;
                await new Promise(resolve => setTimeout(resolve, 2000));
                continue;
            }

            if (data.networks && data.networks.length > 0) {
                // Sort by signal strength
                data.networks.sort((a, b) => b.rssi - a.rssi);

                networksList.innerHTML = data.networks.map(network => {
                    const signalClass = getSignalClass(network.rssi);
                    const signalIcon = getSignalIcon(network.rssi);
                    const escapedSsid = network.ssid.replace(/'/g, "\\'").replace(/"/g, '\\"');
                    return `
                        <div class="network-card" onclick="selectNetwork('${escapedSsid}')">
                            <div class="network-header">
                                <span class="network-name">
                                    <span class="network-icon">📶</span>
                                    ${network.ssid}
                                </span>
                                <span class="network-signal ${signalClass}">
                                    ${signalIcon} ${network.rssi} dBm
                                </span>
                            </div>
                            <div class="network-details">
                                <span>Canal: ${network.channel}</span>
                                <span>${network.encryption > 0 ? '🔒 Protegida' : '🔓 Aberta'}</span>
                            </div>
                        </div>
                    `;
                }).join('');
            } else {
                networksList.innerHTML = '<p>Nenhuma rede encontrada. Tente novamente.</p>';
            }
            return; // Success, exit loop
        } catch (error) {
            console.error('Error scanning WiFi:', error);
            retries++;
            if (retries >= maxRetries) {
                networksList.innerHTML = '<p>❌ Erro ao escanear redes. Tente novamente.</p>';
            }
            await new Promise(resolve => setTimeout(resolve, 1000));
        }
    }
}

function getSignalClass(rssi) {
    if (rssi >= -50) return 'excellent';
    if (rssi >= -60) return 'good';
    if (rssi >= -70) return 'fair';
    return 'weak';
}

function getSignalIcon(rssi) {
    if (rssi >= -50) return '📶';
    if (rssi >= -60) return '📶';
    if (rssi >= -70) return '📶';
    return '📶';
}

function selectNetwork(ssid) {
    document.getElementById('ssid').value = ssid;
    document.getElementById('password').focus();
}

async function connectWiFi() {
    const ssid = document.getElementById('ssid').value;
    const password = document.getElementById('password').value;

    if (!ssid) {
        alert('Por favor, digite o nome da rede');
        return;
    }

    if (!confirm(`Conectar a rede "${ssid}"? O dispositivo sera reiniciado.`)) {
        return;
    }

    try {
        const response = await fetch('/api/wifi/connect', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ ssid, password })
        });

        const data = await response.json();
        if (data.status === 'ok') {
            alert('Configuracao salva! O dispositivo sera reiniciado.');
        } else {
            alert('Erro: ' + (data.error || 'Falha ao conectar'));
        }
    } catch (error) {
        console.error('Error connecting:', error);
        alert('Erro ao conectar');
    }
}

function togglePassword(inputId) {
    const input = document.getElementById(inputId);
    input.type = input.type === 'password' ? 'text' : 'password';
}
