#!/usr/bin/env python3
"""Small SQLite-backed license API compatible with the Android /connect client.

Run behind a TLS reverse proxy in production; do not expose this development HTTP
listener directly to the public internet.
"""
from __future__ import annotations

import hashlib
import hmac
import json
import os
import secrets
import sqlite3
import threading
import time
from datetime import datetime, timezone
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from contextlib import closing
from pathlib import Path
from urllib.parse import parse_qs, urlsplit

ROOT = Path(__file__).resolve().parent
DB_PATH = Path(os.environ.get("LICENSE_DB", ROOT / "licenses.sqlite3"))
EXPECTED_GAME = os.environ.get("EXPECTED_GAME", "FREEFIRE")
ADMIN_TOKEN = os.environ.get("ADMIN_TOKEN", "")
LICENSE_PEPPER = os.environ.get("LICENSE_PEPPER", "")
MAX_BODY_BYTES = 16 * 1024
_db_lock = threading.Lock()


def _db() -> sqlite3.Connection:
    DB_PATH.parent.mkdir(parents=True, exist_ok=True)
    conn = sqlite3.connect(DB_PATH, timeout=10)
    conn.row_factory = sqlite3.Row
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute(
        """CREATE TABLE IF NOT EXISTS licenses (
            key_hash TEXT PRIMARY KEY,
            expires_at TEXT NOT NULL,
            seller TEXT NOT NULL DEFAULT 'Official',
            active INTEGER NOT NULL DEFAULT 1,
            device_hash TEXT,
            created_at TEXT NOT NULL
        )"""
    )
    return conn


def _key_hash(key: str) -> str:
    return hmac.new(LICENSE_PEPPER.encode(), key.encode(), hashlib.sha256).hexdigest()


def _device_hash(device: str) -> str:
    return hmac.new(LICENSE_PEPPER.encode(), b"device:" + device.encode(), hashlib.sha256).hexdigest()


def _utc_now() -> datetime:
    return datetime.now(timezone.utc)


def _parse_expiry(value: str) -> datetime | None:
    try:
        parsed = datetime.fromisoformat(value.replace("Z", "+00:00"))
        if parsed.tzinfo is None:
            parsed = parsed.replace(tzinfo=timezone.utc)
        return parsed.astimezone(timezone.utc)
    except (TypeError, ValueError):
        return None


def create_license(expires_at: str, seller: str = "Official") -> str:
    expiry = _parse_expiry(expires_at)
    if expiry is None or expiry <= _utc_now():
        raise ValueError("expires_at must be a future ISO-8601 timestamp")
    key = "DXK-" + secrets.token_urlsafe(24)
    with _db_lock, closing(_db()) as conn, conn:
        conn.execute(
            "INSERT INTO licenses(key_hash, expires_at, seller, created_at) VALUES (?, ?, ?, ?)",
            (_key_hash(key), expiry.isoformat(), seller[:80] or "Official", _utc_now().isoformat()),
        )
    return key


def revoke_license(key: str) -> bool:
    with _db_lock, closing(_db()) as conn, conn:
        cur = conn.execute("UPDATE licenses SET active=0 WHERE key_hash=?", (_key_hash(key),))
        return cur.rowcount == 1


def authenticate(game: str, key: str, device: str) -> tuple[bool, dict[str, object]]:
    if not key or not device or len(key) > 256 or len(device) > 512:
        return False, {"message": "Invalid request"}
    if game != EXPECTED_GAME:
        return False, {"message": "Unsupported game"}

    with _db_lock, closing(_db()) as conn, conn:
        row = conn.execute("SELECT * FROM licenses WHERE key_hash=?", (_key_hash(key),)).fetchone()
        if row is None or not row["active"]:
            return False, {"message": "Invalid or revoked key"}
        expiry = _parse_expiry(row["expires_at"])
        if expiry is None or expiry <= _utc_now():
            return False, {"message": "License expired"}

        device_digest = _device_hash(device)
        existing = row["device_hash"]
        if existing and not hmac.compare_digest(existing, device_digest):
            return False, {"message": "Key is already activated on another device"}
        if not existing:
            conn.execute(
                "UPDATE licenses SET device_hash=? WHERE key_hash=? AND device_hash IS NULL",
                (device_digest, row["key_hash"]),
            )
            # Re-read after the conditional update to handle simultaneous first logins.
            bound = conn.execute(
                "SELECT device_hash FROM licenses WHERE key_hash=?", (row["key_hash"],)
            ).fetchone()["device_hash"]
            if not hmac.compare_digest(bound, device_digest):
                return False, {"message": "Key is already activated on another device"}

    return True, {
        "data": {
            "expiry": expiry.isoformat().replace("+00:00", "Z"),
            "seller": row["seller"],
            "server_timestamp": int(time.time()),
        }
    }


