# Vendored binaries for RK3588 (linux/arm64)

| Path | Purpose |
|------|---------|
| `librknnrt.so` | Rockchip RKNN runtime |
| `rknn/include/rknn_api.h` | RKNN SDK header (build) |
| `onnxruntime/` | ONNX Runtime 1.14.1 (headers + libs for Docker build) |

Runtime `.so` for the image are collected in the Dockerfile builder via `ldd` (not stored here).

Russian **espeak-ng-data** is fetched at **Docker build** from [zaakirio/kokoro-ru](https://huggingface.co/zaakirio/kokoro-ru) and baked into the image at `bin/espeak-ng-data/`.

Drogon + libopusenc are built inside the Dockerfile (`server-deps` stage), not vendored in git.
