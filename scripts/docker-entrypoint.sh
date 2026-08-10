#!/usr/bin/env bash
set -euo pipefail

APP_DIR="${APP_DIR:-/opt/kokoro-rknn}"
MODELS_DIR="${MODELS_DIR:-/models}"
PORT="${PORT:-8848}"

# Model weights only — espeak-ng-data is baked into the image next to kokoro-server.
if [[ -n "${KOKORO_DOWNLOAD_MODELS:-}" && "${KOKORO_DOWNLOAD_MODELS}" != "0" ]]; then
  MODELS_DIR="${MODELS_DIR}" \
    KOKORO_DOWNLOAD_MODELS="${KOKORO_DOWNLOAD_MODELS}" \
    KOKORO_MODEL_URLS="${KOKORO_MODEL_URLS:-}" \
    "${APP_DIR}/scripts/download_models.sh"
fi

export KOKORO_MODELS_DIR="${MODELS_DIR}"

web_root="${KOKORO_WEB_ROOT:-${APP_DIR}/server/web-content}"
args=(--models-dir "${MODELS_DIR}" --ip "${HOST:-0.0.0.0}" --port "${PORT}"
      --web-root "${web_root}")

if [[ -n "${KOKORO_DEFAULT_VOICE:-}" ]]; then
  args+=(--default-voice "${KOKORO_DEFAULT_VOICE}")
fi
if [[ -n "${KOKORO_TOKEN:-}" ]]; then
  args+=(--auth "${KOKORO_TOKEN}")
elif [[ -n "${OPENAI_API_KEY:-}" ]]; then
  args+=(--auth "${OPENAI_API_KEY}")
fi
if [[ "${KOKORO_DISABLE_WEB_UI:-0}" == "1" ]]; then
  args+=(--disable-web-ui)
fi

echo "starting kokoro-server; web UI http://${HOST:-0.0.0.0}:${PORT}/"
exec "${APP_DIR}/bin/kokoro-server" "${args[@]}" "$@"
