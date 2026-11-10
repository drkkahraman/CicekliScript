#!/usr/bin/env python3
import codecs
import json
import os
import subprocess
import sys
import tempfile
import threading
import time
import uuid
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DOCS_DIR = os.path.join(BASE_DIR, "docs")
BIN_PATH = os.path.join(BASE_DIR, "bin", "cicekliscript")
if not os.path.isfile(BIN_PATH):
    fallback_bin = os.path.join(BASE_DIR, "bin", "metescript")
    if os.path.isfile(fallback_bin):
        BIN_PATH = fallback_bin

# --- Etkileşimli oturum yönetimi -----------------------------------------
SESSIONS = {}
SESSION_LOCK = threading.Lock()
MAX_SESSIONS = 16
SESSION_MAX_TOTAL = 120.0      # bir oturum en fazla bu kadar süre yaşayabilir (sn)
SESSION_MAX_IDLE = 30.0        # bu kadar süredir girdi/çıktı olmayan oturum sonlandırılır
REAP_INTERVAL = 5.0            # temizlik döngüsü
IDLE_KILL_REASON = "Zaman aşımı: oturum çok uzun süre hareketsiz kaldı."
TOTAL_KILL_REASON = "Zaman aşımı: oturum en fazla çalışma süresini aştı."

class CicekliScriptHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DOCS_DIR, **kwargs)

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def _send_json(self, status, payload):
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(json.dumps(payload, ensure_ascii=False).encode("utf-8"))

    def do_POST(self):
        if self.path == "/api/run":
            content_length = int(self.headers.get("Content-Length", 0))
            post_data = self.rfile.read(content_length)
            try:
                data = json.loads(post_data.decode("utf-8"))
                code = data.get("code", "")
                user_stdin = data.get("stdin", "")
            except Exception as e:
                self.send_response(400)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                self.wfile.write(json.dumps({"error": f"Geçersiz JSON: {str(e)}"}).encode("utf-8"))
                return

            if not code.strip():
                self.send_response(200)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                self.wfile.write(json.dumps({
                    "stdout": "",
                    "stderr": "Uyarı: Çalıştırılacak kod boş!",
                    "exit_code": 0,
                    "execution_time_ms": 0
                }).encode("utf-8"))
                return

            if len(code) > 65536:
                self.send_response(400)
                self.send_header("Content-Type", "application/json; charset=utf-8")
                self.send_header("Access-Control-Allow-Origin", "*")
                self.end_headers()
                self.wfile.write(json.dumps({"error": "Kod boyutu çok büyük (maksimum 64KB)"}).encode("utf-8"))
                return

            start_time = time.time()
            temp_filename = ""
            try:
                with tempfile.NamedTemporaryFile("w", suffix=".ccl", delete=False) as tf:
                    tf.write(code)
                    temp_filename = tf.name

                proc = subprocess.run(
                    [BIN_PATH, temp_filename],
                    input=user_stdin,
                    capture_output=True,
                    text=True,
                    timeout=5,
                    env=_child_env(),
                )
                exec_time_ms = round((time.time() - start_time) * 1000, 2)
                resp = {
                    "stdout": proc.stdout,
                    "stderr": proc.stderr,
                    "exit_code": proc.returncode,
                    "execution_time_ms": exec_time_ms
                }
            except subprocess.TimeoutExpired:
                exec_time_ms = round((time.time() - start_time) * 1000, 2)
                resp = {
                    "stdout": "",
                    "stderr": "Zaman Aşımı Hatası: Kod 5 saniye içinde tamamlanamadı (sonsuz döngü olabilir).",
                    "exit_code": -1,
                    "execution_time_ms": exec_time_ms
                }
            except Exception as e:
                exec_time_ms = round((time.time() - start_time) * 1000, 2)
                resp = {
                    "stdout": "",
                    "stderr": f"Sistem hatası: {str(e)}",
                    "exit_code": -1,
                    "execution_time_ms": exec_time_ms
                }
            finally:
                if temp_filename and os.path.exists(temp_filename):
                    try:
                        os.remove(temp_filename)
                    except OSError:
                        pass

            self.send_response(200)
            self.send_header("Content-Type", "application/json; charset=utf-8")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(json.dumps(resp, ensure_ascii=False).encode("utf-8"))
        elif self.path == "/api/interactive/start":
            content_length = int(self.headers.get("Content-Length", 0))
            try:
                data = json.loads(self.rfile.read(content_length).decode("utf-8"))
                code = data.get("code", "")
                initial_stdin = data.get("stdin", "") or ""
            except Exception as e:
                self._send_json(400, {"error": f"Geçersiz JSON: {str(e)}"})
                return

            if not code.strip():
                self._send_json(400, {"error": "Kod boş olamaz."})
                return
            if len(code) > 65536:
                self._send_json(400, {"error": "Kod boyutu çok büyük (maksimum 64KB)"})
                return

            try:
                sid, _session = _spawn_interactive(code, initial_stdin)
            except Exception as e:
                self._send_json(500, {"error": f"İşlem başlatılamadı: {str(e)}"})
                return
            self._send_json(200, {"id": sid})

        elif self.path.startswith("/api/interactive/") and self.path.endswith("/input"):
            sid = self.path[len("/api/interactive/"):-len("/input")]
            try:
                data = json.loads(self.rfile.read(int(self.headers.get("Content-Length", 0))).decode("utf-8"))
                user_input = data.get("input", "")
            except Exception:
                user_input = ""

            with SESSION_LOCK:
                session = SESSIONS.get(sid)
            if not session or session["proc"].poll() is not None:
                self._send_json(404, {"error": "Oturum bulunamadı veya sona erdi."})
                return
            try:
                session["proc"].stdin.write(user_input.encode("utf-8"))
                session["proc"].stdin.flush()
                session["last_activity"] = time.time()
                self._send_json(200, {"ok": True})
            except (BrokenPipeError, OSError):
                self._send_json(410, {"error": "Süreç girdi kabul etmiyor (sonlanmış olabilir)."})
        elif self.path.startswith("/api/interactive/") and self.path.endswith("/stop"):
            sid = self.path[len("/api/interactive/"):-len("/stop")]
            with SESSION_LOCK:
                _close_session(sid)
            self._send_json(200, {"ok": True})
        else:
            self.send_error(404, "Not Found")

    def do_GET(self):
        if self.path.startswith("/api/interactive/") and self.path.endswith("/poll"):
            sid = self.path[len("/api/interactive/"):-len("/poll")]
            with SESSION_LOCK:
                session = SESSIONS.get(sid)
            if not session:
                self._send_json(404, {"error": "Oturum bulunamadı."})
                return

            proc = session["proc"]
            new_stdout = ""
            new_stderr = ""
            if not session["stdout_state"]["eof"]:
                new_stdout = _drain_pipe(proc, proc.stdout, session["stdout_state"])
            if not session["stderr_state"]["eof"]:
                new_stderr = _drain_pipe(proc, proc.stderr, session["stderr_state"])

            rc = proc.poll()
            if rc is not None and session["ended_at"] is None:
                session["ended_at"] = time.time()

            self._send_json(200, {
                "stdout": new_stdout,
                "stderr": new_stderr,
                "running": rc is None,
                "exit_code": rc,
                "killed": session["killed"],
                "kill_reason": session["kill_reason"],
            })
        else:
            super().do_GET()

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()

