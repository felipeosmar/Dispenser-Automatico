// Authentication Functions
document.addEventListener('DOMContentLoaded', function() {
    loadAuthStatus();
});

async function loadAuthStatus() {
    try {
        const response = await fetch('/api/auth/status');
        const data = await response.json();

        document.getElementById('current-username').textContent = data.username || '-';
        document.getElementById('new-username').value = data.username || 'admin';

        if (data.first_login) {
            document.getElementById('auth-status').textContent = 'Senha padrao ativa';
            document.getElementById('first-login-warning').style.display = 'block';
        } else {
            document.getElementById('auth-status').textContent = 'Configurado';
            document.getElementById('first-login-warning').style.display = 'none';
        }
    } catch (error) {
        console.error('Error loading auth status:', error);
    }
}

function togglePassword(inputId) {
    const input = document.getElementById(inputId);
    input.type = input.type === 'password' ? 'text' : 'password';
}

function checkPasswordStrength() {
    const password = document.getElementById('new-password').value;
    const strengthLevel = document.getElementById('strength-level');
    const strengthText = document.getElementById('strength-text');

    let strength = 0;
    let status = '';

    if (password.length >= 8) strength++;
    if (password.length >= 12) strength++;
    if (/[A-Z]/.test(password)) strength++;
    if (/[a-z]/.test(password)) strength++;
    if (/[0-9]/.test(password)) strength++;
    if (/[^A-Za-z0-9]/.test(password)) strength++;

    strengthLevel.className = 'strength-level';

    if (password.length === 0) {
        strengthLevel.style.width = '0%';
        status = 'Digite uma senha';
    } else if (strength <= 2) {
        strengthLevel.style.width = '25%';
        strengthLevel.classList.add('weak');
        status = 'Fraca';
    } else if (strength <= 3) {
        strengthLevel.style.width = '50%';
        strengthLevel.classList.add('medium');
        status = 'Media';
    } else if (strength <= 4) {
        strengthLevel.style.width = '75%';
        strengthLevel.classList.add('good');
        status = 'Boa';
    } else {
        strengthLevel.style.width = '100%';
        strengthLevel.classList.add('strong');
        status = 'Forte';
    }

    strengthText.textContent = status;
}

async function changePassword() {
    const username = document.getElementById('new-username').value;
    const currentPassword = document.getElementById('current-password').value;
    const newPassword = document.getElementById('new-password').value;
    const confirmPassword = document.getElementById('confirm-password').value;

    // Validations
    if (!username) {
        alert('Por favor, digite o usuario');
        return;
    }

    if (!currentPassword) {
        alert('Por favor, digite a senha atual');
        return;
    }

    if (!newPassword) {
        alert('Por favor, digite a nova senha');
        return;
    }

    if (newPassword.length < 8) {
        alert('A senha deve ter no minimo 8 caracteres');
        return;
    }

    if (!/[A-Z]/.test(newPassword)) {
        alert('A senha deve conter pelo menos 1 letra maiuscula');
        return;
    }

    if (!/[a-z]/.test(newPassword)) {
        alert('A senha deve conter pelo menos 1 letra minuscula');
        return;
    }

    if (!/[0-9]/.test(newPassword)) {
        alert('A senha deve conter pelo menos 1 numero');
        return;
    }

    if (newPassword !== confirmPassword) {
        alert('As senhas nao coincidem');
        return;
    }

    try {
        const response = await fetch('/api/auth/change-password', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                username: username,
                current_password: currentPassword,
                new_password: newPassword
            })
        });

        const data = await response.json();

        if (data.status === 'ok') {
            alert('Credenciais alteradas com sucesso! Faca login novamente.');
            // Force re-login
            window.location.href = '/';
        } else {
            alert('Erro: ' + (data.error || 'Falha ao alterar credenciais'));
        }
    } catch (error) {
        console.error('Error changing password:', error);
        alert('Erro ao alterar credenciais');
    }
}
