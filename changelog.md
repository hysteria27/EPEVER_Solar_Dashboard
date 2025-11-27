Version History & Changelog

V3.0 - New UI

Introduced "Solar Command" UI with Tailwind CSS.

Added internal temp reading and raw status.

Issue: WiFi password empty, relied on external internet.

V3.1 - Connectivity Fixes

Added WiFi AP Fallback (Hotspot EPEVER_Direct).

Added Modbus delays for stability.

V3.2 - Energy Data

Replaced raw status code with Daily Energy (kWh) reading.

V3.3 - History Page UI

Added History Page with Summary Cards, Bar Chart, and Table.

Limitation: Session-based storage only (lost on refresh).

V4.0 - Persistent Storage

Moved to LittleFS (Internal Flash).

Added NTP Time Sync.

Saves daily kWh every 10 mins.

V4.1 - Data Export

Added "Download CSV" button for Excel export.

V4.2 - Detailed History

Expanded history from single kWh value to full daily snapshot (Volt, Amp, Power).

V5.0 - Minute Logging

Switched to Minute-by-Minute Logging (CSV files per day).

Added /api/day-log endpoint.

Replaced Bar Chart with 24H Line Chart.

V5.1 - Status in Log

Added Status Code to CSV log.

Bug: UI regression hid detailed table.

V5.2 - Connection Indicator

Fixed "Connecting..." indicator bug.

V5.3 - Telemetry Restore

Restored missing "Live Telemetry" card on Dashboard.

V5.4 - History UI Restore

Restored missing "Download CSV" button and Table headers.

V6.0 - Settings (Read Only)

Added Settings Page to read parameters (Battery Type, Capacity).

V6.1 - Settings (Write)

Allowed writing parameters back to controller.

Added safety range validation.

V7.0 - Advanced Settings

Added ALL voltage protection parameters (OVD, CLV, etc.).

V7.1 - Missing Params

Added Temp Coeff and Rated Voltage.

V7.2 - UI Polish

Fixed UI regressions from V7.0 merge.

V7.3 - Cleanup

Removed Temp Coeff and Rated Voltage (per user request).

V7.4 - Final UI Fix

Fixed missing HTML elements (Telemetry card, Log tables).

V7.5 - Security

Added PIN Protection (123456) for settings changes.

V7.6 - Settings UI Sync

Updated Settings Page HTML to match full V7.x firmware capabilities.

V7.7 - Diagnostics

Added detailed Serial Logging to debug write failures.

V8.8 - Temp Comp Fix

Critical Fix: Force-writes Temp Compensation to 0 to stop internal voltage shifts causing logic errors.

V8.9 - UI Restoration

Restored Temp Coeff field to UI for visibility.

V9.0 - Voltage Lock

Force-writes Rated Voltage to 12V to prevent "Auto" mode issues.

V9.1 - Smart Direction

Automatically detects if user is raising or lowering voltages and reverses write order accordingly.

V9.2 - Diagnostic Dump

Dumps all current settings to Serial Monitor for debugging.

V9.3 - Battery Type Update

Added Li-NiCoMn (Fo4) to battery type dropdown.

V9.4 - Final Cleanup

Removed Li-NiCoMn from dropdown (per request).

Stable state with Smart Direction Write, Hybrid Protocol, and Temp Comp Fix.

V9.5 - Golden State (Final)

Archived stable firmware with full diagnostics.

Clean UI with tooltips.

V9.6 - Modular Config

Moved WiFi & PIN credentials to external config.h file for security and cleaner code management.

V9.7 - Rated Voltage Restore (Current)

Added "Rated Voltage" (Auto/12V/24V) setting back to the UI.