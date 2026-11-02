#!/usr/bin/env python3
import http.server
import socketserver
import threading
import urllib.parse
import urllib.request
import webbrowser
from pathlib import Path

DIR = Path(__file__).resolve().parent
PORT = 8765

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(DIR), **kwargs)

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        super().end_headers()

    def do_GET(self):
        if self.path == "/" or self.path.startswith("/?"):
            self.path = "/OfflineFieldUpdater.html"
            return super().do_GET()
        if self.path.startswith("/api/read"):
            qs = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
            ip = qs.get("ip", ["192.168.4.1"])[0]
            try:
                with urllib.request.urlopen(f"http://{ip}/settingsValues", timeout=4) as r:
                    data = r.read()
                self.send_response(200)
                self.send_header("Content-Type", "text/plain; charset=utf-8")
                self.end_headers()
                self.wfile.write(data)
            except Exception as e:
                self.send_response(502)
                self.end_headers()
                self.wfile.write(str(e).encode("utf-8"))
            return
        return super().do_GET()

    def log_message(self, format, *args):
        pass

if __name__ == "__main__":
    socketserver.TCPServer.allow_reuse_address = True
    try:
        httpd = socketserver.TCPServer(("127.0.0.1", PORT), Handler)
        url = f"http://127.0.0.1:{PORT}/OfflineFieldUpdater.html"
        print(f"Field Updater UI running at {url}")
        threading.Timer(0.4, lambda: webbrowser.open(url)).start()
        httpd.serve_forever()
    except OSError:
        webbrowser.open(f"http://127.0.0.1:{PORT}/OfflineFieldUpdater.html")
