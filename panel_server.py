#!/usr/bin/env python3
import os
import json
import re
import subprocess
import hashlib
import secrets
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse

BASE_DIR = os.environ.get("CCL_PANEL_DIR", "/home/doruk/ccl-panel")
LIBS_DIR = os.path.join(BASE_DIR, "libs")
INC_DIR = os.path.join(BASE_DIR, "include")
META_FILE = os.path.join(LIBS_DIR, "meta.json")
USERS_FILE = os.path.join(BASE_DIR, "users.json")
ADMIN_PASS = os.environ.get("CCL_ADMIN_PASS", "")
os.makedirs(LIBS_DIR, exist_ok=True)

TOKENS = {}  # token -> username


def load_json(path, default):
    if os.path.exists(path):
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    return default


def save_json(path, data):
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)


def hash_pw(pw, salt=None):
    if not salt:
        salt = secrets.token_hex(8)
    h = hashlib.sha256((salt + pw).encode("utf-8")).hexdigest()
    return f"{salt}${h}"


def verify_pw(pw, stored):
    if "$" not in stored:
        return False
    salt, _ = stored.split("$", 1)
    return hash_pw(pw, salt) == stored


HTML_PAGE = """<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>🌸 Comar — ÇiçekliScript Kütüphane Deposu</title>
<style>
  :root {
    --bg: #090d16; --surface: #111827; --card: #161f30; --border: #263248;
    --text: #f1f5f9; --muted: #94a3b8; --accent: #38bdf8; --accent2: #818cf8;
    --ok: #22c55e; --err: #ef4444; --warn: #f59e0b;
  }
  * { box-sizing: border-box; }
  body { margin: 0; font-family: 'Inter', system-ui, -apple-system, sans-serif; background: radial-gradient(circle at top right, #172554 0%, var(--bg) 45%); color: var(--text); min-height: 100vh; padding: 24px; }
  .wrap { max-width: 1200px; margin: 0 auto; }
  .nav { display: flex; justify-content: space-between; align-items: center; padding-bottom: 20px; border-bottom: 1px solid var(--border); margin-bottom: 24px; flex-wrap: wrap; gap: 12px; }
  .brand { display: flex; align-items: center; gap: 12px; }
  .brand h1 { margin: 0; font-size: 24px; background: linear-gradient(90deg, var(--accent), var(--accent2)); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
  .nav-links { display: flex; gap: 10px; align-items: center; flex-wrap: wrap; }
  .btn-link { color: var(--text); text-decoration: none; font-size: 13px; border: 1px solid var(--border); background: var(--surface); padding: 8px 14px; border-radius: 8px; cursor: pointer; transition: 0.15s; }
  .btn-link:hover { border-color: var(--accent); color: var(--accent); }
  .stats { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 14px; margin-bottom: 24px; }
  .stat { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 16px; }
  .stat-val { font-size: 26px; font-weight: 800; color: var(--accent); }
  .stat-lbl { font-size: 12px; color: var(--muted); margin-top: 4px; }
  .grid { display: grid; grid-template-columns: 1.05fr 1.15fr; gap: 22px; align-items: start; }
  @media (max-width: 900px) { .grid { grid-template-columns: 1fr; } }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 14px; padding: 22px; box-shadow: 0 10px 30px rgba(0,0,0,0.25); }
  .card h2 { margin: 0 0 14px; font-size: 18px; display: flex; align-items: center; gap: 8px; }
  label { display: block; font-size: 12px; font-weight: 600; color: var(--muted); margin: 12px 0 6px; text-transform: uppercase; letter-spacing: 0.04em; }
  input, select, textarea { width: 100%; background: #0b1120; color: var(--text); border: 1px solid var(--border); border-radius: 8px; padding: 10px 12px; font-size: 14px; outline: none; transition: border-color 0.15s; }
  input:focus, select:focus, textarea:focus { border-color: var(--accent); }
  textarea { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; min-height: 220px; resize: vertical; line-height: 1.45; }
  .btn { margin-top: 14px; background: linear-gradient(90deg, var(--accent), var(--accent2)); color: #090d16; border: none; border-radius: 8px; padding: 11px 20px; font-weight: 700; cursor: pointer; font-size: 14px; width: 100%; }
  .btn:hover { opacity: 0.92; }
  .c-help { margin-top: 12px; padding: 12px; background: #0b1120; border: 1px solid var(--border); border-radius: 8px; font-size: 12px; color: var(--muted); display: none; }
  .c-help code { color: var(--accent); }
  .msg { margin-top: 12px; padding: 10px 12px; border-radius: 8px; font-size: 13px; white-space: pre-wrap; display: none; }
  .msg.ok { display: block; background: rgba(34,197,94,0.12); border: 1px solid var(--ok); color: #86efac; }
  .msg.err { display: block; background: rgba(239,68,68,0.12); border: 1px solid var(--err); color: #fca5a5; }
  .toolbar { display: flex; gap: 10px; margin-bottom: 14px; flex-wrap: wrap; }
  .toolbar input { flex: 1; min-width: 180px; }
  .tabs { display: flex; gap: 6px; }
  .tab { background: #0b1120; color: var(--muted); border: 1px solid var(--border); border-radius: 8px; padding: 8px 12px; font-size: 12px; font-weight: 600; cursor: pointer; }
  .tab.active { background: var(--accent); color: #090d16; border-color: var(--accent); }
  .lib-item { border: 1px solid var(--border); border-radius: 10px; padding: 14px; margin-bottom: 12px; background: #0b1120; }
  .lib-top { display: flex; justify-content: space-between; align-items: center; gap: 10px; }
  .lib-name { font-size: 16px; font-weight: 700; color: var(--text); }
  .author-tag { font-size: 12px; color: var(--accent2); margin-left: 6px; }
  .badge { font-size: 11px; padding: 3px 9px; border-radius: 999px; font-weight: 700; }
  .badge.ccl { background: rgba(56,189,248,0.15); color: var(--accent); border: 1px solid rgba(56,189,248,0.3); }
  .badge.so { background: rgba(34,197,94,0.15); color: var(--ok); border: 1px solid rgba(34,197,94,0.3); }
  .lib-desc { color: var(--muted); font-size: 13px; margin: 8px 0 10px; }
  .lib-actions { display: flex; justify-content: space-between; align-items: center; gap: 8px; flex-wrap: wrap; }
  .cmd-box { background: #161f30; border: 1px solid var(--border); padding: 5px 10px; border-radius: 6px; font-family: monospace; font-size: 12px; color: #cbd5e1; cursor: pointer; }
  .btn-sm { background: transparent; color: var(--accent); border: 1px solid var(--border); border-radius: 6px; padding: 5px 10px; font-size: 12px; cursor: pointer; }
  .btn-sm:hover { border-color: var(--accent); }
  .modal-bg { position: fixed; inset: 0; background: rgba(0,0,0,0.75); display: none; align-items: center; justify-content: center; padding: 20px; z-index: 50; }
  .modal { background: var(--card); border: 1px solid var(--border); border-radius: 14px; max-width: 760px; width: 100%; padding: 22px; max-height: 85vh; overflow-y: auto; }
  .modal pre { background: #0b1120; border: 1px solid var(--border); padding: 14px; border-radius: 8px; overflow-x: auto; font-size: 13px; line-height: 1.45; }
</style>
</head>
<body>
<div class="wrap">
  <div class="nav">
    <div class="brand">
      <span style="font-size:28px;">🌸</span>
      <div>
        <h1>Comar Kütüphane Deposu</h1>
        <div style="font-size:12px;color:var(--muted);">ÇiçekliScript (.ccl) & C (.so) Paket Sistemi</div>
      </div>
    </div>
    <div class="nav-links">
      <span id="userBadge" style="font-size:13px;color:var(--accent2);display:none;"></span>
      <button id="authBtn" class="btn-link" onclick="openAuthModal()">👤 Giriş / Kayıt</button>
      <button id="logoutBtn" class="btn-link" style="display:none;" onclick="logout()">Çıkış</button>
      <a class="btn-link" href="/admin">🔐 Admin Onay Paneli</a>
    </div>
  </div>

  <div class="stats">
    <div class="stat"><div class="stat-val" id="stTotal">-</div><div class="stat-lbl">Onaylı Kütüphane</div></div>
    <div class="stat"><div class="stat-val" id="stCcl">-</div><div class="stat-lbl">ÇiçekliScript (.ccl)</div></div>
    <div class="stat"><div class="stat-val" id="stSo">-</div><div class="stat-lbl">C Eklentisi (.so)</div></div>
    <div class="stat"><div class="stat-val" style="font-size:15px;padding-top:6px;font-family:monospace;">comar install &lt;ad&gt;</div><div class="stat-lbl">Hızlı Kurulum Komutu</div></div>
  </div>

  <div class="grid">
    <div class="card">
      <h2 id="formTitle">📦 Yeni Kütüphane Gönder</h2>
      <div id="loginWarn" style="color:var(--warn);font-size:13px;margin-bottom:10px;display:none;">Kütüphane göndermek veya güncellemek için giriş yapmalısınız.</div>
      <label>Kütüphane Adı (harf, rakam, _)</label>
      <input id="name" placeholder="ör: mathx" />
      <label>Açıklama</label>
      <input id="desc" placeholder="Ne işe yarar?" />
      <label>Format</label>
      <select id="fmt" onchange="onFmtChange()">
        <option value="ccl">ÇiçekliScript (.ccl)</option>
        <option value="c">C Kütüphanesi (.c -> .so)</option>
      </select>
      <label>Dosyadan Seç (.ccl / .c)</label>
      <input type="file" id="fileInput" accept=".ccl,.c,.ms" onchange="loadFromFile(event)" />
      <label>Kaynak Kod</label>
      <textarea id="code"></textarea>
      <div id="cHelp" class="c-help">
        <strong>C Eklenti Rehberi:</strong> <code>#include "ccl_plugin.h"</code> ekleyin ve <code>void ccl_init(CCLRegisterFn reg)</code> içinde <code>reg("ad", fn, arity);</code> çağırın.
      </div>
      <button class="btn" onclick="submitLib()">Onaya Gönder</button>
      <div id="msg" class="msg"></div>
    </div>

    <div class="card">
      <h2>📚 Onaylı Kütüphaneler</h2>
      <div class="toolbar">
        <input id="search" placeholder="Ara (ad, yapımcı veya açıklama)..." oninput="renderList()" />
        <div class="tabs">
          <button class="tab active" onclick="setFilter('all', this)">Tümü</button>
          <button class="tab" onclick="setFilter('ccl', this)">CCL</button>
          <button class="tab" onclick="setFilter('so', this)">C (.so)</button>
        </div>
      </div>
      <div id="list">Yükleniyor...</div>
    </div>
  </div>
</div>

<!-- Code Modal -->
<div id="modalBg" class="modal-bg" onclick="if(event.target===this)closeModal()">
  <div class="modal">
    <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;">
      <h3 id="mTitle" style="margin:0;"></h3>
      <button class="btn-sm" onclick="closeModal()">Kapat ✕</button>
    </div>
    <div id="mMeta" style="color:var(--muted);font-size:13px;margin-bottom:10px;"></div>
    <pre id="mCode"></pre>
  </div>
</div>

<!-- Auth Modal -->
<div id="authBg" class="modal-bg" onclick="if(event.target===this)closeAuthModal()">
  <div class="modal" style="max-width:420px;">
    <div style="display:flex;justify-content:space-between;align-items:center;margin-bottom:12px;">
      <h3 style="margin:0;">👤 Hesap</h3>
      <button class="btn-sm" onclick="closeAuthModal()">Kapat ✕</button>
    </div>
    <label>Kullanıcı Adı</label>
    <input id="authUser" placeholder="ör: doruk" />
    <label>Şifre</label>
    <input id="authPass" type="password" placeholder="••••••" />
    <div style="display:flex;gap:10px;margin-top:14px;">
      <button class="btn" style="margin-top:0;" onclick="doAuth('login')">Giriş Yap</button>
      <button class="btn" style="margin-top:0;background:var(--surface);color:var(--text);border:1px solid var(--border);" onclick="doAuth('register')">Kayıt Ol</button>
    </div>
    <div id="authMsg" class="msg"></div>
  </div>
</div>

<script>
const TPL_CCL = `// ÇiçekliScript Kütüphanesi\ndef topla(a, b) {\n    return a + b;\n}`;
const TPL_C = `#include "ccl_plugin.h"\n\nstatic CCLVal c_topla(int argc, CCLVal* argv) {\n    if (argc < 2) return ccl_nil();\n    return ccl_num(ccl_as_num(argv[0]) + ccl_as_num(argv[1]));\n}\n\nvoid ccl_init(CCLRegisterFn reg) {\n    reg("c_topla", c_topla, 2);\n}`;

let allLibs = [];
let currentFilter = "all";
let authToken = localStorage.getItem("ccl_token") || "";
let authUsername = localStorage.getItem("ccl_user") || "";

function updateAuthUI() {
  if (authToken && authUsername) {
    document.getElementById("userBadge").textContent = "👤 @" + authUsername;
    document.getElementById("userBadge").style.display = "inline";
    document.getElementById("authBtn").style.display = "none";
    document.getElementById("logoutBtn").style.display = "inline-block";
    document.getElementById("loginWarn").style.display = "none";
  } else {
    document.getElementById("userBadge").style.display = "none";
    document.getElementById("authBtn").style.display = "inline-block";
    document.getElementById("logoutBtn").style.display = "none";
    document.getElementById("loginWarn").style.display = "block";
  }
}
updateAuthUI();

function openAuthModal() { document.getElementById("authBg").style.display = "flex"; }
function closeAuthModal() { document.getElementById("authBg").style.display = "none"; }
function logout() {
  authToken = ""; authUsername = "";
  localStorage.removeItem("ccl_token"); localStorage.removeItem("ccl_user");
  updateAuthUI(); renderList();
}

async function doAuth(mode) {
  const m = document.getElementById("authMsg");
  m.style.display = "none";
  const username = document.getElementById("authUser").value.trim();
  const password = document.getElementById("authPass").value;
  const res = await fetch("/api/auth/" + mode, {
    method: "POST",
    headers: {"Content-Type": "application/json"},
    body: JSON.stringify({username, password})
  });
  const data = await res.json();
  if (res.ok) {
    authToken = data.token; authUsername = data.username;
    localStorage.setItem("ccl_token", authToken);
    localStorage.setItem("ccl_user", authUsername);
    updateAuthUI(); closeAuthModal(); renderList();
  } else {
    m.className = "msg err"; m.textContent = data.error || "Hata!"; m.style.display = "block";
  }
}

function onFmtChange() {
  const fmt = document.getElementById("fmt").value;
  document.getElementById("cHelp").style.display = fmt === "c" ? "block" : "none";
  const ta = document.getElementById("code");
  if (!ta.value.trim() || ta.value === TPL_CCL || ta.value === TPL_C) {
    ta.value = fmt === "c" ? TPL_C : TPL_CCL;
  }
}
onFmtChange();

function loadFromFile(e) {
  const file = e.target.files[0];
  if (!file) return;
  const ext = file.name.split(".").pop().toLowerCase();
  document.getElementById("fmt").value = ext === "c" ? "c" : "ccl";
  onFmtChange();
  const nameEl = document.getElementById("name");
  if (!nameEl.value.trim()) {
    nameEl.value = file.name.replace(/\.[^.]+$/, "").replace(/[^a-zA-Z0-9_]/g, "_");
  }
  const reader = new FileReader();
  reader.onload = ev => { document.getElementById("code").value = ev.target.result; };
  reader.readAsText(file);
}

function setFilter(f, btn) {
  currentFilter = f;
  document.querySelectorAll(".tab").forEach(b => b.classList.remove("active"));
  btn.classList.add("active");
  renderList();
}

function esc(s) { return (s||"").replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;"); }

async function loadLibs() {
  const res = await fetch("/api/libs");
  allLibs = await res.json();
  document.getElementById("stTotal").textContent = allLibs.length;
  document.getElementById("stCcl").textContent = allLibs.filter(l => l.type !== "so").length;
  document.getElementById("stSo").textContent = allLibs.filter(l => l.type === "so").length;
  renderList();
}

function renderList() {
  const q = document.getElementById("search").value.toLowerCase();
  const filtered = allLibs.filter(l => {
    if (currentFilter !== "all" && l.type !== currentFilter) return false;
    return l.name.toLowerCase().includes(q) || (l.description||"").toLowerCase().includes(q) || (l.author||"").toLowerCase().includes(q);
  });
  const el = document.getElementById("list");
  if (!filtered.length) { el.innerHTML = "<p style='color:var(--muted)'>Eşleşen kütüphane bulunamadı.</p>"; return; }
  el.innerHTML = filtered.map(l => `
    <div class="lib-item">
      <div class="lib-top">
        <div>
          <span class="lib-name">${esc(l.name)}</span>
          <span class="author-tag">@${esc(l.author || "anonim")}</span>
        </div>
        <span class="badge ${l.type}">${l.type === "so" ? "C (.so)" : "CCL"}</span>
      </div>
      <div class="lib-desc">${esc(l.description || "Açıklama yok.")}</div>
      <div class="lib-actions">
        <span class="cmd-box" onclick="copyCmd('comar install ${esc(l.name)}', this)" title="Kopyalamak için tıkla">comar install ${esc(l.name)}</span>
        <div style="display:flex;gap:6px;">
          ${authUsername && l.author === authUsername ? `<button class="btn-sm" onclick="editLib('${esc(l.name)}')">✏️ Güncelle</button>` : ""}
          <button class="btn-sm" onclick="viewCode('${esc(l.name)}')">Kodu Gör</button>
        </div>
      </div>
    </div>
  `).join("");
}

function copyCmd(text, el) {
  navigator.clipboard.writeText(text);
  const old = el.textContent;
  el.textContent = "✔ Kopyalandı!";
  setTimeout(() => el.textContent = old, 1200);
}

async function editLib(name) {
  const lib = allLibs.find(l => l.name === name);
  const res = await fetch("/api/libs/" + name + "/code");
  const data = await res.json();
  document.getElementById("name").value = name;
  document.getElementById("desc").value = lib ? lib.description : "";
  document.getElementById("fmt").value = data.type === "so" ? "c" : "ccl";
  onFmtChange();
  document.getElementById("code").value = data.code || "";
  document.getElementById("formTitle").textContent = "✏️ Kütüphane Güncelle: " + name;
  window.scrollTo({ top: 0, behavior: "smooth" });
}

async function viewCode(name) {
  const res = await fetch("/api/libs/" + name + "/code");
  const data = await res.json();
  document.getElementById("mTitle").textContent = name + (data.type === "so" ? " (C)" : " (.ccl)");
  document.getElementById("mMeta").textContent = "Yapımcı: @" + (data.author || "anonim") + "  |  Kullanım: import " + name + ";";
  document.getElementById("mCode").textContent = data.code || "// Kod bulunamadı";
  document.getElementById("modalBg").style.display = "flex";
}

function closeModal() { document.getElementById("modalBg").style.display = "none"; }

async function submitLib() {
  const msg = document.getElementById("msg");
  msg.className = "msg"; msg.style.display = "none";
  if (!authToken) { openAuthModal(); return; }
  const body = {
    name: document.getElementById("name").value.trim(),
    description: document.getElementById("desc").value.trim(),
    format: document.getElementById("fmt").value,
    code: document.getElementById("code").value
  };
  const res = await fetch("/api/libs", {
    method: "POST",
    headers: {"Content-Type": "application/json", "Authorization": "Bearer " + authToken},
    body: JSON.stringify(body)
  });
  const data = await res.json();
  if (res.ok) {
    msg.className = "msg ok"; msg.textContent = data.message || "Gönderildi!"; msg.style.display = "block";
    document.getElementById("formTitle").textContent = "📦 Yeni Kütüphane Gönder";
    loadLibs();
  } else {
    msg.className = "msg err"; msg.textContent = data.error || "Hata!"; msg.style.display = "block";
  }
}
loadLibs();
</script>
</body>
</html>"""


