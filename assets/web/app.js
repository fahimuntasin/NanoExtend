(() => {
  const $ = (s, r=document) => r.querySelector(s);
  const $$ = (s, r=document) => [...r.querySelectorAll(s)];
  let csrf = localStorage.getItem('ne_csrf') || '';
  let session = localStorage.getItem('ne_session') || '';
  let connectSsid = '';
  let currentVersion = '0.0.0';
  let remoteManifest = null;
  let updateChecked = false;
  const rssiHist = Array(40).fill(null);
  const heapHist = Array(40).fill(null);

  function esc(s) {
    return String(s ?? '').replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
  }

  async function api(path, opts={}) {
    const headers = Object.assign({'Content-Type':'application/json'}, opts.headers||{});
    if (csrf && opts.method && opts.method !== 'GET') headers['X-CSRF-Token'] = csrf;
    if (session) headers['X-Session'] = session;
    const res = await fetch(path, Object.assign({}, opts, {headers}));
    const data = await res.json().catch(() => ({ok:false,error:'bad json'}));
    if (data.csrf) { csrf = data.csrf; localStorage.setItem('ne_csrf', csrf); }
    if (data.session) { session = data.session; localStorage.setItem('ne_session', session); }
    if (!res.ok || data.ok === false) throw new Error(data.error || ('HTTP '+res.status));
    return data;
  }

  function drawSpark(canvas, values, invert=false) {
    const ctx = canvas.getContext('2d');
    const w = canvas.width, h = canvas.height;
    ctx.clearRect(0,0,w,h);
    const nums = values.filter(v => v != null);
    if (!nums.length) return;
    const min = Math.min(...nums), max = Math.max(...nums);
    const span = Math.max(1, max-min);
    ctx.strokeStyle = '#fff';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    values.forEach((v,i) => {
      if (v == null) return;
      const x = (i/(values.length-1))*w;
      let n = (v-min)/span;
      if (invert) n = 1-n;
      const y = h - n*(h-6) - 3;
      if (i===0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
    });
    ctx.stroke();
  }

  function setTab(name) {
    $$('.tabs button').forEach(b => b.classList.toggle('active', b.dataset.tab===name));
    $$('.tab').forEach(t => t.classList.toggle('active', t.id==='tab-'+name));
    if (name==='scan') loadScan(false);
    if (name==='clients') loadClients();
    if (name==='settings') loadSettings();
    if (name==='logs') loadLogs();
  }

  function applyStatus(s) {
    const w = s.wifi || {}, sys = s.system || {}, h = (sys.health||{});
    $('#hdrState').textContent = w.state || '—';
    $('#hdrNet').textContent = (h.state || 'unknown').toUpperCase();
    $('#stState').textContent = w.state || '—';
    $('#stSsid').textContent = w.staSsid || 'Not Connected';
    $('#stRssi').textContent = w.staConnected ? (w.rssi + ' dBm') : '—';
    $('#stIp').textContent = w.staIp || '—';
    $('#stGw').textContent = w.gateway || '—';
    $('#stDns').textContent = w.dns || '—';
    $('#stNat').textContent = (w.nat ? 'NAT on' : 'NAT off') + (w.sharing ? ' / sharing' : '');
    $('#stInet').textContent = h.state || '—';
    $('#stClients').textContent = w.apClients ?? '—';
    $('#stUp').textContent = (sys.uptimeSec ?? 0) + 's';
    $('#stHeap').textContent = (sys.freeHeap ?? 0) + ' B';
    $('#stRe').textContent = sys.reconnectCount ?? 0;
    $('#stErr').textContent = sys.lastError || 'none';
    $('#sysFw').textContent = sys.firmware || '—';
    currentVersion = sys.firmware || currentVersion;
    $('#updateCurrent').textContent = 'Current version ' + currentVersion;
    $('#sysSdk').textContent = sys.sdk || '—';
    $('#sysCpu').textContent = (sys.cpuMhz || '—') + ' MHz';
    $('#sysFlash').textContent = ((sys.flashSize||0)/1048576).toFixed(2)+' MB / sketch '+((sys.sketchSize||0)/1024).toFixed(0)+' KB';
    $('#sysReset').textContent = sys.resetReason || '—';
    $('#sysCrash').textContent = sys.crashPending ? (sys.crashText||'pending') : 'none';
    $('#sysHealth').textContent = h.detail || '—';
    if (w.staConnected) { rssiHist.push(w.rssi); rssiHist.shift(); }
    heapHist.push(sys.freeHeap || null); heapHist.shift();
    drawSpark($('#gRssi'), rssiHist, true);
    drawSpark($('#gHeap'), heapHist, false);
    if (s.ota) {
      $('#otaStatus').textContent = s.ota.status + ' ' + (s.ota.progress||0) + '%';
      $('#otaBar').style.width = (s.ota.progress||0) + '%';
    }
    if (!updateChecked && w.staConnected && h.state === 'online') {
      updateChecked = true;
      setTimeout(() => checkForUpdate(false), 1000);
    }
  }

  async function refreshStatus() {
    try {
      const data = await api('/api/v1/status');
      applyStatus(data);
    } catch (e) {
      $('#hdrState').textContent = e.message;
    }
  }

  async function loadScan(force) {
    try {
      const q = force ? '?refresh=1' : '';
      const data = await api('/api/v1/scan'+q);
      const box = $('#scanList');
      box.innerHTML = '';
      $('#scanMeta').textContent = data.cached ? ('cached age '+Math.round((data.ageMs||0)/1000)+'s') : 'fresh';
      (data.networks||[]).forEach(n => {
        const el = document.createElement('div');
        el.className = 'item';
        el.innerHTML = `<div><div class="value" style="margin:0;font-size:16px">${esc(n.ssid||'(hidden)')}</div>
          <div class="muted">${esc(n.rssi)} dBm · ch ${esc(n.channel)} · ${esc(n.encryption||'')}</div></div>
          <button class="btn">Connect</button>`;
        el.querySelector('button').onclick = () => openConnect(n.ssid||'');
        box.appendChild(el);
      });
    } catch (e) {
      $('#scanList').textContent = e.message;
    }
  }

  async function loadClients() {
    try {
      const data = await api('/api/v1/clients');
      const box = $('#clientList');
      box.innerHTML = '';
      (data.clients||[]).forEach(c => {
        const el = document.createElement('div');
        el.className = 'item';
        el.innerHTML = `<div><div class="value" style="margin:0;font-size:16px">${esc(c.hostname||c.mac)}</div>
          <div class="muted">${esc(c.ip||'')} · ${esc(c.mac||'')} · RSSI ${esc(c.rssi)}</div></div>`;
        box.appendChild(el);
      });
      if (!(data.clients||[]).length) box.innerHTML = '<div class="muted">No clients</div>';
    } catch (e) {
      $('#clientList').textContent = e.message;
    }
  }

  async function loadSettings() {
    const data = await api('/api/v1/settings');
    const f = $('#settingsForm');
    f.apSsid.value = data.apSsid || '';
    f.apPass.value = data.apPass || '';
    f.deviceName.value = data.deviceName || '';
    f.hostname.value = data.hostname || '';
  }

  async function loadLogs() {
    const data = await api('/api/v1/logs');
    $('#logView').textContent = data.text || '';
  }

  function versionParts(value) {
    return String(value || '0.0.0').replace(/^v/, '').split('.')
      .map(part => parseInt(part, 10) || 0).slice(0, 3);
  }

  function isNewerVersion(candidate, current) {
    const a = versionParts(candidate), b = versionParts(current);
    for (let i = 0; i < 3; i++) {
      if ((a[i] || 0) > (b[i] || 0)) return true;
      if ((a[i] || 0) < (b[i] || 0)) return false;
    }
    return false;
  }

  async function checkForUpdate(showErrors = true) {
    const url = $('#updateServer').value.trim();
    localStorage.setItem('ne_update_server', url);
    try {
      $('#otaStatus').textContent = 'Checking for updates…';
      const res = await fetch(url, {cache:'no-store'});
      if (!res.ok) throw new Error('Update server returned HTTP ' + res.status);
      const manifest = await res.json();
      if (!manifest.version || !manifest.ota || !manifest.ota.sha256) {
        throw new Error('Unsupported update manifest');
      }
      remoteManifest = manifest;
      const available = isNewerVersion(manifest.version, currentVersion);
      $('#remoteUpdateCard').classList.toggle('hidden', !available);
      $('#remoteVersion').textContent = 'NanoExtend ' + manifest.version;
      $('#remoteNotes').textContent = manifest.releaseNotes || 'A verified firmware update is available.';
      $('#otaStatus').textContent = available ? 'Update available' : 'You are up to date';
    } catch (e) {
      $('#otaStatus').textContent = 'Update check unavailable';
      if (showErrors) alert(e.message || 'Update check failed');
    }
  }

  async function uploadOta(fileOrBlob, expectedSha256 = '') {
    const buffer = await fileOrBlob.arrayBuffer();
    const digest = await crypto.subtle.digest('SHA-256', buffer);
    const sha256 = [...new Uint8Array(digest)]
      .map(b => b.toString(16).padStart(2, '0')).join('');
    if (expectedSha256 && sha256.toLowerCase() !== expectedSha256.toLowerCase()) {
      throw new Error('Downloaded firmware checksum mismatch');
    }
    $('#otaHash').textContent = 'SHA-256 ' + sha256;
    $('#otaStatus').textContent = 'Uploading verified image…';
    const res = await fetch('/api/v1/update', {
      method:'POST',
      headers:{'X-CSRF-Token':csrf,'X-Session':session,'Content-Type':'application/octet-stream','X-File-Size':String(fileOrBlob.size),'X-SHA256':sha256},
      body:fileOrBlob
    });
    const data = await res.json().catch(()=>({}));
    if (!res.ok || data.ok===false) throw new Error(data.error || 'OTA failed');
  }

  function openConnect(ssid) {
    connectSsid = ssid;
    $('#modalSsid').textContent = ssid;
    $('#modalPass').value = '';
    $('#modalStatus').textContent = '';
    $('#modal').classList.remove('hidden');
  }

  $$('.tabs button').forEach(b => b.onclick = () => setTab(b.dataset.tab));
  $('#btnScan').onclick = () => { setTab('scan'); loadScan(true); };
  $('#btnRefreshScan').onclick = () => loadScan(true);
  $('#btnDisconnect').onclick = async () => { await api('/api/v1/disconnect',{method:'POST',body:'{}'}); refreshStatus(); };
  $('#btnReboot').onclick = async () => { await api('/api/v1/reboot',{method:'POST',body:'{}'}); };
  $('#btnFactory').onclick = async () => {
    if (!confirm('Factory reset all settings?')) return;
    await api('/api/v1/reset',{method:'POST',body:'{}'});
  };
  $('#btnLogs').onclick = loadLogs;
  $('#modalClose').onclick = () => $('#modal').classList.add('hidden');
  $('#modalConnect').onclick = async () => {
    $('#modalStatus').textContent = 'Connecting…';
    try {
      await api('/api/v1/connect',{method:'POST',body:JSON.stringify({ssid:connectSsid,password:$('#modalPass').value})});
      $('#modalStatus').textContent = 'Requested';
      setTimeout(() => { $('#modal').classList.add('hidden'); setTab('home'); refreshStatus(); }, 800);
    } catch (e) {
      $('#modalStatus').textContent = e.message;
    }
  };
  $('#settingsForm').onsubmit = async (e) => {
    e.preventDefault();
    const f = e.target;
    await api('/api/v1/settings',{method:'POST',body:JSON.stringify({
      apSsid:f.apSsid.value, apPass:f.apPass.value, deviceName:f.deviceName.value, hostname:f.hostname.value
    })});
    alert('Saved. Restart recommended.');
  };
  $('#btnBackup').onclick = async () => {
    const res = await fetch('/api/v1/settings/backup', {
      headers:{'X-CSRF-Token':csrf,'X-Session':session}
    });
    if (!res.ok) {
      const data = await res.json().catch(()=>({}));
      return alert(data.error || 'Backup failed');
    }
    const blob = await res.blob();
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = 'nanoextend-settings.json';
    a.click();
    URL.revokeObjectURL(url);
  };
  $('#btnRestore').onclick = async () => {
    const file = $('#restoreFile').files[0];
    if (!file) return alert('Choose a NanoExtend backup file');
    if (!confirm('Restore settings and restart NanoExtend?')) return;
    try {
      const data = JSON.parse(await file.text());
      await api('/api/v1/settings/restore', {
        method:'POST',
        body:JSON.stringify(data)
      });
      alert('Settings restored. Restart NanoExtend to apply all changes.');
    } catch (e) {
      alert(e.message || 'Restore failed');
    }
  };
  $('#updateServer').value = localStorage.getItem('ne_update_server') ||
    'https://github.com/fahimuntasin/NanoExtend/releases/latest/download/manifest.json';
  $('#btnCheckUpdate').onclick = () => checkForUpdate(true);
  $('#btnRemoteUpdate').onclick = async () => {
    if (!remoteManifest?.ota) return alert('Check for updates first');
    if (!confirm('Download and install NanoExtend ' + remoteManifest.version + '?')) return;
    try {
      $('#otaStatus').textContent = 'Downloading update…';
      const manifestUrl = $('#updateServer').value.trim();
      const firmwareUrl = new URL(remoteManifest.ota.file, manifestUrl).href;
      const response = await fetch(firmwareUrl, {cache:'no-store'});
      if (!response.ok) throw new Error('Firmware download failed');
      const blob = await response.blob();
      if (remoteManifest.ota.size && blob.size !== remoteManifest.ota.size) {
        throw new Error('Downloaded firmware size mismatch');
      }
      await uploadOta(blob, remoteManifest.ota.sha256);
      alert('Verified update installed. NanoExtend is rebooting…');
    } catch (e) {
      $('#otaStatus').textContent = 'Update failed';
      alert(e.message || 'Remote update failed');
    }
  };
  $('#btnOta').onclick = async () => {
    const file = $('#otaFile').files[0];
    if (!file) return alert('Choose a .bin file');
    try {
      $('#otaStatus').textContent = 'Calculating SHA-256…';
      await uploadOta(file);
      alert('OTA success. Rebooting…');
    } catch (e) {
      alert(e.message || 'OTA failed');
    }
  };

  // WebSocket live status
  function connectWs() {
    const ws = new WebSocket((location.protocol==='https:'?'wss://':'ws://')+location.host+'/ws');
    ws.onmessage = (ev) => {
      try { applyStatus(JSON.parse(ev.data)); } catch (_) {}
    };
    ws.onclose = () => setTimeout(connectWs, 2000);
  }

  api('/api/v1/status').then(applyStatus).catch(()=>{});
  connectWs();
  setInterval(refreshStatus, 5000);
})();
