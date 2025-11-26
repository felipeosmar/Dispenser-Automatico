// Header inclusion and initialization
function loadHeader(pageTitle) {
    fetch('/header.html')
        .then(response => response.text())
        .then(html => {
            const headerPlaceholder = document.getElementById('header-placeholder');
            if (headerPlaceholder) {
                headerPlaceholder.innerHTML = html;
                const titleElement = document.getElementById('page-title');
                if (titleElement && pageTitle) {
                    titleElement.textContent = pageTitle;
                }
                initConnectionStatus();
                initSidebar();
                setActiveMenuItem();
            }
        })
        .catch(error => console.error('Error loading header:', error));
}

function initConnectionStatus() {
    const statusDot = document.getElementById('connection-status');
    const statusText = document.getElementById('status-text');

    if (statusDot && statusText) {
        fetch('/api/status')
            .then(response => response.json())
            .then(data => {
                statusDot.classList.add('connected');
                statusText.textContent = 'Conectado';
            })
            .catch(() => {
                statusDot.classList.remove('connected');
                statusText.textContent = 'Desconectado';
            });

        setInterval(() => {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    statusDot.classList.add('connected');
                    statusText.textContent = 'Conectado';
                })
                .catch(() => {
                    statusDot.classList.remove('connected');
                    statusText.textContent = 'Desconectado';
                });
        }, 5000);
    }
}

function initSidebar() {
    const sidebar = document.getElementById('sidebar');
    const sidebarToggle = document.getElementById('sidebar-toggle');
    const sidebarClose = document.getElementById('sidebar-close');
    const sidebarOverlay = document.getElementById('sidebar-overlay');

    if (!sidebar || !sidebarToggle || !sidebarClose || !sidebarOverlay) {
        return;
    }

    sidebarToggle.addEventListener('click', () => {
        sidebar.classList.add('open');
        sidebarOverlay.classList.add('show');
    });

    const closeSidebar = () => {
        sidebar.classList.remove('open');
        sidebarOverlay.classList.remove('show');
    };

    sidebarClose.addEventListener('click', closeSidebar);
    sidebarOverlay.addEventListener('click', closeSidebar);

    const sidebarLinks = sidebar.querySelectorAll('.sidebar-link');
    sidebarLinks.forEach(link => {
        link.addEventListener('click', () => {
            if (window.innerWidth <= 768) {
                closeSidebar();
            }
        });
    });
}

function setActiveMenuItem() {
    const currentPath = window.location.pathname;
    const sidebarLinks = document.querySelectorAll('.sidebar-link');

    sidebarLinks.forEach(link => {
        const linkPath = new URL(link.href).pathname;
        if (currentPath === '/' && linkPath === '/') {
            link.classList.add('active');
        } else if (currentPath !== '/' && linkPath !== '/' && currentPath.startsWith(linkPath)) {
            link.classList.add('active');
        } else {
            link.classList.remove('active');
        }
    });
}