ADMIN_PAGE = """<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Admin Onay Paneli — ÇiçekliScript</title>
<style>
  :root { --bg: #090d16; --card: #161f30; --border: #263248; --text: #f1f5f9; --muted: #94a3b8; --accent: #38bdf8; --ok: #22c55e; --err: #ef4444; --warn: #f59e0b; }
  * { box-sizing: border-box; }
  body { margin: 0; font-family: system-ui, -apple-system, sans-serif; background: var(--bg); color: var(--text); padding: 24px; }
  .wrap { max-width: 980px; margin: 0 auto; }
  .header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
  h1 { margin: 0; font-size: 22px; color: var(--accent); }
  a { color: var(--muted); text-decoration: none; font-size: 13px; }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 20px; margin-bottom: 20px; }
  input { background: #0b1120; color: var(--text); border: 1px solid var(--border); border-radius: 8px; padding: 10px; font-size: 14px; }
  button { background: var(--accent); color: #090d16; border: none; border-radius: 8px; padding: 8px 14px; font-weight: 700; cursor: pointer; font-size: 13px; }
  .btn-ok { background: var(--ok); color: #052e16; }
  .btn-del { background: transparent; color: var(--err); border: 1px solid var(--err); }
  .item { border: 1px solid var(--border); border-radius: 8px; padding: 14px; margin-bottom: 12px; background: #0b1120; }
  .top { display: flex; justify-content: space-between; align-items: center; gap: 10px; flex-wrap: wrap; }
  .badge { font-size: 11px; padding: 2px 8px; border-radius: 999px; font-weight: 700; }
  .badge.pending { background: rgba(245,158,11,0.2); color: var(--warn); }
  .badge.approved { background: rgba(34,197,94,0.2); color: var(--ok); }
  pre { background: #111827; padding: 10px; border-radius: 6px; overflow-x: auto; font-size: 12px; color: #cbd5e1; max-height: 220px; }
</style>
</head>
<body>
<div class="wrap">
  <div class="header">
    <h1>🔐 Admin Onay Paneli</h1>
    <a href="/">← Ana Panele Dön</a>
  </div>
  <div id="loginBox" class="card">
    <p style="margin-top:0;color:var(--muted);">Devam etmek için admin şifresini girin:</p>
    <input type="password" id="pass" placeholder="Şifre" onkeydown="if(event.key==='Enter')login()" />
    <button onclick="login()">Giriş Yap</button>
    <span id="loginErr" style="color:var(--err);margin-left:10px;font-size:13px;"></span>
  </div>
  <div id="panel" style="display:none;">
    <div class="card">
      <h2 style="margin-top:0;font-size:17px;">⏳ Onay Bekleyen Yeni Kütüphaneler</h2>
      <div id="pendingList"></div>
    </div>
    <div class="card">
      <h2 style="margin-top:0;font-size:17px;">🔄 Onay Bekleyen Güncellemeler</h2>
      <div id="updateList"></div>
    </div>
    <div class="card">
      <h2 style="margin-top:0;font-size:17px;">✅ Onaylı Kütüphaneler</h2>
      <div id="approvedList"></div>
    </div>
  </div>
</div>
<script>
let adminPass = sessionStorage.getItem("ccl_admin_pass") || "";
if (adminPass) tryAutoLogin();

async function tryAutoLogin() {
  const res = await fetch("/api/admin/libs", { headers: { "X-Admin-Pass": adminPass } });
  if (res.ok) {
    document.getElementById("loginBox").style.display = "none";
    document.getElementById("panel").style.display = "block";
    renderLibs(await res.json());
  }
}

async function login() {
  adminPass = document.getElementById("pass").value;
  const res = await fetch("/api/admin/libs", { headers: { "X-Admin-Pass": adminPass } });
  if (res.ok) {
    sessionStorage.setItem("ccl_admin_pass", adminPass);
    document.getElementById("loginBox").style.display = "none";
    document.getElementById("panel").style.display = "block";
    renderLibs(await res.json());
  } else {
    document.getElementById("loginErr").textContent = "Hatalı şifre!";
  }
}

async function loadAdminLibs() {
  const res = await fetch("/api/admin/libs", { headers: { "X-Admin-Pass": adminPass } });
  if (res.ok) renderLibs(await res.json());
}

function esc(s) { return (s||"").replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;"); }

function renderLibs(libs) {
  const pend = libs.filter(l => l.status === "pending");
  const upd = libs.filter(l => l.pending_update);
  const appr = libs.filter(l => l.status !== "pending");

  document.getElementById("pendingList").innerHTML = pend.length ? pend.map(l => `
    <div class="item">
      <div class="top">
        <div><strong>${esc(l.name)}</strong> (@${esc(l.author)}) (${l.type}) <span class="badge pending">Onay Bekliyor</span></div>
        <div>
          <button class="btn-ok" onclick="approveLib('${esc(l.name)}')">Onayla</button>
          <button class="btn-del" onclick="delLib('${esc(l.name)}')">Reddet / Sil</button>
        </div>
      </div>
      <div style="color:var(--muted);font-size:13px;margin:6px 0;">${esc(l.description)}</div>
      <pre>${esc(l.code)}</pre>
    </div>
  `).join("") : "<p style='color:var(--muted)'>Onay bekleyen yeni kütüphane yok.</p>";

  document.getElementById("updateList").innerHTML = upd.length ? upd.map(l => `
    <div class="item">
      <div class="top">
        <div><strong>${esc(l.name)}</strong> (@${esc(l.author)}) <span class="badge pending">Güncelleme Bekliyor</span></div>
        <div>
          <button class="btn-ok" onclick="approveUpdate('${esc(l.name)}')">Güncellemeyi Onayla</button>
          <button class="btn-del" onclick="rejectUpdate('${esc(l.name)}')">Reddet</button>
        </div>
      </div>
      <div style="color:var(--muted);font-size:13px;margin:6px 0;">Yeni Açıklama: ${esc(l.pending_update.description)}</div>
      <pre>${esc(l.pending_update.code)}</pre>
    </div>
  `).join("") : "<p style='color:var(--muted)'>Onay bekleyen güncelleme yok.</p>";

  document.getElementById("approvedList").innerHTML = appr.length ? appr.map(l => `
    <div class="item">
      <div class="top">
        <div><strong>${esc(l.name)}</strong> (@${esc(l.author)}) (${l.type}) <span class="badge approved">Yayında</span></div>
        <button class="btn-del" onclick="delLib('${esc(l.name)}')">Sil</button>
      </div>
      <div style="color:var(--muted);font-size:13px;margin-top:4px;">${esc(l.description)}</div>
    </div>
  `).join("") : "<p style='color:var(--muted)'>Onaylı kütüphane yok.</p>";
}

async function approveLib(name) {
  await fetch("/api/admin/approve/" + name, { method: "POST", headers: { "X-Admin-Pass": adminPass } });
  loadAdminLibs();
}
async function approveUpdate(name) {
  await fetch("/api/admin/approve-update/" + name, { method: "POST", headers: { "X-Admin-Pass": adminPass } });
  loadAdminLibs();
}
async function rejectUpdate(name) {
  await fetch("/api/admin/reject-update/" + name, { method: "POST", headers: { "X-Admin-Pass": adminPass } });
  loadAdminLibs();
}
async function delLib(name) {
  if (!confirm(name + " silinsin mi?")) return;
  await fetch("/api/libs/" + name, { method: "DELETE", headers: { "X-Admin-Pass": adminPass } });
  loadAdminLibs();
}
</script>
</body>
</html>"""


