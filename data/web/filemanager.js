// File Manager Functions
let currentPath = '/';
let editingFile = '';

document.addEventListener('DOMContentLoaded', function() {
    refreshFiles();
    setupDragDrop();
});

function setupDragDrop() {
    const uploadZone = document.getElementById('upload-zone');

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
        uploadFiles(e.dataTransfer.files);
    });

    uploadZone.addEventListener('click', () => {
        document.getElementById('fileInput').click();
    });
}

async function refreshFiles() {
    try {
        const response = await fetch(`/api/files/list?dir=${encodeURIComponent(currentPath)}`);
        const data = await response.json();

        document.getElementById('current-path').textContent = currentPath;

        const fileList = document.getElementById('file-list');
        fileList.innerHTML = '';

        // Add parent directory link if not at root
        if (currentPath !== '/') {
            const parentItem = document.createElement('li');
            parentItem.className = 'file-item';
            parentItem.innerHTML = `
                <span class="file-icon">📁</span>
                <div class="file-info">
                    <span class="file-name">..</span>
                    <span class="file-size">Pasta pai</span>
                </div>
            `;
            parentItem.onclick = () => navigateUp();
            fileList.appendChild(parentItem);
        }

        // Sort: directories first, then files
        const files = data.files || [];
        files.sort((a, b) => {
            if (a.isDir && !b.isDir) return -1;
            if (!a.isDir && b.isDir) return 1;
            return a.name.localeCompare(b.name);
        });

        files.forEach(file => {
            const item = document.createElement('li');
            item.className = 'file-item';

            const icon = file.isDir ? '📁' : getFileIcon(file.name);
            const size = file.isDir ? 'Pasta' : formatBytes(file.size);

            item.innerHTML = `
                <span class="file-icon">${icon}</span>
                <div class="file-info">
                    <span class="file-name">${file.name}</span>
                    <span class="file-size">${size}</span>
                </div>
                <div class="file-actions">
                    ${!file.isDir ? `
                        <button class="btn btn-primary action-btn" onclick="event.stopPropagation(); editFile('${currentPath}${file.name}')">✏️</button>
                        <button class="btn btn-success action-btn" onclick="event.stopPropagation(); downloadFile('${currentPath}${file.name}')">⬇️</button>
                    ` : ''}
                    <button class="btn btn-danger action-btn" onclick="event.stopPropagation(); deleteFile('${currentPath}${file.name}', ${file.isDir})">🗑️</button>
                </div>
            `;

            if (file.isDir) {
                item.onclick = () => navigateTo(file.name);
            }

            fileList.appendChild(item);
        });
    } catch (error) {
        console.error('Error loading files:', error);
    }
}

function getFileIcon(filename) {
    const ext = filename.split('.').pop().toLowerCase();
    const icons = {
        'html': '🌐',
        'css': '🎨',
        'js': '📜',
        'json': '📋',
        'txt': '📄',
        'md': '📝',
        'png': '🖼️',
        'jpg': '🖼️',
        'jpeg': '🖼️',
        'gif': '🖼️',
        'bin': '💾'
    };
    return icons[ext] || '📄';
}

function formatBytes(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
}

function navigateTo(dirname) {
    currentPath = currentPath + dirname + '/';
    refreshFiles();
}

function navigateUp() {
    const parts = currentPath.split('/').filter(p => p);
    parts.pop();
    currentPath = parts.length > 0 ? '/' + parts.join('/') + '/' : '/';
    refreshFiles();
}

async function uploadFiles(files) {
    for (const file of files) {
        const formData = new FormData();
        formData.append('file', file);
        formData.append('dir', currentPath);

        try {
            const response = await fetch('/api/files/upload?dir=' + encodeURIComponent(currentPath), {
                method: 'POST',
                body: formData
            });

            if (!response.ok) {
                alert(`Erro ao enviar ${file.name}`);
            }
        } catch (error) {
            console.error('Upload error:', error);
            alert(`Erro ao enviar ${file.name}`);
        }
    }
    refreshFiles();
}

async function editFile(filepath) {
    try {
        const response = await fetch(`/api/files/read?file=${encodeURIComponent(filepath)}`);
        const data = await response.json();

        if (data.error) {
            alert('Erro: ' + data.error);
            return;
        }

        editingFile = filepath;
        document.getElementById('edit-filename').textContent = filepath;
        document.getElementById('fileEditor').value = data.content;
        document.getElementById('editModal').style.display = 'flex';
    } catch (error) {
        console.error('Error loading file:', error);
        alert('Erro ao carregar arquivo');
    }
}

function closeEditModal() {
    document.getElementById('editModal').style.display = 'none';
    editingFile = '';
}

async function saveFile() {
    const content = document.getElementById('fileEditor').value;

    try {
        const formData = new FormData();
        formData.append('file', editingFile);
        formData.append('content', content);

        const response = await fetch('/api/files/write', {
            method: 'POST',
            body: formData
        });

        const data = await response.json();
        if (data.status === 'ok') {
            alert('Arquivo salvo com sucesso!');
            closeEditModal();
            refreshFiles();
        } else {
            alert('Erro ao salvar: ' + (data.error || 'Erro desconhecido'));
        }
    } catch (error) {
        console.error('Error saving file:', error);
        alert('Erro ao salvar arquivo');
    }
}

function downloadFile(filepath) {
    window.location.href = `/api/files/download?file=${encodeURIComponent(filepath)}`;
}

async function deleteFile(filepath, isDir) {
    const type = isDir ? 'pasta' : 'arquivo';
    if (!confirm(`Deseja excluir o ${type} "${filepath}"?`)) {
        return;
    }

    try {
        const formData = new FormData();
        formData.append('file', filepath);

        const response = await fetch('/api/files/delete', {
            method: 'POST',
            body: formData
        });

        const data = await response.json();
        if (data.status === 'ok') {
            refreshFiles();
        } else {
            alert('Erro ao excluir: ' + (data.error || 'Erro desconhecido'));
        }
    } catch (error) {
        console.error('Error deleting:', error);
        alert('Erro ao excluir');
    }
}

function createNewFile() {
    const filename = prompt('Nome do novo arquivo:');
    if (!filename) return;

    editingFile = currentPath + filename;
    document.getElementById('edit-filename').textContent = editingFile;
    document.getElementById('fileEditor').value = '';
    document.getElementById('editModal').style.display = 'flex';
}

async function createNewDir() {
    const dirname = prompt('Nome da nova pasta:');
    if (!dirname) return;

    try {
        const formData = new FormData();
        formData.append('dir', currentPath + dirname);

        const response = await fetch('/api/files/mkdir', {
            method: 'POST',
            body: formData
        });

        const data = await response.json();
        if (data.status === 'ok') {
            refreshFiles();
        } else {
            alert('Erro ao criar pasta: ' + (data.error || 'Erro desconhecido'));
        }
    } catch (error) {
        console.error('Error creating directory:', error);
        alert('Erro ao criar pasta');
    }
}
