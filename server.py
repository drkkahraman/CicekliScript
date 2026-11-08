#!/usr/bin/env python3
import json
import os
import subprocess
import sys
import tempfile
import time
from http.server import HTTPServer, SimpleHTTPRequestHandler

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DOCS_DIR = os.path.join(BASE_DIR, "docs")
BIN_PATH = os.path.join(BASE_DIR, "bin", "metescript")

class MeteScriptHandler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DOCS_DIR, **kwargs)

    def do_OPTIONS(self):
        self.send_response(200)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

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
                with tempfile.NamedTemporaryFile("w", suffix=".ms", delete=False) as tf:
                    tf.write(code)
                    temp_filename = tf.name

                proc = subprocess.run(
                    [BIN_PATH, temp_filename],
                    input=user_stdin,
                    capture_output=True,
                    text=True,
                    timeout=5
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
        else:
            self.send_error(404, "Not Found")

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()

def run(port=6001):
    server = HTTPServer(("0.0.0.0", port), MeteScriptHandler)
    print(f"MeteScript Dokümantasyon & Online Compiler sunucusu çalışıyor: http://0.0.0.0:{port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()

if __name__ == "__main__":
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 6001
    run(port)
