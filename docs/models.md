# Models

Canonical weights: [ShiWarai/kokoro-rknn-ru](https://huggingface.co/ShiWarai/kokoro-rknn-ru).

## Host layout

On the Orange Pi, keep Kokoro separate from other TTS models (Piper voices, etc.):

```
/home/orangepi/models/
  denis/ irina/ ruslan/ ...   # Piper (other project)
  kokoro-rknn-ru/             # Kokoro model weights only
    packs/base/               # sveta, masha
    packs/dima/               # dima
```

Russian **espeak-ng-data** is baked into the Docker image (`bin/espeak-ng-data/`), not stored on the models volume.

Set in `.env`:

```bash
KOKORO_MODELS_DIR=/home/orangepi/models/kokoro-rknn-ru
```

Inside the container the path is always `/models` (compose volume mount).

## Auto-download

On container start, `scripts/download_models.sh` fills the mounted volume with model weights:

- `KOKORO_DOWNLOAD_MODELS=ru` — packs from [ShiWarai/kokoro-rknn-ru](https://huggingface.co/ShiWarai/kokoro-rknn-ru)

Manual run:

```bash
MODELS_DIR=/home/orangepi/models/kokoro-rknn-ru ./scripts/download_models.sh ru
```

Custom files via `KOKORO_MODEL_URLS=relpath=url,...`.

## Packs

| Pack | Voices | Notes |
|------|--------|-------|
| `packs/base` | sveta, masha | default server pack |
| `packs/dima` | dima | separate fine-tune; use `--default-voice dima` |

## NPU devices (RK3588)

Compose passes through:

- `/dev/dri`
- `/dev/mpp_service`
- `/dev/rga`
- `/dev/dma_heap`

Container runs `privileged: true` for NPU access.

## Environment

| Variable | Description |
|----------|-------------|
| `KOKORO_MODELS_DIR` | Host path → `/models` in container |
| `KOKORO_DOWNLOAD_MODELS` | `ru` / `1` to auto-download weights; `0` to skip |
| `KOKORO_MODEL_URLS` | Optional `relpath=url` list |
| `KOKORO_DEFAULT_VOICE` | Default voice (`sveta`) |
