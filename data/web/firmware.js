// Firmware Update Functions
document.addEventListener('DOMContentLoaded', function() {
    setupFirmwareUpload();
});

function setupFirmwareUpload() {
    const uploadZone = document.getElementById('firmware-upload-zone');
    const fileInput = document.getElementById('firmwareInput');

    uploadZone.addEventListener('dragover', (e) => {
        e.preventDefault();
        uploadZone.classList.add('drag-over');
    });

    uploadZone.addEventListener('dragleave', () => {
        uploadZone.classList.remove('drag-over');
    });

    uploadZone.addEventListener('drop', (e) => {
        e.preventDefault();
        uploadZone.classList.remove('drag-over');
        const files = e.dataTransfer.files;
        if (files.length > 0) {
            uploadFirmware(files[0]);
        }
    });

    uploadZone.addEventListener('click', () => {
        fileInput.click();
    });

    fileInput.addEventListener('change', (e) => {
        if (e.target.files.length > 0) {
            uploadFirmware(e.target.files[0]);
        }
    });
}

async function uploadFirmware(file) {
    if (!file.name.endsWith('.bin')) {
        alert('Por favor, selecione um arquivo .bin');
        return;
    }

    if (!confirm('Deseja realmente atualizar o firmware? O dispositivo sera reiniciado apos a atualizacao.')) {
        return;
    }

    const progressSection = document.getElementById('progress-section');
    const progressFill = document.getElementById('progress-fill');
    const progressText = document.getElementById('progress-text');
    const statusMessage = document.getElementById('status-message');
    const successSection = document.getElementById('success-section');

    progressSection.style.display = 'block';
    successSection.style.display = 'none';

    const formData = new FormData();
    formData.append('firmware', file);

    const xhr = new XMLHttpRequest();

    xhr.upload.addEventListener('progress', (e) => {
        if (e.lengthComputable) {
            const percent = Math.round((e.loaded / e.total) * 100);
            progressFill.style.width = percent + '%';
            progressText.textContent = percent + '%';
            statusMessage.textContent = `Enviando... ${formatBytes(e.loaded)} / ${formatBytes(e.total)}`;
        }
    });

    xhr.addEventListener('load', () => {
        if (xhr.status === 200) {
            progressFill.style.width = '100%';
            progressText.textContent = '100%';
            statusMessage.textContent = 'Atualizacao concluida!';
            progressFill.classList.remove('warning', 'danger');

            setTimeout(() => {
                progressSection.style.display = 'none';
                successSection.style.display = 'block';
            }, 1000);
        } else {
            let errorMsg = 'Erro na atualizacao';
            try {
                const response = JSON.parse(xhr.responseText);
                errorMsg = response.error || errorMsg;
            } catch (e) {}
            statusMessage.textContent = errorMsg;
            progressFill.classList.add('danger');
            alert('Erro: ' + errorMsg);
        }
    });

    xhr.addEventListener('error', () => {
        statusMessage.textContent = 'Erro de conexao';
        progressFill.classList.add('danger');
        alert('Erro de conexao durante a atualizacao');
    });

    xhr.open('POST', '/api/firmware/upload');
    xhr.send(formData);
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}
