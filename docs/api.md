# API

Base URL: `http://127.0.0.1:8848` on the host (port from `PORT` in `.env`).

## Health

```http
GET /health
```

Response: `{"status":"ok"}`

Used by Docker `HEALTHCHECK` and k8s readiness probes.

## Synthesis

```http
POST /api/v1/synthesise
Content-Type: application/json

{
  "text": "Привет!",
  "voice": "sveta",
  "speed": 1.0,
  "audio_format": "wav"
}
```

`audio_format`: `opus` (default), `wav`, `pcm`, `raw`.

Phoneme input:

```json
{"phonemes": "...", "voice": "sveta"}
```

## Voices

```http
GET /api/v1/voices
```

Returns JSON array of voice names from the loaded pack.

```http
GET /api/v1/speakers
```

Paroli-compatible map `name → id`.

## WebSocket streaming

```
WS /api/v1/stream
```

Send JSON like synthesise body; receive binary opus/pcm chunks.

## OpenAI-compatible

```http
POST /v1/audio/speech
Content-Type: application/json

{
  "input": "Hello",
  "voice": "sveta",
  "response_format": "opus"
}
```

## Auth

If `KOKORO_TOKEN` is set, require:

```http
Authorization: Bearer <token>
```

## Web UI

Demo UI: **`http://<host>:8848/`** (same `kokoro-server` / Drogon process as the API).

Static files: `server/web-content/` in the image (`/opt/kokoro-rknn/server/web-content`).

Disable: `KOKORO_DISABLE_WEB_UI=1`.

## Examples

On the Orange Pi host:

```bash
curl -fsS http://127.0.0.1:8848/health

curl -fsS http://127.0.0.1:8848/api/v1/voices

curl -fsS -X POST http://127.0.0.1:8848/api/v1/synthesise \
  -H 'Content-Type: application/json' \
  -d '{"text":"Привет","voice":"sveta","audio_format":"wav"}' \
  -o out.wav
```
