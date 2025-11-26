// Status Page Functions
document.addEventListener('DOMContentLoaded', function() {
    loadStatus();
    setInterval(loadStatus, 5000);
});

async function loadStatus() {
    try {
        const response = await fetch('/api/status');
        const data = await response.json();

        // System health
        const healthCard = document.getElementById('system-health');
        const healthStatus = document.getElementById('health-status');
        if (data.status === 'healthy') {
            healthCard.className = 'status-card healthy';
            healthStatus.textContent = 'Saudavel';
        } else {
            healthCard.className = 'status-card degraded';
            healthStatus.textContent = 'Degradado';
        }

        // Uptime
        document.getElementById('uptime').textContent = data.uptime?.formatted || '-';

        // Memory
        document.getElementById('heap-total').textContent = formatBytes(data.memory?.heap?.total || 0);
        document.getElementById('heap-free').textContent = formatBytes(data.memory?.heap?.free || 0);
        document.getElementById('heap-used').textContent = formatBytes(data.memory?.heap?.used || 0);
        document.getElementById('heap-usage').textContent = (data.memory?.heap?.usage_percent || 0).toFixed(1) + '%';

        // WiFi
        document.getElementById('wifi-status').textContent = data.wifi?.connected ? 'Conectado' : 'Desconectado';
        document.getElementById('wifi-ssid').textContent = data.wifi?.ssid || '-';
        document.getElementById('wifi-ip').textContent = data.wifi?.ip || '-';
        document.getElementById('wifi-signal').textContent = data.wifi?.signal_strength || '-';

        // SPIFFS
        document.getElementById('spiffs-total').textContent = formatBytes(data.spiffs?.total_bytes || 0);
        document.getElementById('spiffs-used').textContent = formatBytes(data.spiffs?.used_bytes || 0);
        document.getElementById('spiffs-free').textContent = formatBytes(data.spiffs?.free_bytes || 0);
        document.getElementById('spiffs-usage').textContent = (data.spiffs?.usage_percent || 0).toFixed(1) + '%';

        // CPU
        document.getElementById('cpu-model').textContent = data.cpu?.chip_model || '-';
        document.getElementById('cpu-freq').textContent = (data.cpu?.frequency_mhz || '-') + ' MHz';

    } catch (error) {
        console.error('Error loading status:', error);
    }
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}
