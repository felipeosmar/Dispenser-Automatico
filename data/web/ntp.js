// NTP Configuration Functions
document.addEventListener('DOMContentLoaded', function() {
    loadNTPConfig();
    loadNTPTime();
    setInterval(loadNTPTime, 1000);

    document.getElementById('ntp-enabled').addEventListener('change', function() {
        document.getElementById('ntp-enabled-label').textContent =
            this.checked ? 'Habilitado' : 'Desabilitado';
    });
});

async function loadNTPConfig() {
    try {
        const response = await fetch('/api/ntp/config');
        const data = await response.json();

        document.getElementById('ntp-enabled').checked = data.enabled;
        document.getElementById('ntp-enabled-label').textContent =
            data.enabled ? 'Habilitado' : 'Desabilitado';
        document.getElementById('ntp-server').value = data.server || 'pool.ntp.org';
        document.getElementById('ntp-offset').value = data.offset || -10800;
        document.getElementById('ntp-interval').value = data.interval || 3600000;
    } catch (error) {
        console.error('Error loading NTP config:', error);
    }
}

async function loadNTPTime() {
    try {
        const response = await fetch('/api/ntp/time');
        const data = await response.json();

        document.getElementById('current-time').textContent = data.time || '--:--:--';

        const statusCard = document.getElementById('ntp-status-card');
        const syncStatus = document.getElementById('ntp-sync-status');

        if (!data.enabled) {
            syncStatus.textContent = 'NTP Desabilitado';
            statusCard.className = 'status-card degraded';
        } else if (data.synced) {
            syncStatus.textContent = 'Sincronizado';
            statusCard.className = 'status-card healthy';
        } else {
            syncStatus.textContent = 'Aguardando sincronizacao';
            statusCard.className = 'status-card';
        }
    } catch (error) {
        console.error('Error loading NTP time:', error);
    }
}

async function saveNTPConfig() {
    const config = {
        enabled: document.getElementById('ntp-enabled').checked,
        server: document.getElementById('ntp-server').value,
        offset: parseInt(document.getElementById('ntp-offset').value),
        interval: parseInt(document.getElementById('ntp-interval').value)
    };

    if (!config.server) {
        alert('Por favor, digite o servidor NTP');
        return;
    }

    if (config.interval < 60000) {
        alert('O intervalo minimo e de 60000ms (60 segundos)');
        return;
    }

    try {
        const response = await fetch('/api/ntp/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });

        const data = await response.json();
        if (data.status === 'ok') {
            alert('Configuracao salva com sucesso!');
            loadNTPConfig();
        } else {
            alert('Erro: ' + (data.error || 'Falha ao salvar'));
        }
    } catch (error) {
        console.error('Error saving NTP config:', error);
        alert('Erro ao salvar configuracao');
    }
}
