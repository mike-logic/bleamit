const char DASHBOARD_HTML[] PROGMEM = R"rawlite(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <title>bleamit Dashboard</title>
  <style>
    body { font-family: sans-serif; padding: 1em; background: #111; color: #eee; }
    h1 { color: #0ff; }
    table { border-collapse: collapse; width: 100%; margin-top: 1em; }
    th, td { border: 1px solid #444; padding: 0.5em; text-align: left; }
    th { background: #222; }
    button { background: #0a0; color: #fff; border: none; padding: 0.4em 1em; margin-left: 0.5em; cursor: pointer; }
    button.reject { background: #a00; }
    label, select { margin-top: 1em; display: inline-block; margin-right: 1em; }
  </style>
</head>
<body>
  <h1>bleamit Device Dashboard</h1>

  <label for="mode">Input:</label>
  <select id="mode" onchange="setMode(this.value)">
    <option value="0">Art-Net (Wi-Fi)</option>
    <option value="1">DMX (XLR input)</option>
  </select>

  <label for="output">Function:</label>
  <select id="output" onchange="setOutputMode(this.value)">
    <option value="0">Base (ESP-NOW broadcast)</option>
    <option value="1">Standalone (BLE broadcast)</option>
  </select>

  <div>
    <h2>Approved Devices</h2>
    <table id="approvedTable">
      <thead><tr><th>MAC</th><th>Type</th><th>Last Seen</th></tr></thead>
      <tbody></tbody>
    </table>
    <h2>Pending Devices</h2>
    <table id="pendingTable">
      <thead><tr><th>MAC</th><th>Action</th></tr></thead>
      <tbody></tbody>
    </table>
  </div>
  <script>
    function loadDashboard() {
      fetch('/status.json')
        .then(r => r.json())
        .then(data => {
          let approvedBody = '';
          data.approved.forEach(d => {
            let type = d.role === 1 ? 'Node' : d.role === 2 ? 'Hub' : 'Unknown';
            approvedBody += `<tr><td>${d.mac}</td><td>${type}</td><td>${d.lastSeen}s ago</td></tr>`;
          });
          document.querySelector('#approvedTable tbody').innerHTML = approvedBody;

          let pendingBody = '';
          data.pending.forEach(mac => {
            pendingBody += `<tr><td>${mac}</td><td>
              <button onclick="approve('${mac}')">Approve</button>
              <button class='reject' onclick="reject('${mac}')">Reject</button>
            </td></tr>`;
          });
          document.querySelector('#pendingTable tbody').innerHTML = pendingBody;
        });
    }
    function approve(mac) { fetch('/approve?mac=' + mac).then(loadDashboard); }
    function reject(mac) { fetch('/reject?mac=' + mac).then(loadDashboard); }
    function setMode(val) { fetch('/setmode?val=' + val).then(() => location.reload()); }
    function setOutputMode(val) { fetch('/setoutputmode?val=' + val).then(() => location.reload()); }
    function loadMode() {
      fetch('/mode').then(r => r.text()).then(val => {
        document.getElementById('mode').value = val.trim();
      });
      fetch('/outputmode').then(r => r.text()).then(val => {
        document.getElementById('output').value = val.trim();
      });
    }
    loadDashboard();
    loadMode();
    setInterval(loadDashboard, 5000);
  </script>
</body>
</html>
)rawlite";
