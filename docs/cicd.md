# CI/CD

## Workflows

| Workflow | Trigger | Action |
|----------|---------|--------|
| `Deploy` | push `main`/`dev`, manual | Unit tests in `Dockerfile` target `dev`; optional `:prerelease` on `dev` with `[prerelease]` in commit message |
| `Publish Docker image` | successful Deploy on `main` | Push `ghcr.io/shiwarai/kokoro-rknn:main` + SHA |

## GHCR tags

| Tag | When |
|-----|------|
| `:main` | After successful tests on `main` |
| `:prerelease` | `dev` push with `[prerelease]` or manual dispatch |
| `:<sha>` | Same build as prerelease/main |

## Local test (as CI)

```bash
docker compose -f docker-compose.dev.yml build
docker compose -f docker-compose.dev.yml run --rm -T dev bash -lc '
  git submodule update --init --recursive
  rm -rf build
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SERVER=OFF -DBUILD_CLI=OFF -DBUILD_TESTS=ON -DUSE_RKNN=OFF \
    -DORT_ROOT=/app/third_party/onnxruntime \
    -DRU_ESPEAK_DATA_DIR=/opt/espeak-ng-data
  cmake --build build -j$(nproc) --target test-ru-g2p
  cd build && ctest -R ru_g2p_unit --output-on-failure
'
```

## Local prod compose

```bash
cp .env.example .env
# set KOKORO_MODELS_DIR

docker compose build
docker compose up -d --wait

curl -fsS http://127.0.0.1:${PORT:-8848}/health

# or GHCR image:
docker compose -f docker-compose.yml -f docker-compose.prod.yml up -d
```

## Telegram notifications

Optional secrets: `TELEGRAM_TOKEN`, `TELEGRAM_TO` (same as whisper-rknn).

## Platform

Images are **linux/arm64** (RK3588). CI uses GitHub-hosted **`ubuntu-24.04-arm`** runners (native arm64, no QEMU).
