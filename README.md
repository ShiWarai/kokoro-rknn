# kokoro-rknn

Fast Kokoro-82M TTS for **Rockchip RK3588 NPU** (encoder ONNX + decoder RKNN). Russian voices from [ShiWarai/kokoro-rknn-ru](https://huggingface.co/ShiWarai/kokoro-rknn-ru).

**Docker-first** runtime (like [whisper-rknn](https://github.com/ShiWarai/whisper-rknn)) — intended for compose / k3s on Orange Pi.

## Quick start

```bash
cp .env.example .env
# KOKORO_MODELS_DIR=/home/orangepi/models/kokoro-rknn-ru

docker compose build          # first build: espeak + drogon (~10–15 min)
docker compose up -d --wait   # RKNN init ~2–3 с
curl -fsS http://127.0.0.1:8848/health
```

GHCR image:

```bash
docker compose -f docker-compose.yml -f docker-compose.prod.yml up -d
```

## Docs

| Doc | Content |
|-----|---------|
| [docs/models.md](docs/models.md) | Model volume, HF download, NPU devices |
| [docs/api.md](docs/api.md) | HTTP/WebSocket API |
| [docs/cicd.md](docs/cicd.md) | GitHub Actions, GHCR tags |
| [docs/rknn-hacking.md](docs/rknn-hacking.md) | RKNN graph optimizations |

## Models

Clone once on the host (separate from Piper voices in `/home/orangepi/models/`):

```bash
git clone https://huggingface.co/ShiWarai/kokoro-rknn-ru /home/orangepi/models/kokoro-rknn-ru
```

Or let the container download on start: `KOKORO_DOWNLOAD_MODELS=ru` in `.env`.

## Native build (maintainers)

```bash
git submodule update --init --recursive
mkdir build && cd build
cmake .. -DUSE_RKNN=ON -DBUILD_SERVER=ON -DORT_ROOT=/path/to/onnxruntime
make -j$(nproc) kokoro-server

export KOKORO_MODELS_DIR=/home/orangepi/models/kokoro-rknn-ru
./kokoro-server --ip 0.0.0.0 --port 8848
```

`build.py` — host-side ONNX/RKNN export only.

## License

MIT. Rockchip `librknnrt.so` in `third_party/` — vendor terms apply.
