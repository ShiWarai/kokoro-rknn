# kokoro-infer

Fast, lightweight end-to-end inference engine and server for Kokoro-82M TTS, optimized for CPU, CUDA, and Rockchip RK3588 NPU.

Primary target: **Russian** (zaakirio/kokoro-ru weights). English and other espeak/misaki languages remain supported.

Runtime is **C++ only** (`libkokoro`, `kokoro-cli`, `kokoro-server`). `build.py` is the sole Python tool — host-side export of ONNX/RKNN artefacts.

## Features

- RK3588 NPU acceleration (~2.5× RTF) via decoder graph rewrites (see `ON_RKNN_HACKING.md`)
- Native C++ library: ONNX Runtime + optional RKNN, no Python at runtime
- Russian G2P (`src/ru_g2p.cpp`) with acute-aware espeak-ng data in `data/espeak-data/`
- English G2P via misaki-cpp; Spanish/French/Hindi/Italian/Portuguese via espeak-ng
- OpenAI-compatible HTTP/WebSocket server (Drogon):
  - `POST /v1/audio/speech`
  - `POST /api/v1/synthesise`
  - `WS /api/v1/stream`
  - `GET /api/v1/voices`
- Built-in Web UI at `http://<host>:8848/`

## Project layout

| Path | Role |
|------|------|
| `build.py` | Host: PyTorch → ONNX → RKNN (run once per checkpoint) |
| `src/` | `libkokoro` — inference + G2P |
| `cli/` | `kokoro-cli` |
| `server/` | `kokoro-server` + web UI |
| `data/espeak-data/` | Russian espeak-ng data (from zaakirio/kokoro-ru; gitignored — copy once, see below) |
| `misaki-cpp/` | English G2P submodule |
| `models/` | Shipped model packs (gitignored): `base/` (sveta, masha), `dima/` (dima) |

## Build

```bash
git submodule update --init --recursive

mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DUSE_RKNN=ON \
         -DBUILD_SERVER=ON \
         -DBUILD_CLI=ON \
         -DBUILD_TESTS=ON
make -j$(nproc)
ctest --output-on-failure
```

Dependencies: ONNX Runtime, espeak-ng, OpenBLAS, fmt, spdlog, nlohmann_json; server adds Drogon, Opus, soxr.

Russian espeak data (required for G2P/tests):

```bash
cp -aL ~/.cache/huggingface/hub/models--zaakirio--kokoro-ru/snapshots/*/espeak-data data/espeak-data
```

## Model export (host, once)

Place Russian checkpoints under `Kokoro-82M/`, then:

```bash
python3 build.py
```

Copy outputs into `models/base/` or `models/dima/` (encoder, har, decoder `.rknn`, `config.json`, `voices_npy/`).

## CLI (Russian, NPU)

Defaults point at `models/base/` and voice `sveta`:

```bash
cd build
./kokoro-cli --text "Привет, как дела?" --voice sveta --out hello.wav
./kokoro-cli --text "Привет" --voice dima \
  --encoder ../models/dima/kokoro_encoder.onnx \
  --har-gen ../models/dima/har_generator.onnx \
  --decoder ../models/dima/kokoro_decoder.rknn \
  --vocab ../models/dima/config.json \
  --voices-dir ../models/dima/voices_npy
```

English (needs misaki-data staged next to the binary):

```bash
./kokoro-cli --text "Hello world." --voice af_heart \
  --encoder onnx/kokoro_encoder.onnx \
  --har-gen onnx/har_generator.onnx \
  --decoder onnx/kokoro_decoder.rknn \
  --vocab Kokoro-82M/config.json \
  --voices-dir voices_npy
```

## Server + Web UI

**One-time server deps** (Drogon + libopusenc are vendored under `deps/`, not in apt):

```bash
sudo apt install -y libjsoncpp-dev libyaml-cpp-dev uuid-dev libssl-dev zlib1g-dev \
  libopus-dev libopusfile-dev libogg-dev libsoxr-dev libtool automake autoconf pkg-config

# libopusenc
git clone --depth 1 https://github.com/xiph/opusenc.git deps/opusenc-src
cd deps/opusenc-src && ./autogen.sh && ./configure --prefix=$PWD/../opusenc-install --disable-shared
make -j$(nproc) && make install && cd ../..

# Drogon
git clone --depth 1 --recursive https://github.com/drogonframework/drogon.git deps/drogon-src
cmake -S deps/drogon-src -B deps/drogon-src/build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=$PWD/deps/drogon-install -DBUILD_CTL=OFF -DBUILD_EXAMPLES=OFF -DBUILD_ORM=OFF
cmake --build deps/drogon-src/build -j$(nproc) && cmake --install deps/drogon-src/build
```

CMake picks up `deps/drogon-install` and `deps/opusenc-install` automatically.

```bash
cd build
cmake .. -DUSE_RKNN=ON -DBUILD_SERVER=ON \
  -DORT_ROOT=/root/paroli-tts/deps/onnxruntime-linux-aarch64-1.14.1
make -j$(nproc) kokoro-server

./kokoro-server --ip 0.0.0.0 --port 8848
```

Open `http://<board-ip>:8848/` — model paths default to `models/base/`.

## Voices

| Voice | Pack | Gender |
|-------|------|--------|
| `sveta`, `masha` | `models/base/` | female |
| `dima` | `models/dima/` | male |

English voices use the `a`/`b` prefix convention (`af_heart`, `bf_emma`, …).

## Optimizations

See `ON_RKNN_HACKING.md` for NPU graph surgery details.
