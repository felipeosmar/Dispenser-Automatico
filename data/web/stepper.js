// Stepper Motor Configuration Functions
document.addEventListener('DOMContentLoaded', function() {
    loadStepperConfig();
    loadStepperStatus();
    setInterval(loadStepperStatus, 2000);

    document.getElementById('stepper-enabled').addEventListener('change', function() {
        document.getElementById('stepper-enabled-label').textContent =
            this.checked ? 'Habilitado' : 'Desabilitado';
    });

    document.getElementById('stepper-speed').addEventListener('input', function() {
        document.getElementById('speed-value').textContent = this.value;
    });
});

async function loadStepperConfig() {
    try {
        const response = await fetch('/api/stepper/config');
        const data = await response.json();

        document.getElementById('stepper-enabled').checked = data.enabled;
        document.getElementById('stepper-enabled-label').textContent =
            data.enabled ? 'Habilitado' : 'Desabilitado';
        document.getElementById('stepper-speed').value = data.speed || 10;
        document.getElementById('speed-value').textContent = data.speed || 10;
        document.getElementById('stepper-steps-per-rev').value = data.steps_per_rev || 2048;
        document.getElementById('stepper-pin1').value = data.pin1 || 25;
        document.getElementById('stepper-pin2').value = data.pin2 || 26;
        document.getElementById('stepper-pin3').value = data.pin3 || 27;
        document.getElementById('stepper-pin4').value = data.pin4 || 14;
    } catch (error) {
        console.error('Error loading stepper config:', error);
    }
}

async function loadStepperStatus() {
    try {
        const response = await fetch('/api/stepper/status');
        const data = await response.json();

        document.getElementById('stepper-position').textContent = 'Posicao: ' + data.position + ' passos';

        const statusCard = document.getElementById('stepper-status-card');
        const statusText = document.getElementById('stepper-status');

        if (!data.enabled) {
            statusText.textContent = 'Desabilitado';
            statusCard.className = 'status-card degraded';
        } else if (data.moving) {
            statusText.textContent = 'Em movimento...';
            statusCard.className = 'status-card';
        } else {
            statusText.textContent = 'Pronto';
            statusCard.className = 'status-card healthy';
        }
    } catch (error) {
        console.error('Error loading stepper status:', error);
    }
}

async function moveStepper(degrees) {
    try {
        const response = await fetch('/api/stepper/move', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ degrees: degrees })
        });

        const data = await response.json();
        if (data.status !== 'ok') {
            alert('Erro: ' + (data.error || 'Falha ao mover motor'));
        }
        loadStepperStatus();
    } catch (error) {
        console.error('Error moving stepper:', error);
        alert('Erro ao mover motor');
    }
}

async function moveCustomDegrees() {
    const degrees = parseInt(document.getElementById('custom-degrees').value);
    if (isNaN(degrees)) {
        alert('Por favor, digite um valor valido');
        return;
    }
    moveStepper(degrees);
}

async function moveCustomSteps() {
    const steps = parseInt(document.getElementById('custom-steps').value);
    if (isNaN(steps)) {
        alert('Por favor, digite um valor valido');
        return;
    }

    try {
        const response = await fetch('/api/stepper/step', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ steps: steps })
        });

        const data = await response.json();
        if (data.status !== 'ok') {
            alert('Erro: ' + (data.error || 'Falha ao mover motor'));
        }
        loadStepperStatus();
    } catch (error) {
        console.error('Error moving stepper:', error);
        alert('Erro ao mover motor');
    }
}

async function stopStepper() {
    try {
        const response = await fetch('/api/stepper/stop', {
            method: 'POST'
        });

        const data = await response.json();
        if (data.status === 'ok') {
            loadStepperStatus();
        } else {
            alert('Erro: ' + (data.error || 'Falha ao parar motor'));
        }
    } catch (error) {
        console.error('Error stopping stepper:', error);
        alert('Erro ao parar motor');
    }
}

async function resetPosition() {
    try {
        const response = await fetch('/api/stepper/reset', {
            method: 'POST'
        });

        const data = await response.json();
        if (data.status === 'ok') {
            loadStepperStatus();
        } else {
            alert('Erro: ' + (data.error || 'Falha ao zerar posicao'));
        }
    } catch (error) {
        console.error('Error resetting position:', error);
        alert('Erro ao zerar posicao');
    }
}

async function saveStepperConfig() {
    const config = {
        enabled: document.getElementById('stepper-enabled').checked,
        speed: parseInt(document.getElementById('stepper-speed').value),
        steps_per_rev: parseInt(document.getElementById('stepper-steps-per-rev').value),
        pin1: parseInt(document.getElementById('stepper-pin1').value),
        pin2: parseInt(document.getElementById('stepper-pin2').value),
        pin3: parseInt(document.getElementById('stepper-pin3').value),
        pin4: parseInt(document.getElementById('stepper-pin4').value)
    };

    // Validation
    if (config.speed < 1 || config.speed > 15) {
        alert('A velocidade deve estar entre 1 e 15 RPM');
        return;
    }

    if (config.steps_per_rev < 200 || config.steps_per_rev > 8000) {
        alert('Passos por volta deve estar entre 200 e 8000');
        return;
    }

    // Check for duplicate pins
    const pins = [config.pin1, config.pin2, config.pin3, config.pin4];
    const uniquePins = new Set(pins);
    if (uniquePins.size !== 4) {
        alert('Os pinos devem ser diferentes uns dos outros');
        return;
    }

    // Check valid GPIO range
    for (const pin of pins) {
        if (pin < 0 || pin > 39) {
            alert('Os pinos devem estar entre 0 e 39');
            return;
        }
    }

    try {
        const response = await fetch('/api/stepper/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(config)
        });

        const data = await response.json();
        if (data.status === 'ok') {
            alert('Configuracao salva com sucesso!');
            loadStepperConfig();
        } else {
            alert('Erro: ' + (data.error || 'Falha ao salvar'));
        }
    } catch (error) {
        console.error('Error saving stepper config:', error);
        alert('Erro ao salvar configuracao');
    }
}
