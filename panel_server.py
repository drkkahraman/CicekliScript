#!/usr/bin/env python3
import os
import json
import re
import subprocess
from http.server import ThreadingHTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse

BASE_DIR = os.environ.get("CCL_PANEL_DIR", "/home/doruk/ccl-panel")
LIBS_DIR = os.path.join(BASE_DIR, "libs")
INC_DIR = os.path.join(BASE_DIR, "include")
META_FILE = os.path.join(LIBS_DIR, "meta.json")
ADMIN_PASS = os.environ.get("CCL_ADMIN_PASS", "")
os.makedirs(LIBS_DIR, exist_ok=True)


def load_meta():
    if os.path.exists(META_FILE):
        with open(META_FILE, "r", encoding="utf-8") as f:
            return json.load(f)
    return {}


def save_meta(meta):
    with open(META_FILE, "w", encoding="utf-8") as f:
        json.dump(meta, f, ensure_ascii=False, indent=2)


HTML_PAGE = """<!DOCTYPE html>
<html lang="tr">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ÇiçekliScript — Comar Kütüphane Paneli</title>
<style>
  :root { --bg: #0f172a; --card: #1e293b; --border: #334155; --text: #e2e8f0; --muted: #94a3b8; --accent: #38bdf8; --ok: #22c55e; --err: #ef4444; }
  * { box-sizing: border-box; }
  body { margin: 0; font-family: system-ui, -apple-system, sans-serif; background: var(--bg); color: var(--text); padding: 24px; }
  .wrap { max-width: 1100px; margin: 0 auto; }
  .header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 6px; }
  h1 { margin: 0; font-size: 24px; color: var(--accent); }
  .admin-link { color: var(--muted); text-decoration: none; font-size: 13px; border: 1px solid var(--border); padding: 6px 12px; border-radius: 8px; }
  .admin-link:hover { color: var(--text); border-color: var(--accent); }
  .sub { color: var(--muted); margin-bottom: 24px; font-size: 14px; }
  .grid { display: grid; grid-template-columns: 1.1fr 0.9fr; gap: 20px; }
  @media (max-width: 850px) { .grid { grid-template-columns: 1fr; } }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 20px; }
  label { display: block; font-size: 13px; color: var(--muted); margin: 12px 0 6px; }
  input, select, textarea { width: 100%; background: #0b1220; color: var(--text); border: 1px solid var(--border); border-radius: 8px; padding: 10px; font-size: 14px; }
  textarea { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; min-height: 220px; resize: vertical; }
  button { margin-top: 14px; background: var(--accent); color: #0f172a; border: none; border-radius: 8px; padding: 10px 18px; font-weight: 700; cursor: pointer; font-size: 14px; }
  button:hover { opacity: 0.9; }
  .msg { margin-top: 12px; padding: 10px; border-radius: 8px; font-size: 13px; white-space: pre-wrap; display: none; }
  .msg.ok { display: block; background: rgba(34,197,94,0.15); border: 1px solid var(--ok); color: #86efac; }
  .msg.err { display: block; background: rgba(239,68,68,0.15); border: 1px solid var(--err); color: #fca5a5; }
  .lib-item { border: 1px solid var(--border); border-radius: 8px; padding: 12px; margin-bottom: 10px; background: #0b1220; }
  .lib-top { display: flex; justify-content: space-between; align-items: center; }
  .badge { font-size: 11px; padding: 2px 8px; border-radius: 999px; font-weight: 700; }
  .badge.ccl { background: rgba(56,189,248,0.2); color: var(--accent); }
  .badge.so { background: rgba(34,197,94,0.2); color: var(--ok); }
  code { background: #1e293b; padding: 2px 6px; border-radius: 4px; font-size: 12px; color: #cbd5e1; }
</style>
</head>
<body>
<div class="wrap">
  <div class="header">
    <h1>🌸 ÇiçekliScript — Comar Kütüphane Paneli</h1>
    <a class="admin-link" href="/admin">🔐 Admin Onay Paneli</a>
  </div>
  <div class="sub">Kütüphane gönderin (.ccl veya C). Admin onayından sonra <code>comar install &lt;ad&gt;</code> ile indirilebilir.</div>
  <div class="grid">
    <div class="card">
      <h2 style="margin-top:0;font-size:18px;">Yeni Kütüphane Gönder</h2>
      <label>Kütüphane Adı (harf, rakam, _)</label>
      <input id="name" placeholder="ör: mathx" />
      <label>Açıklama</label>
      <input id="desc" placeholder="Kısa açıklama..." />
      <label>Format</label>
      <select id="fmt" onchange="setTemplate()">
        <option value="ccl">ÇiçekliScript (.ccl)</option>
        <option value="c">C Kütüphanesi (.c -> .so)</option>
      </select>
      <label>Dosyadan Seç (.ccl / .c)</label>
      <input type="file" id="fileInput" accept=".ccl,.c,.ms" onchange="loadFromFile(event)" />
      <label>Kaynak Kod</label>
      <textarea id="code"></textarea>
      <button onclick="submitLib()">Onaya Gönder</button>
      <div id="msg" class="msg"></div>
    </div>
    <div class="card">
      <h2 style="margin-top:0;font-size:18px;">Onaylı Kütüphaneler</h2>
      <div id="list">Yükleniyor...</div>
    </div>
  </div>
</div>
<script>
const TPL_CCL = `// ÇiçekliScript Kütüphanesi
def topla(a, b) {
    return a + b;
}`;
const TPL_C = `#include "ccl_plugin.h"

static CCLVal c_topla(int argc, CCLVal* argv) {
    if (argc < 2) return ccl_nil();
    return ccl_num(ccl_as_num(argv[0]) + ccl_as_num(argv[1]));
}

void ccl_init(CCLRegisterFn reg) {
    reg("c_topla", c_topla, 2);
}`;
function setTemplate() {
  const fmt = document.getElementById("fmt").value;
  const ta = document.getElementById("code");
  if (!ta.value.trim() || ta.value === TPL_CCL || ta.value === TPL_C) {
    ta.value = fmt === "c" ? TPL_C : TPL_CCL;
  }
}
setTemplate();

function loadFromFile(e) {
  const file = e.target.files[0];
  if (!file) return;
  const ext = file.name.split(".").pop().toLowerCase();
  if (ext === "c") document.getElementById("fmt").value = "c";
  else document.getElementById("fmt").value = "ccl";
  const nameEl = document.getElementById("name");
  if (!nameEl.value.trim()) {
    nameEl.value = file.name.replace(/\.[^.]+$/, "").replace(/[^a-zA-Z0-9_]/g, "_");
  }
  const reader = new FileReader();
  reader.onload = ev => { document.getElementById("code").value = ev.target.result; };
  reader.readAsText(file);
}

async function loadLibs() {
  const res = await fetch("/api/libs");
  const libs = await res.json();
  const el = document.getElementById("list");
  if (!libs.length) { el.innerHTML = "<p style=color:#94a3b8>Henüz onaylı kütüphane yok.</p>"; return; }
  el.innerHTML = libs.map(l => `
    <div class="lib-item">
      <div class="lib-top">
        <div><strong>${l.name}</strong> <span class="badge ${l.type}">${l.type === "so" ? "C (.so)" : "CCL"}</span></div>
      </div>
      <div style="color:#94a3b8;font-size:13px;margin:6px 0;">${l.description || ""}</div>
      <code>comar install ${l.name}</code>
    </div>
  `).join("");
}

async function submitLib() {
  const msg = document.getElementById("msg");
  msg.className = "msg"; msg.style.display = "none";
  const body = {
    name: document.getElementById("name").value.trim(),
    description: document.getElementById("desc").value.trim(),
    format: document.getElementById("fmt").value,
    code: document.getElementById("code").value
  };
  const res = await fetch("/api/libs", { method: "POST", headers: {"Content-Type": "application/json"}, body: JSON.stringify(body) });
  const data = await res.json();
  if (res.ok) {
    msg.className = "msg ok"; msg.textContent = data.message || "Gönderildi!"; msg.style.display = "block";
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
  :root { --bg: #0f172a; --card: #1e293b; --border: #334155; --text: #e2e8f0; --muted: #94a3b8; --accent: #38bdf8; --ok: #22c55e; --err: #ef4444; --warn: #f59e0b; }
  * { box-sizing: border-box; }
  body { margin: 0; font-family: system-ui, -apple-system, sans-serif; background: var(--bg); color: var(--text); padding: 24px; }
  .wrap { max-width: 960px; margin: 0 auto; }
  .header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; }
  h1 { margin: 0; font-size: 22px; color: var(--accent); }
  a { color: var(--muted); text-decoration: none; font-size: 13px; }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 20px; margin-bottom: 20px; }
  input { background: #0b1220; color: var(--text); border: 1px solid var(--border); border-radius: 8px; padding: 10px; font-size: 14px; }
  button { background: var(--accent); color: #0f172a; border: none; border-radius: 8px; padding: 8px 14px; font-weight: 700; cursor: pointer; font-size: 13px; }
  .btn-ok { background: var(--ok); color: #052e16; }
  .btn-del { background: transparent; color: var(--err); border: 1px solid var(--err); }
  .item { border: 1px solid var(--border); border-radius: 8px; padding: 14px; margin-bottom: 12px; background: #0b1220; }
  .top { display: flex; justify-content: space-between; align-items: center; gap: 10px; }
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
      <h2 style="margin-top:0;font-size:17px;">⏳ Onay Bekleyen Kütüphaneler</h2>
      <div id="pendingList"></div>
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
  const appr = libs.filter(l => l.status !== "pending");
  document.getElementById("pendingList").innerHTML = pend.length ? pend.map(l => `
    <div class="item">
      <div class="top">
        <div><strong>${esc(l.name)}</strong> (${l.type}) <span class="badge pending">Onay Bekliyor</span></div>
        <div>
          <button class="btn-ok" onclick="approveLib('${esc(l.name)}')">Onayla</button>
          <button class="btn-del" onclick="delLib('${esc(l.name)}')">Reddet / Sil</button>
        </div>
      </div>
      <div style="color:var(--muted);font-size:13px;margin:6px 0;">${esc(l.description)}</div>
      <pre>${esc(l.code)}</pre>
    </div>
  `).join("") : "<p style='color:var(--muted)'>Onay bekleyen kütüphane yok.</p>";

  document.getElementById("approvedList").innerHTML = appr.length ? appr.map(l => `
    <div class="item">
      <div class="top">
        <div><strong>${esc(l.name)}</strong> (${l.type}) <span class="badge approved">Yayında</span></div>
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
        return self.headers.get("X-Admin-Pass") == ADMIN_PASS

    def do_GET(self):
        p = urlparse(self.path).path
        if p == "/":
            self._send_html(HTML_PAGE)
            return
        if p == "/admin":
            self._send_html(ADMIN_PAGE)
            return
        if p == "/api/libs":
            meta = load_meta()
            out = [
                {"name": k, "type": v.get("type", "ccl"), "description": v.get("description", "")}
                for k, v in sorted(meta.items())
                if v.get("status", "approved") == "approved"
            ]
            self._send_json(200, out)
            return
        if p == "/api/admin/libs":
            if not self._is_admin():
                self._send_json(401, {"error": "Unauthorized"})
                return
            meta = load_meta()
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
                    "description": v.get("description", ""),
                    "status": v.get("status", "approved"),
                    "code": code,
                })
            self._send_json(200, out)
            return
        m = re.match(r"^/api/libs/([a-zA-Z0-9_]+)/download$", p)
        if m:
            name = m.group(1)
            meta = load_meta()
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
        m = re.match(r"^/api/admin/approve/([a-zA-Z0-9_]+)$", p)
        if m:
            if not self._is_admin():
                self._send_json(401, {"error": "Unauthorized"})
                return
            name = m.group(1)
            meta = load_meta()
            if name not in meta:
                self._send_json(404, {"error": "Not found"})
                return
            meta[name]["status"] = "approved"
            save_meta(meta)
            self._send_json(200, {"ok": True})
            return

        if p != "/api/libs":
            self._send_json(404, {"error": "Not found"})
            return
        length = int(self.headers.get("Content-Length", 0))
        body = json.loads(self.rfile.read(length).decode("utf-8"))
        name = body.get("name", "").strip()
        fmt = body.get("format", "ccl")
        desc = body.get("description", "").strip()
        code = body.get("code", "")
        if not re.match(r"^[a-zA-Z0-9_]+$", name):
            self._send_json(400, {"error": "Geçersiz kütüphane adı!"})
            return
        meta = load_meta()
        if fmt == "c":
            c_path = os.path.join(LIBS_DIR, name + ".c")
            so_path = os.path.join(LIBS_DIR, name + ".so")
            with open(c_path, "w", encoding="utf-8") as f:
                f.write(code)
            proc = subprocess.run(["gcc", "-shared", "-fPIC", "-O2", f"-I{INC_DIR}", c_path, "-o", so_path, "-lm"], capture_output=True, text=True)
            if proc.returncode != 0:
                self._send_json(400, {"error": "GCC Derleme Hatası:\n" + proc.stderr})
                return
            meta[name] = {"type": "so", "description": desc, "status": "pending"}
        else:
            ccl_path = os.path.join(LIBS_DIR, name + ".ccl")
            with open(ccl_path, "w", encoding="utf-8") as f:
                f.write(code)
            meta[name] = {"type": "ccl", "description": desc, "status": "pending"}
        save_meta(meta)
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
        meta = load_meta()
        if name in meta:
            del meta[name]
            save_meta(meta)
        for ext in (".ccl", ".c", ".so"):
            fp = os.path.join(LIBS_DIR, name + ext)
            if os.path.exists(fp):
                os.remove(fp)
        self._send_json(200, {"ok": True})


if __name__ == "__main__":
    print("CCL Panel running on 0.0.0.0:8090")
    ThreadingHTTPServer(("0.0.0.0", 8090), Handler).serve_forever()
