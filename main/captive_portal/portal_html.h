/**
 * =============================================================================
 * CalX ESP32 Firmware - Captive Portal HTML
 * =============================================================================
 * Embedded HTML/CSS/JS for WiFi setup portal.
 * Styled to match CalX Frontend design system.
 * =============================================================================
 */

#ifndef PORTAL_HTML_H
#define PORTAL_HTML_H

static const char PORTAL_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no">
    <title>CalX WiFi Setup</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&display=swap" rel="stylesheet">
    <style>
        :root {
            --background: hsl(210, 11%, 7%);
            --foreground: hsl(160, 14%, 93%);
            --primary: hsl(165, 96%, 71%);
            --primary-dark: hsl(160, 100%, 50%);
            --muted: hsl(240, 2%, 16%);
            --muted-foreground: hsla(160, 14%, 93%, 0.7);
            --border: hsla(240, 100%, 100%, 0.08);
            --card: hsla(220, 17%, 98%, 0.01);
            --error: hsl(0, 62%, 50%);
            --success: hsl(142, 76%, 36%);
            --radius: 0.5rem;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Inter', -apple-system, BlinkMacSystemFont, sans-serif;
            background: var(--background);
            color: var(--foreground);
            min-height: 100vh;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: flex-start;
            padding: 20px;
            padding-top: 40px;
        }

        .container {
            width: 100%;
            max-width: 400px;
        }

        /* Header */
        .header {
            text-align: center;
            margin-bottom: 32px;
        }

        .logo {
            font-size: 2.5rem;
            font-weight: 700;
            color: var(--primary);
            text-shadow: 0 0 30px hsla(165, 96%, 71%, 0.4);
            margin-bottom: 8px;
        }

        .subtitle {
            color: var(--muted-foreground);
            font-size: 0.9rem;
        }

        /* Card */
        .card {
            background: var(--card);
            backdrop-filter: blur(10px);
            border: 1px solid var(--border);
            border-radius: calc(var(--radius) * 2);
            padding: 24px;
            margin-bottom: 16px;
        }

        .card-title {
            font-size: 1rem;
            font-weight: 600;
            margin-bottom: 16px;
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .card-title svg {
            width: 20px;
            height: 20px;
            color: var(--primary);
        }

        /* Network List */
        .network-list {
            display: flex;
            flex-direction: column;
            gap: 8px;
            max-height: 250px;
            overflow-y: auto;
        }

        .network-item {
            display: flex;
            align-items: center;
            padding: 12px 16px;
            background: var(--muted);
            border: 1px solid var(--border);
            border-radius: var(--radius);
            cursor: pointer;
            transition: all 0.2s ease;
        }

        .network-item:hover {
            border-color: var(--primary);
            transform: translateX(4px);
        }

        .network-item.selected {
            border-color: var(--primary);
            background: hsla(165, 96%, 71%, 0.1);
        }

        .network-ssid {
            flex: 1;
            font-weight: 500;
        }

        .network-signal {
            display: flex;
            gap: 2px;
            margin-right: 8px;
        }

        .signal-bar {
            width: 4px;
            background: var(--muted-foreground);
            border-radius: 2px;
        }

        .signal-bar.active {
            background: var(--primary);
        }

        .network-lock {
            width: 16px;
            height: 16px;
            color: var(--muted-foreground);
        }

        /* Password Input */
        .input-group {
            margin-bottom: 16px;
        }

        .input-group label {
            display: block;
            font-size: 0.875rem;
            color: var(--muted-foreground);
            margin-bottom: 6px;
        }

        .input-wrapper {
            position: relative;
        }

        .input-wrapper input {
            width: 100%;
            padding: 12px 16px;
            padding-right: 48px;
            background: var(--muted);
            border: 1px solid var(--border);
            border-radius: var(--radius);
            color: var(--foreground);
            font-size: 1rem;
            font-family: inherit;
            transition: all 0.2s ease;
        }

        .input-wrapper input:focus {
            outline: none;
            border-color: var(--primary);
            box-shadow: 0 0 0 3px hsla(165, 96%, 71%, 0.15);
        }

        .toggle-password {
            position: absolute;
            right: 12px;
            top: 50%;
            transform: translateY(-50%);
            background: none;
            border: none;
            color: var(--muted-foreground);
            cursor: pointer;
            padding: 4px;
        }

        .toggle-password:hover {
            color: var(--primary);
        }

        /* Buttons */
        .btn {
            width: 100%;
            padding: 14px 24px;
            border: none;
            border-radius: var(--radius);
            font-size: 1rem;
            font-weight: 600;
            font-family: inherit;
            cursor: pointer;
            transition: all 0.2s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            gap: 8px;
        }

        .btn-primary {
            background: var(--primary);
            color: hsl(160, 8%, 6%);
        }

        .btn-primary:hover:not(:disabled) {
            background: var(--primary-dark);
            box-shadow: 0 0 20px hsla(165, 96%, 71%, 0.4);
        }

        .btn-primary:disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }

        .btn-secondary {
            background: var(--muted);
            color: var(--foreground);
            border: 1px solid var(--border);
        }

        .btn-secondary:hover {
            border-color: var(--primary);
        }

        /* Status States */
        .status {
            text-align: center;
            padding: 40px 20px;
        }

        .status-icon {
            width: 64px;
            height: 64px;
            margin: 0 auto 16px;
        }

        .status-icon.success {
            color: var(--success);
        }

        .status-icon.error {
            color: var(--error);
        }

        .status-title {
            font-size: 1.25rem;
            font-weight: 600;
            margin-bottom: 8px;
        }

        .status-message {
            color: var(--muted-foreground);
        }

        /* Spinner */
        .spinner {
            width: 40px;
            height: 40px;
            border: 3px solid var(--muted);
            border-top-color: var(--primary);
            border-radius: 50%;
            animation: spin 0.8s linear infinite;
            margin: 0 auto 16px;
        }

        @keyframes spin {
            to { transform: rotate(360deg); }
        }

        /* Scanning Animation */
        .scanning-text {
            color: var(--primary);
            animation: pulse 1.5s ease-in-out infinite;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.5; }
        }

        /* Scrollbar */
        ::-webkit-scrollbar {
            width: 6px;
        }

        ::-webkit-scrollbar-track {
            background: var(--muted);
            border-radius: 3px;
        }

        ::-webkit-scrollbar-thumb {
            background: var(--border);
            border-radius: 3px;
        }

        ::-webkit-scrollbar-thumb:hover {
            background: var(--muted-foreground);
        }

        /* Hidden */
        .hidden {
            display: none !important;
        }
    </style>
