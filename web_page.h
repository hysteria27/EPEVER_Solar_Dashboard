const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>EPEVER Solar Monitor v9.8</title>
    <script src="https://cdn.tailwindcss.com"></script>
    <script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
    <script src="https://unpkg.com/lucide@latest"></script>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;600;800&display=swap" rel="stylesheet">
    <script>
        tailwind.config = {
            darkMode: 'class',
            theme: {
                extend: {
                    fontFamily: { sans: ['Inter', 'sans-serif'] },
                    colors: { solar: { 50: '#f0fdf4', 100: '#dcfce7', 500: '#008744', 600: '#16a34a' }, slate: { 850: '#152033', 900: '#0f172a', 950: '#020617' } }
                }
            }
        }
    </script>
    <style>
        :root { --transition-speed: 0.3s; }
        body { transition: background-color var(--transition-speed) ease, color var(--transition-speed) ease; }
        .hidden-view { display: none !important; }
        .gauge-ring { transform: rotate(-90deg); transform-origin: 50% 50%; transition: stroke-dashoffset 1s, stroke 0.5s; }
        ::-webkit-scrollbar { width: 6px; height: 6px; }
        ::-webkit-scrollbar-track { background: transparent; }
        ::-webkit-scrollbar-thumb { background: #cbd5e1; border-radius: 4px; }
        .dark ::-webkit-scrollbar-thumb { background: #334155; }
        input[type=number] { -moz-appearance: textfield; }
        .has-tooltip { cursor: help; }
        input:disabled, select:disabled { background-color: #e2e8f0; color: #94a3b8; cursor: not-allowed; }
        .dark input:disabled, .dark select:disabled { background-color: #334155; color: #64748b; }
    </style>
</head>
<body class="bg-gray-50 dark:bg-slate-950 text-gray-800 dark:text-gray-100 min-h-screen flex flex-col">
    <div class="max-w-7xl mx-auto w-full p-4 md:p-6 lg:p-8">
        <header class="flex flex-col md:flex-row justify-between items-start md:items-center mb-8 gap-4">
            <div><h1 class="text-2xl md:text-3xl font-extrabold tracking-tight flex items-center gap-2"><i data-lucide="zap" class="text-solar-500 w-8 h-8"></i>EPEVER Solar Monitor v9.8</h1><p class="text-gray-500 dark:text-gray-400 text-sm mt-1 flex items-center gap-2"><span id="conn-dot" class="inline-block w-2 h-2 rounded-full bg-red-500"></span><span id="connection-status">Connecting...</span></p></div>
            <div class="flex bg-white dark:bg-slate-900 p-1 rounded-lg border border-gray-200 dark:border-slate-800 shadow-sm flex-wrap gap-1">
                <button onclick="switchView('dashboard')" id="btn-dashboard" class="px-4 py-2 rounded-md text-sm font-medium flex gap-2 items-center transition-all"><i data-lucide="layout-dashboard" class="w-4 h-4"></i> Live</button>
                <button onclick="switchView('history')" id="btn-history" class="px-4 py-2 rounded-md text-sm font-medium flex gap-2 items-center transition-all"><i data-lucide="calendar-clock" class="w-4 h-4"></i> Log</button>
                <button onclick="switchView('settings')" id="btn-settings" class="px-4 py-2 rounded-md text-sm font-medium flex gap-2 items-center transition-all"><i data-lucide="settings" class="w-4 h-4"></i> Settings</button>
                <button onclick="switchView('firmware')" id="btn-firmware" class="px-4 py-2 rounded-md text-sm font-medium flex gap-2 items-center transition-all"><i data-lucide="package" class="w-4 h-4"></i> Firmware</button>
            </div>
            <button onclick="toggleTheme()" class="absolute top-6 right-6 md:static p-2 rounded-full hover:bg-gray-200 dark:hover:bg-slate-800 transition-colors"><i id="theme-icon" data-lucide="moon" class="w-5 h-5"></i></button>
        </header>

        <div id="view-dashboard" class="fade-in space-y-6">
            <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-4 gap-4">
                <div class="bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-5 rounded-xl shadow-sm has-tooltip" title="Current charging stage"><div><p class="text-xs font-bold text-gray-400 uppercase">Status</p><h3 id="charging-stage-display" class="text-lg font-bold mt-1">Standby</h3></div></div>
                <div class="bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-5 rounded-xl shadow-sm has-tooltip" title="Real-time power"><div><p class="text-xs font-bold text-gray-400 uppercase">PV Power</p><h3 id="panel-power" class="text-2xl font-black">0</h3><span class="text-sm text-gray-500">W</span></div><div class="w-full bg-gray-100 dark:bg-slate-800 h-1.5 rounded-full mt-3 overflow-hidden"><div id="pv-bar" class="bg-orange-500 h-full" style="width: 0%"></div></div></div>
                <div class="bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-5 rounded-xl shadow-sm has-tooltip" title="Battery voltage"><div><p class="text-xs font-bold text-gray-400 uppercase">Battery</p><h3 id="battery-voltage" class="text-2xl font-black">0.0</h3><span class="text-sm text-gray-500">V</span></div></div>
                <div class="bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-5 rounded-xl shadow-sm has-tooltip" title="DC Load power"><div><p class="text-xs font-bold text-gray-400 uppercase">DC Load</p><h3 id="load-power" class="text-2xl font-black">0</h3><span class="text-sm text-gray-500">W</span></div></div>
            </div>
            <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
                <div class="bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-6 rounded-xl shadow-sm flex flex-col items-center justify-center has-tooltip" title="State of Charge"><h3 class="text-sm font-bold text-gray-500 mb-4">SOC</h3><div class="w-48 h-48 relative"><svg class="w-full h-full" viewBox="0 0 100 100"><circle cx="50" cy="50" r="45" fill="none" stroke="currentColor" stroke-width="8" class="text-gray-100 dark:text-slate-800"></circle><circle id="gauge-ring" cx="50" cy="50" r="45" fill="none" stroke="#eab308" stroke-width="8" stroke-dasharray="282.7" stroke-dashoffset="282.7" class="gauge-ring"></circle></svg><div class="absolute inset-0 flex flex-col items-center justify-center"><span id="gauge-soc-text" class="text-4xl font-black">0%</span></div></div></div>
                <div class="lg:col-span-2 bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-6 rounded-xl shadow-sm"><div class="flex justify-between items-center mb-4"><h3 class="font-bold text-lg">Live Flow</h3><span class="text-xs bg-green-100 text-green-700 px-2 py-1 rounded-full">Live</span></div><div class="h-64 w-full"><canvas id="liveChart"></canvas></div></div>
            </div>
            <div class="bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-6 rounded-2xl shadow-sm">
                <div class="flex justify-between items-center mb-6"><h3 class="font-bold text-lg text-gray-800 dark:text-gray-100">Live Telemetry</h3><span class="bg-blue-100 text-blue-800 text-xs font-medium px-2.5 py-0.5 rounded dark:bg-blue-900 dark:text-blue-300">Modbus RTU</span></div>
                <div class="grid grid-cols-2 md:grid-cols-4 gap-4">
                    <div class="p-4 bg-gray-50 dark:bg-slate-800/50 rounded-xl border border-gray-100 dark:border-slate-700/50 has-tooltip" title="Panel Voltage"><p class="text-xs text-gray-500 dark:text-gray-400 mb-1">Panel Voltage</p><p class="text-2xl font-bold text-gray-800 dark:text-white"><span id="detail-pv-volt">0.0</span> <span class="text-sm font-normal text-gray-500">V</span></p></div>
                    <div class="p-4 bg-gray-50 dark:bg-slate-800/50 rounded-xl border border-gray-100 dark:border-slate-700/50 has-tooltip" title="Charge Current"><p class="text-xs text-gray-500 dark:text-gray-400 mb-1">Charge Current</p><p class="text-2xl font-bold text-gray-800 dark:text-white"><span id="detail-chg-amp">0.0</span> <span class="text-sm font-normal text-gray-500">A</span></p></div>
                    <div class="p-4 bg-gray-50 dark:bg-slate-800/50 rounded-xl border border-gray-100 dark:border-slate-700/50 has-tooltip" title="Device Temp"><p class="text-xs text-gray-500 dark:text-gray-400 mb-1">Device Temp</p><p class="text-2xl font-bold text-gray-800 dark:text-white"><span id="detail-temp">0.0</span> <span class="text-sm font-normal text-gray-500">°C</span></p></div>
                    <div class="p-4 bg-gray-50 dark:bg-slate-800/50 rounded-xl border border-gray-100 dark:border-slate-700/50 has-tooltip" title="Daily Energy"><p class="text-xs text-gray-500 dark:text-gray-400 mb-1">Daily Energy</p><p class="text-2xl font-bold text-green-600 dark:text-green-400"><span id="detail-daily-energy">0.00</span> <span class="text-sm font-normal text-gray-500">kWh</span></p></div>
                </div>
            </div>
        </div>

        <div id="view-history" class="hidden-view fade-in space-y-6">
            <div class="flex flex-col md:flex-row justify-between items-center bg-white dark:bg-slate-900 p-4 rounded-2xl border border-gray-100 dark:border-slate-800 shadow-sm gap-4">
                <div><h2 class="text-xl font-bold">History Log</h2><p class="text-gray-500 text-xs">Select date to view logs.</p></div>
                <div class="flex gap-2 items-center">
                    <input type="date" id="logDateSelector" class="bg-gray-50 dark:bg-slate-800 border border-gray-200 dark:border-slate-700 text-gray-900 dark:text-white text-sm rounded-lg block p-2.5">
                    <button onclick="fetchDayLog()" class="bg-solar-500 hover:bg-solar-600 text-white font-medium rounded-lg text-sm px-4 py-2.5" title="Load data"><i data-lucide="search" class="w-4 h-4"></i></button>
                    <button onclick="downloadLog()" class="bg-white dark:bg-slate-800 border border-gray-200 dark:border-slate-700 hover:bg-gray-50 dark:hover:bg-slate-700 text-gray-700 dark:text-gray-200 font-medium rounded-lg text-sm px-4 py-2.5 flex items-center gap-2" title="Download CSV"><i data-lucide="download" class="w-4 h-4"></i> <span class="hidden md:inline">CSV</span></button>
                </div>
            </div>
            <div class="bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 p-6 rounded-2xl shadow-sm"><h3 class="font-bold text-lg mb-4">Intraday PV Power (Watts)</h3><div class="h-72 w-full relative"><canvas id="dayChart"></canvas><div id="no-data-msg" class="absolute inset-0 flex items-center justify-center text-gray-400 hidden">No data</div></div></div>
            <div class="bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 rounded-2xl shadow-sm overflow-hidden">
                <div class="p-6 border-b border-gray-100 dark:border-slate-800"><h3 class="font-bold text-lg">Detailed Log <span id="detail-date-title" class="text-sm font-normal text-gray-500 ml-2"></span></h3></div>
                <div class="max-h-64 overflow-y-auto"><table class="w-full text-left text-sm"><thead class="bg-gray-50 dark:bg-slate-800/50 text-gray-500 uppercase text-xs sticky top-0"><tr><th class="px-6 py-3">Time</th><th class="px-6 py-3">Status</th><th class="px-6 py-3 text-right">Power</th><th class="px-6 py-3 text-right">Bat</th><th class="px-6 py-3 text-right">SOC</th></tr></thead><tbody id="detail-table-body" class="divide-y divide-gray-100 dark:divide-slate-800"><tr><td colspan="5" class="p-4 text-center text-gray-400">Select date</td></tr></tbody></table></div>
            </div>
            <div class="bg-white dark:bg-slate-900 border border-gray-100 dark:border-slate-800 rounded-2xl shadow-sm overflow-hidden">
                <div class="p-6 border-b border-gray-100 dark:border-slate-800 flex justify-between items-center"><h3 class="font-bold text-lg">Monthly Summary</h3><button onclick="fetchSummary()" class="text-sm text-solar-500 font-bold" title="Reload summary">Refresh</button></div>
                <div class="max-h-64 overflow-y-auto"><table class="w-full text-left text-sm"><thead class="bg-gray-50 dark:bg-slate-800/50 text-gray-500 uppercase text-xs sticky top-0"><tr><th class="px-6 py-3">Date</th><th class="px-6 py-3 text-right">Yield (kWh)</th></tr></thead><tbody id="summary-table-body" class="divide-y divide-gray-100 dark:divide-slate-800"><tr><td colspan="2" class="p-4 text-center text-gray-400">Loading...</td></tr></tbody></table></div>
            </div>
        </div>

        <div id="view-settings" class="hidden-view fade-in space-y-6">
            <div class="bg-white dark:bg-slate-900 p-6 rounded-2xl border border-gray-100 dark:border-slate-800 shadow-sm">
                <div class="flex justify-between items-center mb-6"><div><h2 class="text-xl font-bold">Control Parameters</h2><p class="text-gray-500 text-sm">Configure controller voltages and durations.</p></div><button onclick="fetchSettings()" class="text-sm text-solar-500 font-bold flex gap-1 items-center" title="Reload"><i data-lucide="refresh-cw" class="w-4 h-4"></i> Reload</button></div>
                <div class="grid grid-cols-1 lg:grid-cols-2 gap-8">
                    <div class="space-y-4">
                        <h3 class="font-bold text-gray-400 text-xs uppercase tracking-wider border-b pb-2">General</h3>
                        <div class="grid grid-cols-2 gap-4">
                            <div class="has-tooltip" title="Battery chemistry"><label class="text-xs font-semibold">Bat Type</label><select id="set-type" onchange="updateFormState()" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"><option value="0">User</option><option value="1">Sealed</option><option value="2">Gel</option><option value="3">Flooded</option><option value="4">LiFePO4</option></select></div>
                            <div class="has-tooltip" title="Capacity in Ah"><label class="text-xs font-semibold">Capacity (Ah)</label><input type="number" id="set-cap" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                        </div>
                        <div class="grid grid-cols-2 gap-4">
                             <div class="has-tooltip" title="Temp coeff (mV/°C/2V)"><label class="text-xs font-semibold">Temp Comp</label><input type="number" id="set-tcomp" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                             <div class="has-tooltip" title="System Voltage"><label class="text-xs font-semibold">Rated Voltage</label><select id="set-rated-volt" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"><option value="0">Auto</option><option value="1">12V</option><option value="2">24V</option></select></div>
                        </div>
                        <h3 class="font-bold text-gray-400 text-xs uppercase tracking-wider border-b pb-2 mt-4">High Voltage Protection</h3>
                        <div class="space-y-3">
                            <div class="has-tooltip" title="OVD"><label class="text-xs block mb-1">Over Volt. Disconnect (V)</label><input type="number" step="0.1" id="set-ovd" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="CLV"><label class="text-xs block mb-1">Charging Limit (V)</label><input type="number" step="0.1" id="set-clv" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="OVR"><label class="text-xs block mb-1">Over Volt. Reconnect (V)</label><input type="number" step="0.1" id="set-ovr" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                        </div>
                    </div>
                    <div class="space-y-4">
                        <h3 class="font-bold text-gray-400 text-xs uppercase tracking-wider border-b pb-2">Charging Voltages</h3>
                        <div class="grid grid-cols-2 gap-4">
                            <div class="has-tooltip" title="EQV"><label class="text-xs block mb-1">Equalize (V)</label><input type="number" step="0.1" id="set-eqv" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="BST"><label class="text-xs block mb-1">Boost (V)</label><input type="number" step="0.1" id="set-bst" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="FLT"><label class="text-xs block mb-1">Float (V)</label><input type="number" step="0.1" id="set-flt" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="BSR"><label class="text-xs block mb-1">Boost Recon. (V)</label><input type="number" step="0.1" id="set-bsr" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                        </div>
                        <h3 class="font-bold text-gray-400 text-xs uppercase tracking-wider border-b pb-2 mt-4">Low Voltage & Limits</h3>
                        <div class="grid grid-cols-2 gap-4">
                            <div class="has-tooltip" title="LVR"><label class="text-xs block mb-1">Low Volt Recon (V)</label><input type="number" step="0.1" id="set-lvr" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="UVR"><label class="text-xs block mb-1">Under Volt Rec (V)</label><input type="number" step="0.1" id="set-uvr" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="UVW"><label class="text-xs block mb-1">Under Volt Warn (V)</label><input type="number" step="0.1" id="set-uvw" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="LVD"><label class="text-xs block mb-1">Low Volt Disc (V)</label><input type="number" step="0.1" id="set-lvd" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="DLV"><label class="text-xs block mb-1">Discharge Limit (V)</label><input type="number" step="0.1" id="set-dlv" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                        </div>
                        <h3 class="font-bold text-gray-400 text-xs uppercase tracking-wider border-b pb-2 mt-4">Duration (Minutes)</h3>
                        <div class="grid grid-cols-2 gap-4">
                            <div class="has-tooltip" title="Eq Duration"><label class="text-xs block mb-1">Equalize Time</label><input type="number" id="set-eqt" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                            <div class="has-tooltip" title="Boost Duration"><label class="text-xs block mb-1">Boost Time</label><input type="number" id="set-bstt" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border rounded text-xs"></div>
                        </div>
                    </div>
                </div>
                <div class="mt-8 pt-4 border-t border-gray-200 dark:border-slate-700">
                    <button onclick="saveSettings()" class="w-full text-white bg-solar-600 hover:bg-solar-700 font-bold rounded-lg text-sm px-5 py-3 text-center flex justify-center items-center gap-2"><i data-lucide="save" class="w-5 h-5"></i> Update Parameters</button>
                    <p class="text-center text-xs text-gray-400 mt-2">PIN Required. Improper voltages can damage batteries.</p>
                </div>
            </div>

            <div class="bg-white dark:bg-slate-900 p-6 rounded-2xl border border-gray-100 dark:border-slate-800 shadow-sm">
                <div class="flex justify-between items-center mb-6"><div><h2 class="text-xl font-bold">WiFi Configuration</h2><p class="text-gray-500 text-sm">Manage connectivity.</p></div><button onclick="fetchWiFiStatus()" class="text-sm text-solar-500 font-bold flex gap-1 items-center" title="Refresh"><i data-lucide="refresh-cw" class="w-4 h-4"></i> Refresh</button></div>
                <div id="wifi-status-container" class="mb-6 p-4 bg-gray-50 dark:bg-slate-800 rounded-lg border border-gray-200 dark:border-slate-700">
                    <div class="flex items-center gap-2 mb-2"><span id="wifi-status-dot" class="inline-block w-3 h-3 rounded-full bg-gray-300"></span><span id="wifi-status-text" class="font-semibold">Checking...</span></div>
                    <p id="wifi-status-details" class="text-xs text-gray-600 dark:text-gray-400"></p>
                </div>
                <div class="grid grid-cols-1 lg:grid-cols-2 gap-6">
                    <div class="space-y-4">
                        <h3 class="font-bold text-gray-400 text-xs uppercase tracking-wider border-b pb-2">Manual Connection</h3>
                        <div><label class="text-xs font-semibold block mb-1">SSID</label><input type="text" id="wifi-ssid" placeholder="Network name" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border border-gray-200 dark:border-slate-700 rounded text-xs"></div>
                        <div><label class="text-xs font-semibold block mb-1">Password</label><input type="password" id="wifi-password" placeholder="Password" class="w-full p-2 bg-gray-50 dark:bg-slate-800 border border-gray-200 dark:border-slate-700 rounded text-xs"></div>
                        <button onclick="connectToWiFi()" class="w-full text-white bg-blue-600 hover:bg-blue-700 font-bold rounded-lg text-sm px-4 py-2 flex justify-center items-center gap-2"><i data-lucide="wifi" class="w-4 h-4"></i> Connect</button>
                        <p id="wifi-connect-status" class="text-xs text-center text-gray-500"></p>
                    </div>
                    <div class="space-y-4">
                        <div class="flex justify-between items-center border-b pb-2"><h3 class="font-bold text-gray-400 text-xs uppercase tracking-wider">Available Networks</h3><button onclick="scanWiFiNetworks()" class="text-xs text-solar-500 font-bold flex gap-1 items-center" title="Scan"><i data-lucide="search" class="w-3 h-3"></i> Scan</button></div>
                        <div id="wifi-networks-container" class="space-y-2 max-h-48 overflow-y-auto"><p class="text-xs text-gray-400">Click "Scan" to search...</p></div>
                    </div>
                </div>
            </div>
        </div>

        <div id="view-firmware" class="hidden-view fade-in space-y-6">
            <div class="bg-white dark:bg-slate-900 p-6 rounded-2xl border border-gray-100 dark:border-slate-800 shadow-sm">
                <div class="flex justify-between items-center mb-6"><div><h2 class="text-xl font-bold">Firmware Update</h2><p class="text-gray-500 text-sm">Update device OTA.</p></div><button onclick="fetchFirmwareInfo()" class="text-sm text-solar-500 font-bold flex gap-1 items-center" title="Refresh"><i data-lucide="refresh-cw" class="w-4 h-4"></i> Refresh</button></div>
                <div class="grid grid-cols-1 lg:grid-cols-3 gap-6">
                    <div class="space-y-4">
                        <h3 class="font-bold text-gray-400 text-xs uppercase tracking-wider border-b pb-2">System Info</h3>
                        <div class="space-y-3 text-sm">
                            <div class="flex justify-between"><span class="text-gray-500">Version:</span><span id="fw-version" class="font-semibold">--</span></div>
                            <div class="flex justify-between"><span class="text-gray-500">Date:</span><span id="fw-date" class="font-semibold">--</span></div>
                            <div class="flex justify-between"><span class="text-gray-500">Chip:</span><span id="fw-chip" class="font-semibold text-xs">--</span></div>
                            <div class="flex justify-between"><span class="text-gray-500">Free Flash:</span><span id="fw-free" class="font-semibold text-xs">--</span></div>
                        </div>
                    </div>
                    <div class="lg:col-span-2 space-y-4">
                        <h3 class="font-bold text-gray-400 text-xs uppercase tracking-wider border-b pb-2">Upload Firmware</h3>
                        <div class="p-4 bg-blue-50 dark:bg-blue-950 border border-blue-200 dark:border-blue-800 rounded-lg text-xs text-blue-700 dark:text-blue-300"><p><strong>Instructions:</strong></p><ol class="list-decimal list-inside mt-2 space-y-1"><li>Build project in Arduino IDE</li><li>Sketch → Export compiled Binary (.bin)</li><li>Select file below and upload</li></ol></div>
                        <div class="border-2 border-dashed border-gray-300 dark:border-slate-700 rounded-lg p-6 text-center hover:bg-gray-50 dark:hover:bg-slate-800 transition-colors cursor-pointer" id="upload-dropzone"><i data-lucide="cloud-upload" class="w-8 h-8 mx-auto text-gray-400 mb-2"></i><p class="text-sm font-semibold mb-1">Drag and drop .bin file</p><p class="text-xs text-gray-500">or click to browse</p><input type="file" id="fw-file-input" accept=".bin" class="hidden" onchange="handleFileSelect(event)"></div>
                        <div id="fw-upload-status" class="p-3 bg-gray-50 dark:bg-slate-800 rounded-lg text-sm hidden"><div class="flex justify-between mb-2"><span>Upload Progress:</span><span id="fw-progress-text">0%</span></div><div class="w-full bg-gray-200 dark:bg-slate-700 h-2 rounded-full overflow-hidden"><div id="fw-progress-bar" class="bg-solar-500 h-full" style="width: 0%"></div></div></div>
                        <button id="fw-upload-btn" onclick="uploadFirmware()" class="w-full text-white bg-solar-600 hover:bg-solar-700 font-bold rounded-lg text-sm px-4 py-3 flex justify-center items-center gap-2 disabled:opacity-50 disabled:cursor-not-allowed"><i data-lucide="upload-cloud" class="w-4 h-4"></i> Upload Firmware</button>
                        <p id="fw-message" class="text-xs text-center text-gray-500"></p>
                    </div>
                </div>
            </div>
        </div>
    </div>
<script>
    var gateway = "ws://" + window.location.hostname + "/ws";
    var websocket;
    let liveChartInstance = null; let dayChartInstance = null;
    let livePVData = Array(30).fill(0); let liveLabels = Array(30).fill('');

    window.addEventListener('load', () => {
        if(localStorage.getItem('theme') === 'dark') toggleTheme();
        initCharts(); lucide.createIcons(); initWebSocket();
        document.getElementById('logDateSelector').valueAsDate = new Date();
        setTimeout(fetchSummary, 1000);
    });

    async function fetchSettings() {
        try {
            const res = await fetch('/api/settings');
            const d = await res.json();
            document.getElementById('set-type').value = d.type;
            document.getElementById('set-cap').value = d.cap;
            document.getElementById('set-tcomp').value = d.tcomp;
            document.getElementById('set-rated-volt').value = d.ratedVolt;
            document.getElementById('set-ovd').value = d.ovd.toFixed(1);
            document.getElementById('set-clv').value = d.clv.toFixed(1);
            document.getElementById('set-ovr').value = d.ovr.toFixed(1);
            document.getElementById('set-eqv').value = d.eqv.toFixed(1);
            document.getElementById('set-bst').value = d.bst.toFixed(1);
            document.getElementById('set-flt').value = d.flt.toFixed(1);
            document.getElementById('set-bsr').value = d.bsr.toFixed(1);
            document.getElementById('set-lvr').value = d.lvr.toFixed(1);
            document.getElementById('set-uvr').value = d.uvr.toFixed(1);
            document.getElementById('set-uvw').value = d.uvw.toFixed(1);
            document.getElementById('set-lvd').value = d.lvd.toFixed(1);
            document.getElementById('set-dlv').value = d.dlv.toFixed(1);
            document.getElementById('set-eqt').value = d.eqt;
            document.getElementById('set-bstt').value = d.bstt;
            updateFormState();
        } catch(e) { console.error(e); alert("Failed to load settings"); }
    }

    function updateFormState() {
        const batType = parseInt(document.getElementById('set-type').value);
        const isUser = (batType === 0);
        const voltageOnlyIds = ['set-ovd', 'set-clv', 'set-ovr', 'set-eqv', 'set-bst', 'set-flt', 'set-bsr', 'set-lvr', 'set-uvr', 'set-uvw', 'set-lvd', 'set-dlv'];
        const alwaysEditableIds = ['set-cap', 'set-tcomp', 'set-rated-volt', 'set-eqt', 'set-bstt'];
        voltageOnlyIds.forEach(id => {
            const el = document.getElementById(id);
            if(el) { el.disabled = !isUser; if (!isUser) el.classList.add('bg-gray-200', 'dark:bg-slate-700', 'opacity-50'); else el.classList.remove('bg-gray-200', 'dark:bg-slate-700', 'opacity-50'); }
        });
        alwaysEditableIds.forEach(id => { const el = document.getElementById(id); if(el) { el.disabled = false; el.classList.remove('bg-gray-200', 'dark:bg-slate-700', 'opacity-50'); } });
    }

    async function saveSettings() {
        const pin = prompt("Enter Administrator PIN to confirm changes:");
        if (pin === null) return;
        const batType = parseInt(document.getElementById('set-type').value);
        const isUser = (batType === 0);
        const p = { pin: pin, type: batType, cap: parseInt(document.getElementById('set-cap').value), tcomp: parseInt(document.getElementById('set-tcomp').value), ratedVolt: parseInt(document.getElementById('set-rated-volt').value), eqt: parseInt(document.getElementById('set-eqt').value), bstt: parseInt(document.getElementById('set-bstt').value) };
        if (isUser) {
             p.ovd = parseFloat(document.getElementById('set-ovd').value); p.clv = parseFloat(document.getElementById('set-clv').value); p.ovr = parseFloat(document.getElementById('set-ovr').value); p.eqv = parseFloat(document.getElementById('set-eqv').value); p.bst = parseFloat(document.getElementById('set-bst').value); p.flt = parseFloat(document.getElementById('set-flt').value); p.bsr = parseFloat(document.getElementById('set-bsr').value); p.lvr = parseFloat(document.getElementById('set-lvr').value); p.uvr = parseFloat(document.getElementById('set-uvr').value); p.uvw = parseFloat(document.getElementById('set-uvw').value); p.lvd = parseFloat(document.getElementById('set-lvd').value); p.dlv = parseFloat(document.getElementById('set-dlv').value);
        }
        try {
            const res = await fetch('/api/settings', { method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify(p) });
            if(res.ok) alert("Update Started! Check Serial Monitor."); else if (res.status === 401) alert("Incorrect PIN!"); else if (res.status === 429) alert("Busy, please wait."); else alert("Save Failed");
        } catch(e) { alert("Connection Error"); }
    }

    function switchView(id) {
        document.getElementById('view-dashboard').className = id==='dashboard'?'fade-in space-y-6':'hidden-view';
        document.getElementById('view-history').className = id==='history'?'fade-in space-y-6':'hidden-view';
        document.getElementById('view-settings').className = id==='settings'?'fade-in space-y-6':'hidden-view';
        document.getElementById('view-firmware').className = id==='firmware'?'fade-in space-y-6':'hidden-view';
        if(id==='settings') fetchSettings();
        if(id==='firmware') fetchFirmwareInfo();
        const inactive="text-gray-600 dark:text-gray-300", active="bg-solar-500 text-white shadow-md";
        ['dashboard','history','settings','firmware'].forEach(v => document.getElementById('btn-'+v).className = "px-4 py-2 rounded-md text-sm font-medium transition-all flex gap-2 items-center " + (id===v?active:inactive));
    }
    
    function downloadLog() { const dateVal = document.getElementById('logDateSelector').value; if(!dateVal) { alert("Please select a date first."); return; } window.location.href = `/api/day-log?date=${dateVal}`; }
    async function fetchDayLog() {
        const dateVal = document.getElementById('logDateSelector').value; if(!dateVal) return;
        document.getElementById('detail-date-title').textContent = `(${dateVal})`;
        try {
            const response = await fetch(`/api/day-log?date=${dateVal}`);
            if(response.status === 404) { resetDayView(true); return; }
            const csvText = await response.text();
            resetDayView(false);
            const rows = csvText.trim().split('\n');
            const labels = [], powerData = [], voltData = [];
            const tbody = document.getElementById('detail-table-body'); tbody.innerHTML = "";
            rows.forEach(row => {
                const cols = row.split(',');
                if(cols.length >= 3) {
                    labels.push(cols[0]); powerData.push(parseFloat(cols[1])); voltData.push(parseFloat(cols[2])); 
                    const statusRaw = cols.length >= 5 ? parseInt(cols[4]) : 0;
                    const status = decodeStatus(statusRaw);
                    const soc = cols.length >= 4 ? cols[3] : "-";
                    const tr = `<tr class="hover:bg-gray-50 dark:hover:bg-slate-800"><td class="px-6 py-2 font-mono text-xs">${cols[0]}</td><td class="px-6 py-2"><span class="text-xs font-bold ${status.color}">${status.text}</span></td><td class="px-6 py-2 text-right font-mono">${parseFloat(cols[1]).toFixed(1)}</td><td class="px-6 py-2 text-right font-mono text-xs">${parseFloat(cols[2]).toFixed(1)}</td><td class="px-6 py-2 text-right font-mono text-xs">${soc}</td></tr>`;
                    tbody.insertAdjacentHTML('afterbegin', tr);
                }
            });
            dayChartInstance.data.labels = labels; dayChartInstance.data.datasets[0].data = powerData; dayChartInstance.data.datasets[1].data = voltData; dayChartInstance.update();
        } catch (e) { console.error(e); }
    }
    function resetDayView(e) {
        if(e){ document.getElementById('no-data-msg').classList.remove('hidden'); document.getElementById('detail-table-body').innerHTML='<tr><td colspan="5" class="p-4 text-center text-gray-400">No log found</td></tr>'; dayChartInstance.data.labels=[]; dayChartInstance.data.datasets.forEach(d=>d.data=[]); }
        else { document.getElementById('no-data-msg').classList.add('hidden'); }
        dayChartInstance.update();
    }
    async function fetchSummary() {
        try {
            const response = await fetch('/api/summary'); const data = await response.json();
            const tbody = document.getElementById('summary-table-body'); tbody.innerHTML = "";
            Object.keys(data).sort().reverse().forEach(date => {
                const val = data[date].y || data[date];
                const row = `<tr class="hover:bg-gray-50 dark:hover:bg-slate-800"><td class="px-6 py-3">${date}</td><td class="px-6 py-3 text-right font-bold">${typeof val === 'number' ? val.toFixed(2) : '0.00'}</td></tr>`;
                tbody.innerHTML += row;
            });
        } catch(e) {}
    }
    function initCharts() {
        const ctxLive = document.getElementById('liveChart');
        if(ctxLive) liveChartInstance = new Chart(ctxLive, { type: 'line', data: { labels: liveLabels, datasets: [{ label: 'Power (W)', data: livePVData, borderColor: '#0ea5e9', backgroundColor: '#0ea5e933', fill: true, tension: 0.4, pointRadius: 0 }] }, options: { responsive: true, maintainAspectRatio: false, plugins: { legend: {display:false}, tooltip: {enabled: true, mode: 'index', intersect: false} }, scales: { x: { display: false }, y: { beginAtZero: true, grid: { color: '#33415520' } } } } });
        const ctxDay = document.getElementById('dayChart');
        if(ctxDay) dayChartInstance = new Chart(ctxDay, { type: 'line', data: { labels: [], datasets: [{ label: 'PV Power (W)', data: [], borderColor: '#f97316', backgroundColor: '#f9731622', fill: true, pointRadius: 0, yAxisID: 'y' }, { label: 'Bat Volt (V)', data: [], borderColor: '#22c55e', borderDash: [5, 5], pointRadius: 0, yAxisID: 'y1' }] }, options: { responsive: true, maintainAspectRatio: false, interaction: { mode: 'index', intersect: false }, plugins: { tooltip: {enabled: true, callbacks: {label: function(context) { let label = context.dataset.label || ''; if (label) { label += ': '; } if (context.parsed.y !== null) { label += context.parsed.y + (context.dataset.yAxisID === 'y' ? ' W' : ' V'); } return label; }}} }, scales: { x: { grid: { display: false } }, y: { type: 'linear', display: true, position: 'left', title: {display:true, text:'Watts'} }, y1: { type: 'linear', display: true, position: 'right', grid: {drawOnChartArea: false}, title: {display:true, text:'Volts'} } } } });
    }
    function initWebSocket() {
        websocket = new WebSocket(gateway);
        websocket.onopen = () => { document.getElementById('connection-status').textContent="Connected"; document.getElementById('connection-status').className="text-solar-500 font-bold"; document.getElementById('conn-dot').className="inline-block w-2 h-2 rounded-full bg-solar-500 animate-pulse"; };
        websocket.onclose = () => { document.getElementById('connection-status').textContent="Disconnected"; document.getElementById('connection-status').className="text-red-500"; document.getElementById('conn-dot').className="inline-block w-2 h-2 rounded-full bg-red-500"; setTimeout(initWebSocket, 2000); };
        websocket.onmessage = (e) => {
            const data = JSON.parse(e.data); if(data.type!=='live'||!data.valid)return;
            document.getElementById('panel-power').textContent=Math.floor(data.pvPower);
            document.getElementById('pv-bar').style.width=Math.min((data.pvPower/500)*100,100)+"%";
            document.getElementById('battery-voltage').textContent=data.battVolt.toFixed(2);
            document.getElementById('load-power').textContent=Math.floor(data.loadPower);
            document.getElementById('detail-pv-volt').textContent=data.pvVolt.toFixed(1);
            document.getElementById('detail-chg-amp').textContent=data.chgAmp.toFixed(1);
            document.getElementById('detail-temp').textContent=data.temp.toFixed(1);
            document.getElementById('detail-daily-energy').textContent=data.dailyKwh?data.dailyKwh.toFixed(2):"0.00";
            const st = decodeStatus(data.chgStatus);
            const stEl = document.getElementById('charging-stage-display'); stEl.textContent=st.text; stEl.className="text-lg font-bold mt-1 "+st.color;
            document.getElementById('gauge-soc-text').textContent=Math.round(data.soc)+"%";
            const offset=282.7-((data.soc/100)*282.7);
            document.querySelector('.gauge-ring').style.strokeDashoffset=offset;
            document.querySelector('.gauge-ring').style.stroke = data.soc<30?"#ef4444":(data.soc<60?"#eab308":"#22c55e");
            if(liveChartInstance){ livePVData.push(data.pvPower); livePVData.shift(); liveChartInstance.update('none'); }
        };
    }
    function decodeStatus(val) {
        const chg=(val>>2)&0x03, voltStatus=(val>>14)&0x03; 
        if(voltStatus===1)return{text:"OVP",color:"text-red-500"}; if(val&0x8000)return{text:"Fault",color:"text-red-600"};
        switch(chg){case 0:return{text:"Standby",color:"text-gray-400"};case 1:return{text:"Float",color:"text-solar-500"};case 2:return{text:"Boost",color:"text-blue-500"};case 3:return{text:"Equalize",color:"text-orange-500"};default:return{text:"Active",color:"text-green-500"};}
    }
    function toggleTheme() {
        document.documentElement.classList.toggle('dark');
        localStorage.setItem('theme', document.documentElement.classList.contains('dark') ? 'dark' : 'light');
        document.getElementById('theme-icon').setAttribute('data-lucide', document.documentElement.classList.contains('dark') ? 'sun' : 'moon');
        lucide.createIcons();
    }
    async function fetchWiFiStatus() {
        try {
            const res = await fetch('/api/wifi-status');
            const data = await res.json();
            const statusDot = document.getElementById('wifi-status-dot');
            const statusText = document.getElementById('wifi-status-text');
            const statusDetails = document.getElementById('wifi-status-details');
            if (data.wifiConnected) {
                statusDot.className = 'inline-block w-3 h-3 rounded-full bg-green-500 animate-pulse';
                statusText.textContent = '✓ WiFi Connected';
                statusText.className = 'font-semibold text-green-600 dark:text-green-400';
                statusDetails.textContent = `SSID: ${data.ssid} | IP: ${data.ip} | Signal: ${data.rssi} dBm`;
            } else if (data.isAPMode) {
                statusDot.className = 'inline-block w-3 h-3 rounded-full bg-yellow-500 animate-pulse';
                statusText.textContent = '⚠ AP Mode (No WiFi)';
                statusText.className = 'font-semibold text-yellow-600 dark:text-yellow-400';
                statusDetails.textContent = `Access Point: ${data.apSSID} | IP: ${data.apIP}`;
            } else {
                statusDot.className = 'inline-block w-3 h-3 rounded-full bg-red-500';
                statusText.textContent = '✗ Disconnected';
                statusText.className = 'font-semibold text-red-600 dark:text-red-400';
                statusDetails.textContent = 'No connection established';
            }
        } catch(e) { console.error('WiFi status error:', e); }
    }
    async function scanWiFiNetworks() {
        const container = document.getElementById('wifi-networks-container');
        container.innerHTML = '<p class="text-xs text-gray-400">Scanning...</p>';
        try {
            const res = await fetch('/api/wifi-scan');
            const data = await res.json();
            if (!data.networks || data.networks.length === 0) { container.innerHTML = '<p class="text-xs text-gray-400">No networks found</p>'; return; }
            container.innerHTML = '';
            data.networks.forEach(net => {
                const signal = net.rssi > -50 ? 'Excellent' : net.rssi > -70 ? 'Good' : 'Weak';
                const lockIcon = net.secure ? '🔒' : '🔓';
                const item = document.createElement('div');
                item.className = 'p-2 bg-white dark:bg-slate-700 border border-gray-200 dark:border-slate-600 rounded cursor-pointer hover:bg-gray-100 dark:hover:bg-slate-600 transition-colors';
                item.innerHTML = `<div class="flex justify-between items-start"><div><div class="text-xs font-semibold">${lockIcon} ${net.ssid}</div><div class="text-xs text-gray-500">${signal} (${net.rssi} dBm)</div></div></div>`;
                item.onclick = () => { document.getElementById('wifi-ssid').value = net.ssid; document.getElementById('wifi-password').focus(); };
                container.appendChild(item);
            });
        } catch(e) { console.error('Scan error:', e); container.innerHTML = '<p class="text-xs text-red-500">Scan failed</p>'; }
    }
    async function connectToWiFi() {
        const ssid = document.getElementById('wifi-ssid').value.trim();
        const password = document.getElementById('wifi-password').value;
        const statusEl = document.getElementById('wifi-connect-status');
        if (!ssid) { statusEl.textContent = 'SSID required'; statusEl.className = 'text-xs text-center text-red-500'; return; }
        statusEl.textContent = 'Connecting...'; statusEl.className = 'text-xs text-center text-blue-500';
        try {
            const res = await fetch('/api/wifi-connect', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ ssid, password }) });
            if (res.ok) { statusEl.textContent = 'Connecting...'; setTimeout(() => { fetchWiFiStatus(); statusEl.textContent = ''; }, 5000); } 
            else { const msg = await res.text(); statusEl.textContent = `Error: ${msg}`; statusEl.className = 'text-xs text-center text-red-500'; }
        } catch(e) { statusEl.textContent = 'Connection failed'; statusEl.className = 'text-xs text-center text-red-500'; }
    }
    window.addEventListener('load', () => { fetchWiFiStatus(); setTimeout(() => fetchWiFiStatus(), 2000); });
    
    // --- FIRMWARE ---
    let selectedFile = null;
    async function fetchFirmwareInfo() {
        try {
            const res = await fetch('/api/firmware-info'); const data = await res.json();
            document.getElementById('fw-version').textContent = data.version || '--';
            document.getElementById('fw-date').textContent = data.date || '--';
            document.getElementById('fw-chip').textContent = `${data.chipModel} (Rev ${data.chipRevision})`;
            document.getElementById('fw-free').textContent = `${(data.freeSketch / 1024 / 1024).toFixed(2)} MB`;
        } catch(e) {}
    }
    document.getElementById('upload-dropzone')?.addEventListener('click', () => { document.getElementById('fw-file-input').click(); });
    document.getElementById('upload-dropzone')?.addEventListener('dragover', (e) => { e.preventDefault(); document.getElementById('upload-dropzone').classList.add('bg-gray-100', 'dark:bg-slate-700'); });
    document.getElementById('upload-dropzone')?.addEventListener('dragleave', () => { document.getElementById('upload-dropzone').classList.remove('bg-gray-100', 'dark:bg-slate-700'); });
    document.getElementById('upload-dropzone')?.addEventListener('drop', (e) => { e.preventDefault(); document.getElementById('upload-dropzone').classList.remove('bg-gray-100', 'dark:bg-slate-700'); if (e.dataTransfer.files.length > 0) handleFileSelect({target:{files:e.dataTransfer.files}}); });
    function handleFileSelect(event) {
        const file = event.target.files[0];
        if (file && file.name.endsWith('.bin')) { selectedFile = file; document.getElementById('fw-message').textContent = `Selected: ${file.name}`; document.getElementById('fw-message').className = 'text-xs text-center text-green-600 dark:text-green-400'; } 
        else { document.getElementById('fw-message').textContent = 'Please select a .bin file'; document.getElementById('fw-message').className = 'text-xs text-center text-red-600'; selectedFile = null; }
    }
    async function uploadFirmware() {
        if (!selectedFile) { document.getElementById('fw-message').textContent = 'Select file first'; return; }
        const btn = document.getElementById('fw-upload-btn'); const status = document.getElementById('fw-upload-status'); const msg = document.getElementById('fw-message');
        btn.disabled = true; status.classList.remove('hidden'); msg.textContent = 'Uploading...'; msg.className = 'text-xs text-center text-blue-600';
        try {
            const formData = new FormData(); formData.append('firmware', selectedFile);
            const xhr = new XMLHttpRequest();
            xhr.upload.addEventListener('progress', (e) => { if (e.lengthComputable) { const pct = (e.loaded / e.total) * 100; document.getElementById('fw-progress-bar').style.width = pct + '%'; document.getElementById('fw-progress-text').textContent = Math.round(pct) + '%'; } });
            xhr.addEventListener('load', () => {
                if (xhr.status === 200) { msg.textContent = 'Success! Restarting...'; msg.className = 'text-xs text-center text-green-600'; selectedFile = null; document.getElementById('fw-file-input').value = ''; setTimeout(() => { status.classList.add('hidden'); btn.disabled = false; msg.textContent = ''; location.reload(); }, 5000); } 
                else { msg.textContent = 'Failed: ' + xhr.responseText; msg.className = 'text-xs text-center text-red-600'; btn.disabled = false; }
            });
            xhr.addEventListener('error', () => { msg.textContent = 'Upload error'; btn.disabled = false; });
            xhr.open('POST', '/api/firmware-update'); xhr.send(formData);
        } catch(e) { msg.textContent = 'Error: ' + e.message; btn.disabled = false; }
    }
</script>
</body>
</html>
)rawliteral";