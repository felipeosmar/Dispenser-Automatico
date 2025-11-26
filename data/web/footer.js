// Footer inclusion
function loadFooter() {
    const footerPlaceholder = document.getElementById('footer-placeholder');
    if (footerPlaceholder) {
        footerPlaceholder.innerHTML = `
            <footer>
                <p>ESP32 Dispenser v1.0 | Ultima atualizacao: <span id="last-update">-</span></p>
            </footer>
        `;
        updateLastUpdateTime();
    }
}

function updateLastUpdateTime() {
    const lastUpdate = document.getElementById('last-update');
    if (lastUpdate) {
        const now = new Date();
        lastUpdate.textContent = now.toLocaleString('pt-BR');
    }
}

document.addEventListener('DOMContentLoaded', function() {
    loadFooter();
    setInterval(updateLastUpdateTime, 60000);
});
