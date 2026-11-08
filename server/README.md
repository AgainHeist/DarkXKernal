# Self-hosted license API

This small, dependency-free Python API implements the `/connect` form endpoint
used by the Android client. It stores only HMAC-SHA-256 hashes of license keys and device IDs in SQLite. A license binds to the first device that uses it.

> Deploy this only for software/services you own or are authorized to operate.
> The included listener is plain HTTP for local testing; put it behind a TLS
> reverse proxy (for example, Caddy or nginx) before using it over the internet.

## Run locally

Python 3.10+ is required; there are no pip packages to install.

```sh
cd server
export ADMIN_TOKEN="$(python3 -c 'import secrets; print(secrets.token_urlsafe(32))')"
export LICENSE_PEPPER="$(python3 -c 'import secrets; print(secrets.token_hex(32))')"
python3 app.py
```

By default it listens on `127.0.0.1:8080` and creates `licenses.sqlite3` beside
`app.py`. Configure `HOST`, `PORT`, and `LICENSE_DB` if needed. Keep both secrets
stable and private; changing `LICENSE_PEPPER` invalidates existing licenses.

## Create and revoke a license

Create one (the returned key is shown only once):

```sh
curl -X POST http://127.0.0.1:8080/admin/licenses \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"expires_at":"2030-01-01T00:00:00Z","seller":"Official"}'
```

Revoke it:

```sh
curl -X POST http://127.0.0.1:8080/admin/licenses/revoke \
  -H "Authorization: Bearer $ADMIN_TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"key":"PASTE-KEY-HERE"}'
```

`GET /health` is a basic health check. The app's `/connect` endpoint accepts
`application/x-www-form-urlencoded` fields `game`, `user_key`, `serial`, and
`version`. It responds with the JSON shape the current client parses:

```json
{"status":true,"data":{"expiry":"2030-01-01T00:00:00Z","seller":"Official","server_timestamp":1893456000}}
```

## Point the Android client at your deployment

After deploying behind HTTPS, edit `jni/src/Login/LoginConfig.h` and replace
`YOUR_DOMAIN` with your API host and path, for example:

```cpp
#define DARKX_LOGIN_SERVER_URL "https://api.example.com/connect"
```

The client now requires valid HTTPS certificates. For a production deployment,
use a domain with a valid TLS certificate; do not disable certificate checks.

The server currently implements license validation only. It deliberately does
not provide game offsets or other game-specific payloads.
