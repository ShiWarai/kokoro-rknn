# kokoro-infer

Fast, lightweight end-to-end inference engine and server for Kokoro-82M TTS, optimized for CPU, CUDA, and Rockchip RK3588 NPU.

Russian models: **[ShiWarai/kokoro-rknn-ru](https://huggingface.co/ShiWarai/kokoro-rknn-ru)**.

Runtime is **C++ only** (`libkokoro`, `kokoro-cli`, `kokoro-server`). `build.py` is only for maintainers re-exporting ONNX/RKNN.

## Quick start

```bash
git submodule update --init --recursive

# Kokoro weights — subdirectory alongside other models on the board, e.g.:
git clone https://huggingface.co/ShiWarai/kokoro-rknn-ru /home/orangepi/models/kokoro-rknn-ru

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DUSE_RKNN=ON \
         -DBUILD_SERVER=ON -DBUILD_CLI=ON \
         -DORT_ROOT=/path/to/onnxruntime
make -j$(nproc)

export KOKORO_MODELS_DIR=/home/orangepi/models/kokoro-rknn-ru

./kokoro-cli --text "Привет!" --voice sveta --out hello.wav
./kokoro-server --ip 0.0.0.0 --port 8848
```

`KOKORO_MODELS_DIR` (or `--models-dir`) must point at the **kokoro HF clone root** — not the shared `/home/orangepi/models/` tree that also holds piper voices (`denis`, `irina`, …) and `hf/`.

Inside that root the code looks for packs as `packs/base/` (HF layout) or flat `base/`:

| Pack | Voices |
|------|--------|
| `base` | sveta, masha |
| `dima` | dima |

Russian espeak data (for G2P):

```bash
cp -aL ~/.cache/huggingface/hub/models--zaakirio--kokoro-ru/snapshots/*/espeak-data data/espeak-data
```

## Server deps

Drogon + libopusenc are vendored under `deps/` — build once (see prior commits / board notes).

## Maintainer

`python3 build.py` → push `packs/*` to [ShiWarai/kokoro-rknn-ru](https://huggingface.co/ShiWarai/kokoro-rknn-ru).

## Optimizations

See `ON_RKNN_HACKING.md`.
