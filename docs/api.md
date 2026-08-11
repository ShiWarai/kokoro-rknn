# API

Base URL: `http://<host>:8848` (port from `PORT` in `.env`). On the Orange Pi use `127.0.0.1`; from another machine use the Pi's LAN IP (`hostname -I`).

OpenAI clients: `OPENAI_BASE_URL=http://<host>:8848/v1`

## Health

```http
GET /health
```

Response: `{"status":"ok"}` — no auth.

## Model packs (dual NPU)

Both packs load at startup when present under `KOKORO_MODELS_DIR`:

| Pack | Model id | Voices |
|------|----------|--------|
| `packs/base` | `kokoro-base`, `tts-1` | sveta, masha |
| `packs/dima` | `kokoro-dima` | dima |

`tts-1` is an alias for the `base` pack (OpenAI SDK compatibility).

## OpenAI-compatible

### Models

```http
GET /v1/models
Authorization: Bearer <token>
```

```json
{
  "object": "list",
  "data": [
    {"id": "kokoro-base", "object": "model", "created": 0, "owned_by": "kokoro-rknn"},
    {"id": "kokoro-dima", "object": "model", "created": 0, "owned_by": "kokoro-rknn"},
    {"id": "tts-1", "object": "model", "created": 0, "owned_by": "kokoro-rknn"}
  ]
}
```

### Speech

```http
POST /v1/audio/speech
Content-Type: application/json
Authorization: Bearer <token>

{
  "model": "tts-1",
  "input": "Привет!",
  "voice": "sveta",
  "speed": 1.0,
  "response_format": "mp3"
}
```

| Field | Notes |
|-------|-------|
| `input` | Required, max 4096 chars |
| `voice` | `sveta`, `masha`, `dima` |
| `model` | `kokoro-base`, `kokoro-dima`, `tts-1` — optional; must match voice pack |
| `speed` | 0.25–4.0 |
| `response_format` | `mp3` (default), `opus`, `wav`, `pcm` — `aac`/`flac` → 400 |

Response: binary audio (`audio/mpeg` for mp3). Sample rate: **24 kHz** mono.

## Native API

```http
POST /api/v1/synthesise
Content-Type: application/json

{
  "text": "Привет!",
  "voice": "dima",
  "model": "kokoro-dima",
  "speed": 1.0,
  "audio_format": "wav"
}
```

`audio_format`: `opus` (default), `wav`, `pcm`, `mp3`.

```http
GET /api/v1/voices
GET /api/v1/speakers
```

`WS /api/v1/stream` — JSON body like synthesise; binary opus/pcm chunks. On connect, send one JSON message; receive binary audio chunks, then `{"status":"ok","message":"finished"}` or `{"status":"failed","message":"..."}`.

## Auth

When `KOKORO_TOKEN` or `OPENAI_API_KEY` is set:

```http
Authorization: Bearer <token>
```

Required for all API routes except `/health`. Invalid/missing key → **401**:

```json
{
  "error": {
    "message": "Incorrect API key provided",
    "type": "invalid_request_error",
    "code": "invalid_api_key"
  }
}
```

WebSocket auth: `Authorization: Bearer <token>` on the upgrade request, or query `?token=<token>` (browsers cannot set WS headers; the web UI uses `?token=`).

## Web UI

`http://<host>:8848/` — optional API token field (localStorage). Disable: `KOKORO_DISABLE_WEB_UI=1`.

## Examples

```bash
curl -fsS http://127.0.0.1:8848/health

curl -fsS http://127.0.0.1:8848/v1/models \
  -H "Authorization: Bearer $KOKORO_TOKEN"

curl -fsS -X POST http://127.0.0.1:8848/v1/audio/speech \
  -H "Authorization: Bearer $KOKORO_TOKEN" \
  -H 'Content-Type: application/json' \
  -d '{"model":"tts-1","input":"Привет","voice":"sveta","response_format":"mp3"}' \
  -o out.mp3

curl -fsS -X POST http://127.0.0.1:8848/api/v1/synthesise \
  -H 'Content-Type: application/json' \
  -d '{"text":"Привет","voice":"dima","model":"kokoro-dima","audio_format":"wav"}' \
  -o out.wav
```