</head>
<body>
    <div class="container">
        <!-- Header -->
        <div class="header">
            <div class="logo">CalX</div>
            <p class="subtitle">WiFi Setup</p>
        </div>

        <!-- Scanning State -->
        <div id="scanningState" class="card">
            <div class="status">
                <div class="spinner"></div>
                <p class="scanning-text">Scanning for networks...</p>
            </div>
        </div>

        <!-- Network Selection -->
        <div id="networkState" class="card hidden">
            <div class="card-title">
                <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <path d="M5 12.55a11 11 0 0 1 14.08 0"></path>
                    <path d="M1.42 9a16 16 0 0 1 21.16 0"></path>
                    <path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path>
                    <line x1="12" y1="20" x2="12.01" y2="20"></line>
                </svg>
                Available Networks
            </div>
            <div id="networkList" class="network-list">
                <!-- Networks will be inserted here -->
            </div>
            <button onclick="scanNetworks()" class="btn btn-secondary" style="margin-top: 16px;">
                <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <polyline points="23 4 23 10 17 10"></polyline>
                    <path d="M20.49 15a9 9 0 1 1-2.12-9.36L23 10"></path>
                </svg>
                Refresh
            </button>
        </div>

        <!-- Password Input -->
        <div id="passwordState" class="card hidden">
            <div class="card-title">Connect to <span id="selectedNetwork"></span></div>
            <div class="input-group">
                <label for="password">Password</label>
                <div class="input-wrapper">
                    <input type="password" id="password" placeholder="Enter password">
                    <button type="button" class="toggle-password" onclick="togglePassword()">
                        <svg id="eyeIcon" xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                            <path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path>
                            <circle cx="12" cy="12" r="3"></circle>
                        </svg>
                    </button>
                </div>
            </div>
            <button onclick="connectToNetwork()" id="connectBtn" class="btn btn-primary">
                Connect
            </button>
            <button onclick="showNetworks()" class="btn btn-secondary" style="margin-top: 8px;">
                Back
            </button>
        </div>

        <!-- Connecting State -->
        <div id="connectingState" class="card hidden">
            <div class="status">
                <div class="spinner"></div>
                <p class="status-title">Connecting...</p>
                <p class="status-message">Please wait while CalX connects to your network</p>
            </div>
        </div>

        <!-- Success State -->
        <div id="successState" class="card hidden">
            <div class="status">
                <svg class="status-icon success" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="10"></circle>
                    <polyline points="9 12 12 15 16 10"></polyline>
                </svg>
                <p class="status-title">Connected!</p>
                <p class="status-message">CalX is connected. Connect your phone to the same WiFi network, then access:</p>
                <p id="newIpAddress" style="color: var(--primary); font-weight: 600; margin-top: 8px; font-size: 1.1rem;"></p>
            </div>
        </div>

        <!-- Error State -->
        <div id="errorState" class="card hidden">
            <div class="status">
                <svg class="status-icon error" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                    <circle cx="12" cy="12" r="10"></circle>
                    <line x1="15" y1="9" x2="9" y2="15"></line>
                    <line x1="9" y1="9" x2="15" y2="15"></line>
                </svg>
                <p class="status-title">Connection Failed</p>
                <p id="errorMessage" class="status-message">Please check your password and try again.</p>
            </div>
            <button onclick="showNetworks()" class="btn btn-primary" style="margin-top: 16px;">
                Try Again
            </button>
        </div>
    </div>

    <script>
        let selectedSSID = '';
        let isSecure = false;

        // Initial scan
        window.onload = () => {
            scanNetworks();
        };

        function scanNetworks() {
            showState('scanning');
            
            fetch('/scan')
                .then(res => res.json())
                .then(networks => {
                    renderNetworks(networks);
                    showState('network');
                })
                .catch(err => {
                    console.error('Scan failed:', err);
                    showState('network');
                });
        }

        function renderNetworks(networks) {
            const list = document.getElementById('networkList');
            list.innerHTML = '';
            
            networks.forEach(net => {
                const div = document.createElement('div');
                div.className = 'network-item';
                div.onclick = () => selectNetwork(net.ssid, net.secure);
                div.innerHTML = `
                    <span class="network-ssid">${escapeHtml(net.ssid)}</span>
                    <span style="color: var(--muted-foreground); font-size: 0.85rem; margin-left: auto;">${net.rssi} dBm</span>
                    ${net.secure ? '<svg class="network-lock" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect><path d="M7 11V7a5 5 0 0 1 10 0v4"></path></svg>' : ''}
                `;
                list.appendChild(div);
            });
        }

        function getSignalBars(rssi) {
            const strength = rssi > -50 ? 4 : rssi > -60 ? 3 : rssi > -70 ? 2 : 1;
            let bars = '';
            for (let i = 4; i >= 1; i--) {
                const height = 4 + ((5 - i) * 3);  // Inverted: i=4 gives shortest, i=1 gives tallest
                const active = i <= strength ? 'active' : '';
                bars += `<div class="signal-bar ${active}" style="height: ${height}px;"></div>`;
            }
            return bars;
        }

        function selectNetwork(ssid, secure) {
            selectedSSID = ssid;
            isSecure = secure;
            
            document.getElementById('selectedNetwork').textContent = ssid;
            
            if (secure) {
                document.getElementById('password').value = '';
                showState('password');
            } else {
                connectToNetwork();
            }
        }

        function showNetworks() {
            showState('network');
        }

        function togglePassword() {
            const input = document.getElementById('password');
            const icon = document.getElementById('eyeIcon');
            
            if (input.type === 'password') {
                input.type = 'text';
                icon.innerHTML = '<path d="M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19m-6.72-1.07a3 3 0 1 1-4.24-4.24"></path><line x1="1" y1="1" x2="23" y2="23"></line>';
            } else {
                input.type = 'password';
                icon.innerHTML = '<path d="M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z"></path><circle cx="12" cy="12" r="3"></circle>';
            }
        }

        function connectToNetwork() {
            const password = document.getElementById('password')?.value || '';
            
            showState('connecting');
            
            fetch('/connect', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid: selectedSSID, password: password })
            })
            .then(res => res.json())
            .then(data => {
                if (data.status === 'connecting') {
                    // Wait then check status to get new IP
                    setTimeout(() => {
                        fetch('/status')
                            .then(res => res.json())
                            .then(status => {
                                if (status.wifi_connected && status.ip) {
                                    document.getElementById('newIpAddress').textContent = `http://${status.ip}/display`;
                                } else {
                                    document.getElementById('newIpAddress').textContent = 'Check your router for device IP';
                                }
                                showState('success');
                            })
                            .catch(() => {
                                document.getElementById('newIpAddress').textContent = 'Check your router for device IP';
                                showState('success');
                            });
                    }, 5000);
                }
            })
            .catch(err => {
                console.error('Connect failed:', err);
                document.getElementById('errorMessage').textContent = 'Connection failed. Please try again.';
                showState('error');
            });
        }

        function showState(state) {
            ['scanning', 'network', 'password', 'connecting', 'success', 'error'].forEach(s => {
                document.getElementById(s + 'State').classList.add('hidden');
            });
            document.getElementById(state + 'State').classList.remove('hidden');
        }

        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }
    </script>
</body>
</html>
)rawliteral";

#endif // PORTAL_HTML_H
