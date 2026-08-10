#!/usr/bin/env bash
# Download Kokoro RKNN Russian model packs into MODELS_DIR (mounted volume).
#
# Presets (KOKORO_DOWNLOAD_MODELS or first argument):
#   ru | 1 — ShiWarai/kokoro-rknn-ru on Hugging Face (packs/base + packs/dima)
#
# Custom URLs (KOKORO_MODEL_URLS):
#   relpath=URL,...
#
# Russian espeak-ng-data is baked into the Docker image (not downloaded here).
set -euo pipefail

MODELS_DIR="${MODELS_DIR:-/models}"
HF_BASE="${KOKORO_HF_BASE:-https://huggingface.co/ShiWarai}"
HF_REPO="${KOKORO_HF_REPO:-kokoro-rknn-ru}"

mkdir -p "${MODELS_DIR}"

declare -a WANT_LOCAL=()
declare -a WANT_URL=()

add_ru_pack() {
  local pack="$1"
  local -a files=(
    "packs/${pack}/config.json"
    "packs/${pack}/kokoro_encoder.onnx"
    "packs/${pack}/har_generator.onnx"
    "packs/${pack}/kokoro_decoder.rknn"
  )
  local f
  for f in "${files[@]}"; do
    WANT_LOCAL+=("${f}")
    WANT_URL+=("${HF_BASE}/${HF_REPO}/resolve/main/${f}")
  done
}

add_ru_voices() {
  local pack="$1"
  shift
  local voice
  for voice in "$@"; do
    local rel="packs/${pack}/voices_npy/${voice}.npy"
    WANT_LOCAL+=("${rel}")
    WANT_URL+=("${HF_BASE}/${HF_REPO}/resolve/main/${rel}")
  done
}

add_ru() {
  WANT_LOCAL+=("manifest.json")
  WANT_URL+=("${HF_BASE}/${HF_REPO}/resolve/main/manifest.json")
  add_ru_pack base
  add_ru_voices base sveta masha
  add_ru_pack dima
  add_ru_voices dima dima
}

parse_selection() {
  local raw="${1:-}"
  raw="${raw// /}"
  if [[ -z "${raw}" || "${raw}" == "0" ]]; then
    return 0
  fi
  if [[ "${raw}" == "urls" || "${raw}" == "custom" ]]; then
    return 0
  fi
  case "${raw}" in
    1 | ru) add_ru ;;
    *)
      echo "error: unknown model preset '${raw}' (use ru or 1)" >&2
      exit 1
      ;;
  esac
}

parse_custom_urls() {
  local raw="${KOKORO_MODEL_URLS:-}"
  [[ -z "${raw}" ]] && return 0
  raw="${raw//$'\n'/,}"
  local IFS=',' entry local_name url
  for entry in ${raw}; do
    entry="${entry#"${entry%%[![:space:]]*}"}"
    entry="${entry%"${entry##*[![:space:]]}"}"
    [[ -z "${entry}" ]] && continue
    if [[ "${entry}" != *"="* ]]; then
      echo "error: KOKORO_MODEL_URLS entry must be relpath=URL, got: ${entry}" >&2
      exit 1
    fi
    local_name="${entry%%=*}"
    url="${entry#*=}"
    local_name="${local_name#"${local_name%%[![:space:]]*}"}"
    url="${url#"${url%%[![:space:]]*}"}"
    if [[ -z "${local_name}" || -z "${url}" ]]; then
      echo "error: invalid KOKORO_MODEL_URLS entry: ${entry}" >&2
      exit 1
    fi
    WANT_LOCAL+=("${local_name}")
    WANT_URL+=("${url}")
  done
}

download_if_missing() {
  local rel="$1"
  local url="$2"
  local dest="${MODELS_DIR}/${rel}"

  if [[ -f "${dest}" ]]; then
    echo "skip (exists): ${rel}"
    return 0
  fi

  mkdir -p "$(dirname "${dest}")"
  local tmp="${dest}.part"
  echo "download: ${rel} <- ${url}"
  curl -fL --retry 3 --retry-delay 5 -o "${tmp}" "${url}"
  mv "${tmp}" "${dest}"
}

selection="${KOKORO_DOWNLOAD_MODELS:-}"
if [[ $# -gt 0 ]]; then
  selection="$1"
fi

parse_selection "${selection}"
parse_custom_urls

if [[ ${#WANT_LOCAL[@]} -eq 0 ]]; then
  echo "nothing to download (set KOKORO_DOWNLOAD_MODELS=ru and/or KOKORO_MODEL_URLS)" >&2
  exit 1
fi

for i in "${!WANT_LOCAL[@]}"; do
  download_if_missing "${WANT_LOCAL[$i]}" "${WANT_URL[$i]}"
done

for f in "${WANT_LOCAL[@]}"; do
  if [[ ! -f "${MODELS_DIR}/${f}" ]]; then
    echo "error: missing ${MODELS_DIR}/${f}" >&2
    exit 1
  fi
done

echo "models ready in ${MODELS_DIR}"