class Handler(BaseHTTPRequestHandler):
    def _send_json(self, code, obj):
        b = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(b)))
        self.end_headers()
        self.wfile.write(b)

    def _send_html(self, html):
        b = html.encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(b)))
        self.end_headers()
        self.wfile.write(b)

    def _is_admin(self):
        return bool(ADMIN_PASS) and self.headers.get("X-Admin-Pass") == ADMIN_PASS

    def _get_user(self):
        auth = self.headers.get("Authorization", "")
        if auth.startswith("Bearer "):
            tok = auth[7:].strip()
            return TOKENS.get(tok)
        return None

    def do_GET(self):
        p = urlparse(self.path).path
        if p == "/":
            self._send_html(HTML_PAGE)
            return
        if p == "/admin":
            self._send_html(ADMIN_PAGE)
            return
        if p == "/api/libs":
            meta = load_json(META_FILE, {})
            out = [
                {
                    "name": k,
                    "type": v.get("type", "ccl"),
                    "description": v.get("description", ""),
                    "author": v.get("author", "anonim"),
                }
                for k, v in sorted(meta.items())
                if v.get("status", "approved") == "approved"
            ]
            self._send_json(200, out)
            return
        m_code = re.match(r"^/api/libs/([a-zA-Z0-9_]+)/code$", p)
        if m_code:
            name = m_code.group(1)
            meta = load_json(META_FILE, {})
            if name not in meta or meta[name].get("status", "approved") != "approved":
                self._send_json(404, {"error": "Not found"})
                return
            ltype = meta[name].get("type", "ccl")
            src_ext = ".c" if ltype == "so" else ".ccl"
            src_path = os.path.join(LIBS_DIR, name + src_ext)
            code = ""
            if os.path.exists(src_path):
                with open(src_path, "r", encoding="utf-8", errors="replace") as f:
                    code = f.read()
            self._send_json(200, {"name": name, "type": ltype, "author": meta[name].get("author", "anonim"), "code": code})
            return
        if p == "/api/admin/libs":
            if not self._is_admin():
                self._send_json(401, {"error": "Unauthorized"})
                return
            meta = load_json(META_FILE, {})
            out = []
            for k, v in sorted(meta.items()):
                ltype = v.get("type", "ccl")
                src_ext = ".c" if ltype == "so" else ".ccl"
                src_path = os.path.join(LIBS_DIR, k + src_ext)
                code = ""
                if os.path.exists(src_path):
                    with open(src_path, "r", encoding="utf-8", errors="replace") as f:
                        code = f.read()
                out.append({
                    "name": k,
                    "type": ltype,
                    "author": v.get("author", "anonim"),
                    "description": v.get("description", ""),
                    "status": v.get("status", "approved"),
                    "code": code,
                    "pending_update": v.get("pending_update"),
                })
            self._send_json(200, out)
            return
        m = re.match(r"^/api/libs/([a-zA-Z0-9_]+)/download$", p)
        if m:
            name = m.group(1)
            meta = load_json(META_FILE, {})
            if name not in meta or meta[name].get("status", "approved") != "approved":
                self._send_json(404, {"error": "Not found or not approved"})
                return
            ltype = meta[name].get("type", "ccl")
            ext = ".so" if ltype == "so" else ".ccl"
            fpath = os.path.join(LIBS_DIR, name + ext)
            if not os.path.exists(fpath):
                self._send_json(404, {"error": "File missing"})
                return
            with open(fpath, "rb") as f:
                data = f.read()
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("X-Lib-Type", ltype)
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
            return
        self._send_json(404, {"error": "Not found"})

    def do_POST(self):
        p = urlparse(self.path).path
        length = int(self.headers.get("Content-Length", 0))
        raw = self.rfile.read(length).decode("utf-8") if length > 0 else "{}"
        body = json.loads(raw) if raw.strip() else {}

        if p in ("/api/auth/register", "/api/auth/login"):
            uname = body.get("username", "").strip()
            pw = body.get("password", "")
            if not re.match(r"^[a-zA-Z0-9_]{3,24}$", uname) or len(pw) < 3:
                self._send_json(400, {"error": "Kullanıcı adı (3-24 harf/rakam) ve şifre (min 3) geçerli olmalı."})
                return
            users = load_json(USERS_FILE, {})
            if p.endswith("/register"):
                if uname in users:
                    self._send_json(400, {"error": "Bu kullanıcı adı zaten alınmış."})
                    return
                users[uname] = hash_pw(pw)
                save_json(USERS_FILE, users)
            else:
                if uname not in users or not verify_pw(pw, users[uname]):
                    self._send_json(401, {"error": "Hatalı kullanıcı adı veya şifre."})
                    return
            tok = secrets.token_hex(24)
            TOKENS[tok] = uname
            self._send_json(200, {"token": tok, "username": uname})
            return

        m = re.match(r"^/api/admin/approve/([a-zA-Z0-9_]+)$", p)
        if m:
            if not self._is_admin():
                self._send_json(401, {"error": "Unauthorized"})
                return
            name = m.group(1)
            meta = load_json(META_FILE, {})
            if name not in meta:
                self._send_json(404, {"error": "Not found"})
                return
            meta[name]["status"] = "approved"
            save_json(META_FILE, meta)
            self._send_json(200, {"ok": True})
            return

        m_upd = re.match(r"^/api/admin/approve-update/([a-zA-Z0-9_]+)$", p)
        if m_upd:
            if not self._is_admin():
                self._send_json(401, {"error": "Unauthorized"})
                return
            name = m_upd.group(1)
            meta = load_json(META_FILE, {})
            if name not in meta or not meta[name].get("pending_update"):
                self._send_json(404, {"error": "Not found"})
                return
            upd = meta[name].pop("pending_update")
            ltype = upd["type"]
            meta[name]["type"] = ltype
            meta[name]["description"] = upd["description"]
            if ltype == "so":
                c_path = os.path.join(LIBS_DIR, name + ".c")
                so_path = os.path.join(LIBS_DIR, name + ".so")
                with open(c_path, "w", encoding="utf-8") as f:
                    f.write(upd["code"])
                pend_so = os.path.join(LIBS_DIR, name + "_pending.so")
                if os.path.exists(pend_so):
                    os.replace(pend_so, so_path)
            else:
                ccl_path = os.path.join(LIBS_DIR, name + ".ccl")
                with open(ccl_path, "w", encoding="utf-8") as f:
                    f.write(upd["code"])
            save_json(META_FILE, meta)
            self._send_json(200, {"ok": True})
            return

        m_rej = re.match(r"^/api/admin/reject-update/([a-zA-Z0-9_]+)$", p)
        if m_rej:
            if not self._is_admin():
                self._send_json(401, {"error": "Unauthorized"})
                return
            name = m_rej.group(1)
            meta = load_json(META_FILE, {})
            if name in meta and "pending_update" in meta[name]:
                meta[name].pop("pending_update", None)
                save_json(META_FILE, meta)
            pend_so = os.path.join(LIBS_DIR, name + "_pending.so")
            if os.path.exists(pend_so):
                os.remove(pend_so)
            self._send_json(200, {"ok": True})
            return

        if p != "/api/libs":
            self._send_json(404, {"error": "Not found"})
            return

        user = self._get_user()
        if not user:
            self._send_json(401, {"error": "Giriş yapmalısınız!"})
            return

        name = body.get("name", "").strip()
        fmt = body.get("format", "ccl")
        desc = body.get("description", "").strip()
        code = body.get("code", "")
        if not re.match(r"^[a-zA-Z0-9_]+$", name):
            self._send_json(400, {"error": "Geçersiz kütüphane adı!"})
            return

        meta = load_json(META_FILE, {})
        is_existing_approved = name in meta and meta[name].get("status", "approved") == "approved"
        if is_existing_approved:
            if meta[name].get("author") not in (None, "anonim", user):
                self._send_json(403, {"error": "Bu kütüphane başka bir yapımcıya ait!"})
                return
            if fmt == "c":
                pend_c = os.path.join(LIBS_DIR, name + "_pending.c")
                pend_so = os.path.join(LIBS_DIR, name + "_pending.so")
                with open(pend_c, "w", encoding="utf-8") as f:
                    f.write(code)
                proc = subprocess.run(["gcc", "-shared", "-fPIC", "-O2", f"-I{INC_DIR}", pend_c, "-o", pend_so, "-lm"], capture_output=True, text=True)
                os.remove(pend_c)
                if proc.returncode != 0:
                    self._send_json(400, {"error": "GCC Derleme Hatası:\n" + proc.stderr})
                    return
                ltype = "so"
            else:
                ltype = "ccl"
            meta[name]["pending_update"] = {"type": ltype, "description": desc, "code": code}
            save_json(META_FILE, meta)
            self._send_json(200, {"message": f"{name} güncellemesi admin onayına gönderildi!"})
            return

        if fmt == "c":
            c_path = os.path.join(LIBS_DIR, name + ".c")
            so_path = os.path.join(LIBS_DIR, name + ".so")
            with open(c_path, "w", encoding="utf-8") as f:
                f.write(code)
            proc = subprocess.run(["gcc", "-shared", "-fPIC", "-O2", f"-I{INC_DIR}", c_path, "-o", so_path, "-lm"], capture_output=True, text=True)
            if proc.returncode != 0:
                self._send_json(400, {"error": "GCC Derleme Hatası:\n" + proc.stderr})
                return
            meta[name] = {"type": "so", "description": desc, "author": user, "status": "pending"}
        else:
            ccl_path = os.path.join(LIBS_DIR, name + ".ccl")
            with open(ccl_path, "w", encoding="utf-8") as f:
                f.write(code)
            meta[name] = {"type": "ccl", "description": desc, "author": user, "status": "pending"}
        save_json(META_FILE, meta)
        self._send_json(200, {"message": f"{name} onaya gönderildi! Admin onayından sonra yayınlanacaktır."})

    def do_DELETE(self):
        if not self._is_admin():
            self._send_json(401, {"error": "Unauthorized"})
            return
        m = re.match(r"^/api/libs/([a-zA-Z0-9_]+)$", urlparse(self.path).path)
        if not m:
            self._send_json(404, {"error": "Not found"})
            return
        name = m.group(1)
        meta = load_json(META_FILE, {})
        if name in meta:
            del meta[name]
            save_json(META_FILE, meta)
        for ext in (".ccl", ".c", ".so", "_pending.so"):
            fp = os.path.join(LIBS_DIR, name + ext)
            if os.path.exists(fp):
                os.remove(fp)
        self._send_json(200, {"ok": True})


if __name__ == "__main__":
    print("CCL Panel running on 0.0.0.0:8090")
    ThreadingHTTPServer(("0.0.0.0", 8090), Handler).serve_forever()
