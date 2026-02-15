#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <Arduino.h>

const char WEB_DASHBOARD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1">
<title>UK-OUI-SPY PRO</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
:root{--bg:#0a0e17;--card:#111827;--border:#1e293b;--cyan:#06b6d4;--red:#ef4444;--orange:#f97316;--yellow:#eab308;--green:#22c55e;--text:#e2e8f0;--muted:#64748b}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:var(--bg);color:var(--text);min-height:100vh}
.header{background:linear-gradient(135deg,#0f172a,#1e1b4b);padding:16px 20px;border-bottom:1px solid var(--border);display:flex;justify-content:space-between;align-items:center}
.header h1{font-size:18px;color:var(--cyan);font-weight:700;letter-spacing:1px}
.header .status{display:flex;gap:8px;align-items:center}
.badge{padding:3px 8px;border-radius:12px;font-size:11px;font-weight:600}
.badge-scan{background:rgba(6,182,212,0.15);color:var(--cyan)}
.badge-web{background:rgba(34,197,94,0.15);color:var(--green)}
.nav{display:flex;background:#111827;border-bottom:1px solid var(--border);overflow-x:auto}
.nav a{padding:12px 16px;color:var(--muted);text-decoration:none;font-size:13px;font-weight:500;white-space:nowrap;border-bottom:2px solid transparent;transition:all .2s}
.nav a.active,.nav a:hover{color:var(--cyan);border-bottom-color:var(--cyan)}
.content{padding:16px;max-width:960px;margin:0 auto}
.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(140px,1fr));gap:12px;margin-bottom:20px}
.stat{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:14px;text-align:center}
.stat .val{font-size:24px;font-weight:700;color:var(--cyan)}
.stat .lbl{font-size:11px;color:var(--muted);margin-top:4px;text-transform:uppercase;letter-spacing:.5px}
.card{background:var(--card);border:1px solid var(--border);border-radius:10px;padding:16px;margin-bottom:12px}
.card h3{font-size:14px;color:var(--muted);margin-bottom:12px;text-transform:uppercase;letter-spacing:.5px}
.det-row{display:flex;align-items:center;padding:10px;border-radius:8px;margin-bottom:6px;background:rgba(255,255,255,0.02);border-left:3px solid var(--green);transition:background .2s}
.det-row:hover{background:rgba(255,255,255,0.05)}
.det-row.high{border-left-color:var(--red)}
.det-row.medium{border-left-color:var(--yellow)}
.det-info{flex:1}
.det-mfr{font-weight:600;font-size:14px}
.det-meta{font-size:12px;color:var(--muted);margin-top:2px}
.det-right{text-align:right}
.det-threat{font-size:20px;font-weight:700}
.det-rssi{font-size:11px;color:var(--muted)}
.threat-high{color:var(--red)}.threat-med{color:var(--orange)}.threat-low{color:var(--green)}
.toggle-row{display:flex;justify-content:space-between;align-items:center;padding:12px 0;border-bottom:1px solid var(--border)}
.toggle-row:last-child{border-bottom:none}
.toggle-label{font-size:14px}
.toggle{position:relative;width:44px;height:24px;cursor:pointer}
.toggle input{opacity:0;width:0;height:0}
.toggle .slider{position:absolute;inset:0;background:#374151;border-radius:12px;transition:.3s}
.toggle .slider:before{content:'';position:absolute;height:18px;width:18px;left:3px;bottom:3px;background:#fff;border-radius:50%;transition:.3s}
.toggle input:checked+.slider{background:var(--cyan)}
.toggle input:checked+.slider:before{transform:translateX(20px)}
canvas{width:100%;border-radius:10px;background:var(--card);border:1px solid var(--border)}
.filter-bar{display:flex;gap:8px;margin-bottom:16px;flex-wrap:wrap}
.filter-btn{padding:6px 14px;border-radius:16px;border:1px solid var(--border);background:transparent;color:var(--muted);font-size:12px;cursor:pointer;transition:all .2s}
.filter-btn.active{background:var(--cyan);color:#000;border-color:var(--cyan)}
.btn{padding:8px 20px;border-radius:8px;border:none;font-size:13px;font-weight:600;cursor:pointer;transition:all .2s}
.btn-primary{background:var(--cyan);color:#000}.btn-primary:hover{opacity:.8}
.footer{text-align:center;padding:20px;color:var(--muted);font-size:11px;border-top:1px solid var(--border);margin-top:20px}
.page{display:none}.page.active{display:block}
@media(max-width:600px){.grid{grid-template-columns:repeat(2,1fr)}.header h1{font-size:15px}}
</style>
</head>
<body>
<div class="header">
<h1>UK-OUI-SPY PRO</h1>
<div class="status">
<span class="badge badge-scan" id="scanBadge">SCANNING</span>
<span class="badge badge-web" id="webBadge">WEB LIVE</span>
</div>
</div>
<nav class="nav">
<a href="#" class="active" onclick="showPage('dashboard',this)">Dashboard</a>
<a href="#" onclick="showPage('detections',this)">Detections</a>
<a href="#" onclick="showPage('radar',this)">Radar</a>
<a href="#" onclick="showPage('config',this)">Settings</a>
<a href="#" onclick="showPage('logs',this)">Logs</a>
<a href="#" onclick="showPage('about',this)">About</a>
</nav>
<div class="content">

<!-- DASHBOARD -->
<div id="dashboard" class="page active">
<div class="grid">
<div class="stat"><div class="val" id="sTotal">0</div><div class="lbl">Detections</div></div>
<div class="stat"><div class="val" id="sHigh" style="color:var(--red)">0</div><div class="lbl">High Threat</div></div>
<div class="stat"><div class="val" id="sPackets">0</div><div class="lbl">Scanned</div></div>
<div class="stat"><div class="val" id="sBattery">--%</div><div class="lbl">Battery</div></div>
<div class="stat"><div class="val" id="sMemory">--</div><div class="lbl">Free Memory</div></div>
<div class="stat"><div class="val" id="sUptime">--</div><div class="lbl">Uptime</div></div>
</div>
<div class="card">
<h3>Recent Detections</h3>
<div id="recentList"></div>
</div>
</div>

<!-- DETECTIONS -->
<div id="detections" class="page">
<div class="filter-bar">
<button class="filter-btn active" onclick="setFilter('all',this)">All</button>
<button class="filter-btn" onclick="setFilter('high',this)">High</button>
<button class="filter-btn" onclick="setFilter('medium',this)">Medium</button>
<button class="filter-btn" onclick="setFilter('low',this)">Low</button>
</div>
<div id="detList"></div>
</div>

<!-- RADAR -->
<div id="radar" class="page">
<canvas id="radarCanvas" width="600" height="600"></canvas>
</div>

<!-- CONFIG -->
<div id="config" class="page">
<div class="card">
<h3>Scanning</h3>
<div class="toggle-row"><span class="toggle-label">BLE Scanning</span><label class="toggle"><input type="checkbox" id="cfgBLE" onchange="updateConfig()"><span class="slider"></span></label></div>
<div class="toggle-row"><span class="toggle-label">WiFi Scanning</span><label class="toggle"><input type="checkbox" id="cfgWiFi" onchange="updateConfig()"><span class="slider"></span></label></div>
</div>
<div class="card">
<h3>Logging</h3>
<div class="toggle-row"><span class="toggle-label">SD Card Logging</span><label class="toggle"><input type="checkbox" id="cfgLog" onchange="updateConfig()"><span class="slider"></span></label></div>
<div class="toggle-row"><span class="toggle-label">Encrypted Logs</span><label class="toggle"><input type="checkbox" id="cfgSecure" onchange="updateConfig()"><span class="slider"></span></label></div>
</div>
<div class="card">
<h3>Display</h3>
<div class="toggle-row"><span class="toggle-label">Auto Brightness</span><label class="toggle"><input type="checkbox" id="cfgAutoBr" onchange="updateConfig()"><span class="slider"></span></label></div>
<div class="toggle-row"><span class="toggle-label">Show Baseline</span><label class="toggle"><input type="checkbox" id="cfgBaseline" onchange="updateConfig()"><span class="slider"></span></label></div>
</div>
</div>

<!-- LOGS -->
<div id="logs" class="page">
<div class="card">
<h3>Detection Log</h3>
<p style="color:var(--muted);font-size:13px;margin-bottom:12px">Download the raw CSV detection log from the SD card.</p>
<a href="/api/logs/download" class="btn btn-primary">Download CSV</a>
</div>
<div class="card">
<h3>Recent Log Entries</h3>
<div id="logEntries" style="font-family:monospace;font-size:12px;color:var(--muted);max-height:400px;overflow-y:auto"></div>
</div>
</div>

<!-- ABOUT -->
<div id="about" class="page">
<div class="card">
<h3>Device Information</h3>
<table style="width:100%;font-size:13px">
<tr><td style="color:var(--muted);padding:6px 0">Firmware</td><td id="aFW" style="text-align:right;padding:6px 0">--</td></tr>
<tr><td style="color:var(--muted);padding:6px 0">OUI Database</td><td id="aDB" style="text-align:right;padding:6px 0">--</td></tr>
<tr><td style="color:var(--muted);padding:6px 0">SD Card</td><td id="aSD" style="text-align:right;padding:6px 0">--</td></tr>
<tr><td style="color:var(--muted);padding:6px 0">Touch</td><td id="aTouch" style="text-align:right;padding:6px 0">--</td></tr>
<tr><td style="color:var(--muted);padding:6px 0">WiFi Clients</td><td id="aClients" style="text-align:right;padding:6px 0">--</td></tr>
</table>
</div>
</div>

</div>
<div class="footer">UK-OUI-SPY PRO &middot; Professional Surveillance Detection System</div>

<script>
let allDetections=[],currentFilter='all';
function showPage(id,el){
  document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
  document.getElementById(id).classList.add('active');
  document.querySelectorAll('.nav a').forEach(a=>a.classList.remove('active'));
  if(el)el.classList.add('active');
  if(id==='radar')drawRadar();
  if(id==='logs')loadLogs();
}
function setFilter(f,el){
  currentFilter=f;
  document.querySelectorAll('.filter-btn').forEach(b=>b.classList.remove('active'));
  el.classList.add('active');
  renderDetections();
}
function threatClass(s){return s>=60?'threat-high':s>=30?'threat-med':'threat-low'}
function prioClass(p){return p>=4?'high':p>=3?'medium':'low'}
function renderDetRow(d){
  return '<div class="det-row '+prioClass(d.priority)+'">'+
    '<div class="det-info"><div class="det-mfr">'+d.manufacturer+'</div>'+
    '<div class="det-meta">'+d.category+' &middot; '+(d.isBLE?'BLE':'WiFi')+' &middot; '+d.rssi+' dBm &middot; x'+d.sightings+'</div></div>'+
    '<div class="det-right"><div class="det-threat '+threatClass(d.threat)+'">'+d.threat+'</div><div class="det-rssi">'+d.mac.substring(0,8)+'</div></div></div>';
}
function renderDetections(){
  var f=currentFilter==='all'?allDetections:allDetections.filter(function(d){return d.relevance.toLowerCase()===currentFilter});
  document.getElementById('detList').innerHTML=f.length?f.map(renderDetRow).join(''):'<p style="color:var(--muted);text-align:center;padding:40px">No detections yet...</p>';
}
function drawRadar(){
  var c=document.getElementById('radarCanvas'),ctx=c.getContext('2d');
  var w=c.width,h=c.height,cx=w/2,cy=h/2,r=Math.min(cx,cy)-30;
  ctx.clearRect(0,0,w,h);ctx.fillStyle='#111827';ctx.fillRect(0,0,w,h);
  ctx.strokeStyle='#1e293b';ctx.lineWidth=1;
  for(var i=1;i<=4;i++){ctx.beginPath();ctx.arc(cx,cy,r*i/4,0,Math.PI*2);ctx.stroke();}
  ctx.beginPath();ctx.moveTo(cx-r,cy);ctx.lineTo(cx+r,cy);ctx.moveTo(cx,cy-r);ctx.lineTo(cx,cy+r);ctx.stroke();
  ctx.fillStyle='#06b6d4';ctx.beginPath();ctx.arc(cx,cy,4,0,Math.PI*2);ctx.fill();
  ctx.fillStyle='#64748b';ctx.font='11px sans-serif';ctx.fillText('YOU',cx+8,cy-8);
  if(allDetections.length===0){ctx.fillStyle='#64748b';ctx.font='13px sans-serif';ctx.textAlign='center';ctx.fillText('No devices in range',cx,cy+r/2);ctx.textAlign='start';return;}
  allDetections.forEach(function(d){
    var dist=(1-((Math.min(Math.max(d.rssi,-100),-30)+100)/70))*r;
    var hash=0;for(var j=0;j<d.mac.length;j++)hash=hash*31+d.mac.charCodeAt(j);
    var angle=(hash%360)*Math.PI/180;
    var px=cx+dist*Math.cos(angle),py=cy+dist*Math.sin(angle);
    var col=d.threat>=60?'#ef4444':d.threat>=30?'#f97316':'#22c55e';
    ctx.fillStyle=col;ctx.beginPath();ctx.arc(px,py,6,0,Math.PI*2);ctx.fill();
    ctx.strokeStyle=col;ctx.lineWidth=1;ctx.beginPath();ctx.arc(px,py,9,0,Math.PI*2);ctx.stroke();
    ctx.fillStyle='#e2e8f0';ctx.font='10px sans-serif';ctx.fillText(d.manufacturer.substring(0,8),px+12,py+4);
  });
}
async function fetchData(){
  try{
    var res=await Promise.all([fetch('/api/detections').then(function(r){return r.json()}),fetch('/api/status').then(function(r){return r.json()})]);
    var det=res[0],st=res[1];
    allDetections=det.detections||[];
    document.getElementById('sTotal').textContent=det.total||0;
    document.getElementById('sHigh').textContent=det.highCount||0;
    document.getElementById('sPackets').textContent=st.totalScanned||0;
    document.getElementById('sBattery').textContent=(st.battery||'--')+'%';
    document.getElementById('sMemory').textContent=(st.freeHeap||'--')+' KB';
    document.getElementById('sUptime').textContent=st.uptime||'--';
    document.getElementById('aFW').textContent=st.firmware||'--';
    document.getElementById('aDB').textContent=(st.ouiCount||'--')+' entries';
    document.getElementById('aSD').textContent=st.sdCard?'Ready':'Not Found';
    document.getElementById('aTouch').textContent=st.touch?'OK':'Error';
    document.getElementById('aClients').textContent=st.webClients||0;
    document.getElementById('recentList').innerHTML=allDetections.slice(0,5).map(renderDetRow).join('')||'<p style="color:var(--muted);padding:20px;text-align:center">Scanning...</p>';
    renderDetections();
    if(document.getElementById('radar').classList.contains('active'))drawRadar();
  }catch(e){console.error('Fetch error:',e);}
}
async function loadCfg(){
  try{
    var c=await fetch('/api/config').then(function(r){return r.json()});
    document.getElementById('cfgBLE').checked=c.ble;
    document.getElementById('cfgWiFi').checked=c.wifi;
    document.getElementById('cfgLog').checked=c.logging;
    document.getElementById('cfgSecure').checked=c.secure;
    document.getElementById('cfgAutoBr').checked=c.autoBrightness;
    document.getElementById('cfgBaseline').checked=c.showBaseline;
  }catch(e){}
}
async function updateConfig(){
  var body={ble:document.getElementById('cfgBLE').checked,wifi:document.getElementById('cfgWiFi').checked,
    logging:document.getElementById('cfgLog').checked,secure:document.getElementById('cfgSecure').checked,
    autoBrightness:document.getElementById('cfgAutoBr').checked,showBaseline:document.getElementById('cfgBaseline').checked};
  try{await fetch('/api/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});}catch(e){}
}
async function loadLogs(){
  try{
    var data=await fetch('/api/logs').then(function(r){return r.text()});
    document.getElementById('logEntries').innerText=data||'No log data available.';
  }catch(e){document.getElementById('logEntries').innerText='Error loading logs.';}
}
fetchData();loadCfg();setInterval(fetchData,3000);
</script>
</body>
</html>
)rawliteral";

#endif
