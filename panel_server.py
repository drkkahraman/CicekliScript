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
  h1 { margin: 0 0 6px; font-size: 24px; color: var(--accent); }
  .sub { color: var(--muted); margin-bottom: 24px; font-size: 14px; }
  .grid { display: grid; grid-template-columns: 1.1fr 0.9fr; gap: 20px; }
  @media (max-width: 850px) { .grid { grid-template-columns: 1fr; } }
  .card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 20px; }
  label { display: block; font-size: 13px; color: var(--muted); margin: 12px 0 6px; }
  input, select, textarea { width: 100%; background: #0b1220; color: var(--text); border: 1px solid var(--border); border-radius: 8px; padding: 10px; font-size: 14px; }
  textarea { font-family: ui-monospace, SFMono-Regular, Menlo, monospace; min-height: 240px; resize: vertical; }
  button { margin-top: 14px; background: var(--accent); color: #0f172a; border: none; border-radius: 8px; padding: 10px 18px; font-weight: 700; cursor: pointer; font-size: 14px; }
  button:hover { opacity: 0.9; }
  .btn-del { background: transparent; color: var(--err); border: 1px solid var(--err); padding: 4px 10px; font-size: 12px; margin: 0; }
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
  <h1>🌸 ÇiçekliScript — Comar Kütüphane Paneli</h1>
  <div class="sub">Kütüphane ekleyin (.ccl veya C) ve terminalden <code>comar install &lt;ad&gt;</code> ile indirin.</div>
  <div class="grid">
    <div class="card">
      <h2 style="margin-top:0;font-size:18px;">Yeni Kütüphane Ekle</h2>
      <label>Kütüphane Adı (harf, rakam, _)</label>
      <input id="name" placeholder="ör: mathx" />
      <label>Açıklama</label>
      <input id="desc" placeholder="Kısa açıklama..." />
      <label>Format</label>
      <select id="fmt" onchange="setTemplate()">
        <option value="ccl">ÇiçekliScript (.ccl)</option>
        <option value="c">C Kütüphanesi (.c -> .so)</option>
      </select>
      <label>Kaynak Kod</label>
      <textarea id="code"></textarea>
      <button onclick="submitLib()">Kaydet ve Yayınla</button>
      <div id="msg" class="msg"></div>
    </div>
    <div class="card">
      <h2 style="margin-top:0;font-size:18px;">Yayınlanan Kütüphaneler</h2>
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

async function loadLibs() {
  const res = await fetch("/api/libs");
  const libs = await res.json();
  const el = document.getElementById("list");
  if (!libs.length) { el.innerHTML = "<p style=color:#94a3b8>Henüz kütüphane yok.</p>"; return; }
  el.innerHTML = libs.map(l => `
    <div class="lib-item">
      <div class="lib-top">
        <div><strong>${l.name}</strong> <span class="badge ${l.type}">${l.type === "so" ? "C (.so)" : "CCL"}</span></div>
        <button class="btn-del" onclick="delLib('${l.name}')">Sil</button>
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
    msg.className = "msg ok"; msg.textContent = data.message || "Başarılı!"; msg.style.display = "block";
    loadLibs();
  } else {
    msg.className = "msg err"; msg.textContent = data.error || "Hata!"; msg.style.display = "block";
  }
}

async function delLib(name) {
  if (!confirm(name + " silinsin mi?")) return;
  await fetch("/api/libs/" + name, { method: "DELETE" });
  loadLibs();
}
loadLibs();
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

    def do_GET(self):
        p = urlparse(self.path).path
        if p == "/":
            b = HTML_PAGE.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(b)))
            self.end_headers()
            self.wfile.write(b)
            return
        if p == "/api/libs":
            meta = load_meta()
            out = [{"name": k, "type": v.get("type", "ccl"), "description": v.get("description", "")} for k, v in sorted(meta.items())]
            self._send_json(200, out)
            return
        m = re.match(r"^/api/libs/([a-zA-Z0-9_]+)/download$", p)
        if m:
            name = m.group(1)
            meta = load_meta()
            if name not in meta:
                self._send_json(404, {"error": "Not found"})
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
        if urlparse(self.path).path != "/api/libs":
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
            meta[name] = {"type": "so", "description": desc}
        else:
            ccl_path = os.path.join(LIBS_DIR, name + ".ccl")
            with open(ccl_path, "w", encoding="utf-8") as f:
                f.write(code)
            meta[name] = {"type": "ccl", "description": desc}
        save_meta(meta)
        self._send_json(200, {"message": f"{name} başarıyla yayınlandı!"})

    def do_DELETE(self):
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
