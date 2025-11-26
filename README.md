# ESP32 Dispenser Automatico

Sistema de controle para dispenser automatico baseado em ESP32 com interface web completa.

## Funcionalidades

- **WiFi Manager** - Modo AP e Station com fallback automatico
- **Interface Web** - Dashboard responsivo servido via LittleFS
- **Gerenciador de Arquivos** - Upload, download, edicao e exclusao de arquivos
- **NTP Manager** - Sincronizacao de horario via internet
- **Motor de Passo** - Controle de motor stepper com configuracao via web
- **OTA Updates** - Atualizacao de firmware via interface web
- **Autenticacao** - Sistema de login com hash SHA256

## Hardware

- ESP32 NodeMCU-32S
- Motor de passo 28BYJ-48 com driver ULN2003 (opcional)

### Pinagem Padrao do Motor de Passo

| Pino ESP32 | Driver ULN2003 |
|------------|----------------|
| GPIO 25    | IN1            |
| GPIO 26    | IN2            |
| GPIO 27    | IN3            |
| GPIO 14    | IN4            |

## Instalacao

### Pre-requisitos

- [PlatformIO](https://platformio.org/) (VSCode ou CLI)
- Python 3.x

### Compilar e Enviar

```bash
# Compilar
pio run

# Enviar firmware
pio run --target upload

# Enviar sistema de arquivos (interface web)
pio run --target uploadfs

# Monitor serial
pio device monitor
```

## Configuracao

### Primeiro Acesso

1. Conecte-se a rede WiFi `Dispenser-AP` (senha: `12345678`)
2. Acesse `http://192.168.4.1`
3. Login padrao: `admin` / `admin`
4. Altere a senha no primeiro login

### Arquivo de Configuracao

O arquivo `/config.json` contem todas as configuracoes:

```json
{
  "web": {
    "username": "admin",
    "password_hash": "...",
    "first_login": true
  },
  "wifi": {
    "ssid": "SuaRede",
    "password": "SuaSenha",
    "ap_mode": false
  },
  "ntp": {
    "server": "pool.ntp.org",
    "offset": -10800,
    "interval": 3600000,
    "enabled": true
  },
  "stepper": {
    "pin1": 25,
    "pin2": 26,
    "pin3": 27,
    "pin4": 14,
    "speed": 10,
    "steps_per_rev": 2048,
    "enabled": true
  }
}
```

## Interface Web

| Pagina | Descricao |
|--------|-----------|
| `/` | Pagina principal |
| `/wifi` | Configuracao WiFi |
| `/ntp` | Configuracao de horario |
| `/stepper` | Controle do motor de passo |
| `/status` | Status do sistema |
| `/firmware` | Atualizacao OTA |
| `/filemanager` | Gerenciador de arquivos |
| `/auth` | Configuracao de senha |

## API REST

### Status
- `GET /api/status` - Status geral do sistema

### WiFi
- `GET /api/wifi/scan` - Escanear redes
- `GET /api/wifi/status` - Status da conexao
- `POST /api/wifi/connect` - Conectar a uma rede

### NTP
- `GET /api/ntp/config` - Obter configuracao
- `POST /api/ntp/config` - Salvar configuracao
- `GET /api/ntp/time` - Obter hora atual

### Motor de Passo
- `GET /api/stepper/config` - Obter configuracao
- `POST /api/stepper/config` - Salvar configuracao
- `GET /api/stepper/status` - Status do motor
- `POST /api/stepper/move` - Mover em graus `{"degrees": 90}`
- `POST /api/stepper/step` - Mover em passos `{"steps": 100}`
- `POST /api/stepper/stop` - Parar motor
- `POST /api/stepper/reset` - Zerar posicao

### Arquivos
- `GET /api/files/list?dir=/` - Listar arquivos
- `GET /api/files/download?file=/path` - Download
- `POST /api/files/upload` - Upload
- `POST /api/files/delete` - Deletar
- `POST /api/files/mkdir` - Criar diretorio

## Estrutura do Projeto

```
Dispenser-Automatico/
├── src/
│   ├── main.cpp              # Ponto de entrada
│   ├── config.h              # Configuracoes padrao
│   ├── web_server.h/cpp      # Servidor web e API
│   ├── spiffs_manager.h/cpp  # Gerenciador LittleFS
│   ├── ntp_manager.h/cpp     # Sincronizacao NTP
│   ├── stepper_manager.h/cpp # Controle motor de passo
│   └── auth_manager.h        # Autenticacao
├── data/
│   ├── config.json           # Configuracoes
│   └── web/                  # Interface web
│       ├── index.html
│       ├── wifi.html
│       ├── ntp.html
│       ├── stepper.html
│       ├── status.html
│       ├── firmware.html
│       ├── filemanager.html
│       ├── auth.html
│       ├── header.html
│       ├── unified.css
│       └── *.js
└── platformio.ini            # Configuracao PlatformIO
```

## Dependencias

- ESPAsyncWebServer
- AsyncTCP
- ArduinoJson
- NTPClient
- LittleFS

## Memoria

- Flash: ~70% utilizado
- RAM: ~14% utilizado

## Licenca

MIT License