def _child_env():
    env = dict(os.environ)
    env["CICEKLI_GUI_HEADLESS"] = "1"  # GUI kodları web önizlemesi için komut dökümü yazar
    env["METE_GUI_HEADLESS"] = "1"
    return env


def _drain_pipe(proc, stream, state):
    """Eşzamansız olmayan engellemesiz okuma: mevcut olan tüm veriyi döndürür."""
    chunks = []
    try:
        while True:
            data = os.read(stream.fileno(), 8192)
            if not data:
                state["eof"] = True
                break
            chunks.append(data)
    except BlockingIOError:
        pass
    except OSError:
        state["eof"] = True
    if chunks:
        return state["decoder"].decode(b"".join(chunks))
    return ""


def _remove_tmpfile(path):
    if path:
        try:
            os.remove(path)
        except OSError:
            pass


def _close_session(sid):
    """Oturumu listeden çıkarır, süreç kapatır ve geçici dosyayı siler."""
    session = SESSIONS.pop(sid, None)
    if not session:
        return None
    try:
        proc = session["proc"]
        if proc.poll() is None:
            proc.kill()
        for f in (proc.stdout, proc.stderr, proc.stdin):
            try:
                f.close()
            except Exception:
                pass
        proc.wait(timeout=3)
    except Exception:
        pass
    _remove_tmpfile(session.get("tmpfile"))
    return session