def _json_bytes(value: object) -> bytes:
    return json.dumps(value, separators=(",", ":")).encode("utf-8")


class Handler(BaseHTTPRequestHandler):
    server_version = "LicenseAPI/1.0"

    def log_message(self, fmt: str, *args: object) -> None:
        # Avoid logging request bodies or license keys.
        super().log_message(fmt, *args)

    def _send(self, status: int, value: object) -> None:
        body = _json_bytes(value)
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _read_body(self) -> bytes | None:
        try:
            length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            return None
        if length < 0 or length > MAX_BODY_BYTES:
            return None
        return self.rfile.read(length)

    def _admin_ok(self) -> bool:
        supplied = self.headers.get("Authorization", "")
        expected = "Bearer " + ADMIN_TOKEN
        return bool(ADMIN_TOKEN) and hmac.compare_digest(supplied, expected)

    def do_GET(self) -> None:
        if urlsplit(self.path).path == "/health":
            return self._send(200, {"ok": True})
        self._send(404, {"message": "Not found"})

    def do_POST(self) -> None:
        path = urlsplit(self.path).path
        body = self._read_body()
        if body is None:
            return self._send(413, {"status": False, "message": "Request too large"})

        if path == "/connect":
            form = parse_qs(body.decode("utf-8", errors="replace"), keep_blank_values=True)
            get = lambda name: form.get(name, [""])[0]
            ok, result = authenticate(get("game"), get("user_key"), get("serial"))
            # The Android client parses the JSON status, so auth failures stay HTTP 200.
            return self._send(200, {"status": ok, **result})

        if path == "/admin/licenses":
            if not self._admin_ok():
                return self._send(401, {"message": "Unauthorized"})
            try:
                payload = json.loads(body or b"{}")
                key = create_license(str(payload["expires_at"]), str(payload.get("seller", "Official")))
            except (ValueError, KeyError, TypeError, json.JSONDecodeError) as exc:
                return self._send(400, {"message": str(exc)})
            return self._send(201, {"key": key, "message": "Store this key now; it is not shown again."})

        if path == "/admin/licenses/revoke":
            if not self._admin_ok():
                return self._send(401, {"message": "Unauthorized"})
            try:
                payload = json.loads(body or b"{}")
                key = str(payload["key"])
            except (ValueError, KeyError, TypeError, json.JSONDecodeError):
                return self._send(400, {"message": "JSON body must contain key"})
            if not revoke_license(key):
                return self._send(404, {"message": "License not found"})
            return self._send(200, {"ok": True})

        self._send(404, {"message": "Not found"})


def main() -> None:
    if not ADMIN_TOKEN or len(ADMIN_TOKEN) < 24:
        raise SystemExit("Set ADMIN_TOKEN to a random value of at least 24 characters.")
    if not LICENSE_PEPPER or len(LICENSE_PEPPER) < 24:
        raise SystemExit("Set LICENSE_PEPPER to a random value of at least 24 characters.")
    host = os.environ.get("HOST", "127.0.0.1")
    port = int(os.environ.get("PORT", "8080"))
    _db().close()
    print(f"License API listening on http://{host}:{port}")
    ThreadingHTTPServer((host, port), Handler).serve_forever()


if __name__ == "__main__":
    main()
