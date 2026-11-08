import os
import tempfile
import unittest
from datetime import datetime, timedelta, timezone
from pathlib import Path
from urllib.parse import urlencode
from urllib.request import Request, urlopen
from http.server import ThreadingHTTPServer
from threading import Thread

os.environ.setdefault("ADMIN_TOKEN", "test-admin-token-that-is-long-enough-123456")
os.environ.setdefault("LICENSE_PEPPER", "test-license-pepper-that-is-long-enough-123456")

from server import app


class LicenseApiTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        app.DB_PATH = Path(self.tmp.name) / "test.sqlite3"
        app.EXPECTED_GAME = "FREEFIRE"
        app._db().close()
        self.server = ThreadingHTTPServer(("127.0.0.1", 0), app.Handler)
        self.thread = Thread(target=self.server.serve_forever, daemon=True)
        self.thread.start()
        self.base = f"http://127.0.0.1:{self.server.server_port}"

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.thread.join(timeout=2)
        self.tmp.cleanup()

    def post_form(self, values):
        body = urlencode(values).encode()
        req = Request(self.base + "/connect", data=body, headers={
            "Content-Type": "application/x-www-form-urlencoded"
        })
        with urlopen(req, timeout=3) as response:
            import json
            return json.loads(response.read())

    def test_client_protocol_and_device_binding(self):
        expiry = (datetime.now(timezone.utc) + timedelta(days=2)).isoformat()
        key = app.create_license(expiry, "Test seller")
        form = {"game": "FREEFIRE", "user_key": key, "serial": "device-A", "version": "2.0"}

        ok = self.post_form(form)
        self.assertTrue(ok["status"])
        self.assertEqual(ok["data"]["seller"], "Test seller")
        self.assertIn("server_timestamp", ok["data"])

        self.assertTrue(self.post_form(form)["status"])
        form["serial"] = "device-B"
        denied = self.post_form(form)
        self.assertFalse(denied["status"])
        self.assertIn("another device", denied["message"])

    def test_wrong_game_and_revoked_key_are_denied(self):
        expiry = (datetime.now(timezone.utc) + timedelta(days=2)).isoformat()
        key = app.create_license(expiry)
        self.assertFalse(self.post_form({
            "game": "OTHER", "user_key": key, "serial": "device-A", "version": "2.0"
        })["status"])
        self.assertTrue(app.revoke_license(key))
        self.assertFalse(self.post_form({
            "game": "FREEFIRE", "user_key": key, "serial": "device-A", "version": "2.0"
        })["status"])

    def test_admin_license_creation_requires_token(self):
        import json
        expiry = (datetime.now(timezone.utc) + timedelta(days=5)).isoformat()
        payload = json.dumps({"expires_at": expiry, "seller": "Admin test"}).encode()
        req = Request(self.base + "/admin/licenses", data=payload, headers={
            "Content-Type": "application/json",
            "Authorization": "Bearer " + app.ADMIN_TOKEN,
        })
        with urlopen(req, timeout=3) as response:
            result = json.loads(response.read())
            self.assertEqual(response.status, 201)
        self.assertTrue(result["key"].startswith("DXK-"))


if __name__ == "__main__":
    unittest.main()