def _session_reaper():
    while True:
        time.sleep(REAP_INTERVAL)
        now = time.time()
        with SESSION_LOCK:
            for sid in list(SESSIONS):
                s = SESSIONS[sid]
                proc = s["proc"]
                rc = proc.poll()
                if rc is not None:
                    if s["ended_at"] is None:
                        s["ended_at"] = now
                    elif now - s["ended_at"] > 60:
                        _close_session(sid)
                    continue
                if s.get("killed"):
                    continue
                if (now - s["last_activity"] > SESSION_MAX_IDLE
                        or now - s["started_at"] > SESSION_MAX_TOTAL):
                    s["killed"] = True
                    s["kill_reason"] = (TOTAL_KILL_REASON
                                        if now - s["started_at"] > SESSION_MAX_TOTAL
                                        else IDLE_KILL_REASON)
                    try:
                        proc.kill()
                        proc.wait(timeout=3)
                    except Exception:
                        pass
                    _remove_tmpfile(s.get("tmpfile"))


def _spawn_interactive(code, initial_stdin=""):
    tf = tempfile.NamedTemporaryFile("w", suffix=".ccl", delete=False)
    tf.write(code)
    tf.close()
    proc = subprocess.Popen(
        [BIN_PATH, tf.name],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=_child_env(),
    )

    os.set_blocking(proc.stdout.fileno(), False)
    os.set_blocking(proc.stderr.fileno(), False)

    session = {
        "proc": proc,
        "started_at": time.time(),
        "last_activity": time.time(),
        "ended_at": None,
        "killed": False,
        "kill_reason": "",
        "tmpfile": tf.name,
        "stdout_state": {"decoder": codecs.getincrementaldecoder("utf-8")(errors="replace"), "eof": False},
        "stderr_state": {"decoder": codecs.getincrementaldecoder("utf-8")(errors="replace"), "eof": False},
    }

    if initial_stdin:
        try:
            proc.stdin.write(initial_stdin.encode("utf-8"))
            proc.stdin.flush()
        except (BrokenPipeError, OSError):
            pass

    with SESSION_LOCK:
        sid = str(uuid.uuid4())
        SESSIONS[sid] = session
        # Limit aşılırsa en eski oturumu sonlandır
        if len(SESSIONS) > MAX_SESSIONS:
            oldest = min(SESSIONS, key=lambda k: SESSIONS[k]["started_at"])
            _close_session(oldest)
    return sid, session


def _serve_port(port):
    try:
        ThreadingHTTPServer.allow_reuse_address = True
        server = ThreadingHTTPServer(("0.0.0.0", port), CicekliScriptHandler)
        print(f"CicekliScript Dokümantasyon & Online Compiler sunucusu çalışıyor: http://0.0.0.0:{port}")
        server.serve_forever()
    except Exception as e:
        print(f"Port {port} üzerinde sunucu başlatılamadı: {e}", file=sys.stderr)

def run(port=6000):
    reaper = threading.Thread(target=_session_reaper, daemon=True)
    reaper.start()

    # Port 6001'i de paralel başlat
    if port == 6000:
        t_alt = threading.Thread(target=_serve_port, args=(6001,), daemon=True)
        t_alt.start()
    elif port == 6001:
        t_alt = threading.Thread(target=_serve_port, args=(6000,), daemon=True)
        t_alt.start()

    _serve_port(port)

if __name__ == "__main__":
    p = int(sys.argv[1]) if len(sys.argv) > 1 else 6000
    run(p)
