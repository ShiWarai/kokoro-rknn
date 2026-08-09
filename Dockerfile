# syntax=docker/dockerfile:1
# Kokoro RKNN — linux/arm64 (RK3588). Espeak + server deps baked at build; slim runtime.

ARG ESPEAK_HF_REPO=https://huggingface.co/zaakirio/kokoro-ru
ARG DROGON_TAG=v1.9.10

# Russian espeak-ng data (only espeak-data/ lands in the final image).
FROM debian:bookworm-slim AS espeak-data
ARG ESPEAK_HF_REPO
ENV DEBIAN_FRONTEND=noninteractive
COPY docker/apt.conf /etc/apt/apt.conf.d/99-docker-build
RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates git git-lfs \
 && rm -rf /var/lib/apt/lists/* \
 && git lfs install \
 && git clone --depth 1 "${ESPEAK_HF_REPO}" /tmp/kokoro-ru \
 && test -f /tmp/kokoro-ru/espeak-data/ru_dict \
 && cp -a /tmp/kokoro-ru/espeak-data /espeak-ng-data \
 && rm -rf /tmp/kokoro-ru

# Drogon + libopusenc (not in git; built once per image, cached by Docker).
FROM gcc:12-bookworm AS server-deps
ARG DROGON_TAG
ENV DEBIAN_FRONTEND=noninteractive
COPY docker/apt.conf /etc/apt/apt.conf.d/99-docker-build
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      autoconf automake ca-certificates cmake git libtool \
      libjsoncpp-dev libogg-dev libopus-dev libssl-dev \
      libyaml-cpp-dev pkg-config uuid-dev zlib1g-dev \
 && rm -rf /var/lib/apt/lists/* \
 && git clone --depth 1 --branch "${DROGON_TAG}" \
      https://github.com/drogonframework/drogon.git /tmp/drogon \
 && cd /tmp/drogon && git submodule update --init \
 && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/deps/drogon-install \
      -DBUILD_EXAMPLES=OFF -DBUILD_CTL=OFF \
 && cmake --build build -j"$(nproc)" \
 && cmake --install build \
 && git clone --depth 1 https://github.com/xiph/libopusenc.git /tmp/libopusenc \
 && cd /tmp/libopusenc \
 && ./autogen.sh \
 && ./configure --prefix=/deps/opusenc-install --disable-shared \
 && make -j"$(nproc)" \
 && make install \
 && test -f /deps/drogon-install/lib/cmake/Drogon/DrogonConfig.cmake \
 && test -f /deps/opusenc-install/lib/libopusenc.a

FROM gcc:12-bookworm AS builder

ENV DEBIAN_FRONTEND=noninteractive

COPY docker/apt.conf /etc/apt/apt.conf.d/99-docker-build

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates cmake curl \
      libblas-dev libespeak-ng-dev libfmt-dev libjsoncpp-dev libogg-dev \
      libopenblas-dev libopus-dev libspdlog-dev libsoxr-dev libssl-dev \
      libyaml-cpp-dev nlohmann-json3-dev pkg-config uuid-dev zlib1g-dev \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /src

COPY third_party/onnxruntime/ third_party/onnxruntime/
COPY third_party/librknnrt.so third_party/librknnrt.so
COPY third_party/rknn/include/ third_party/rknn/include/
COPY --from=server-deps /deps/drogon-install/ deps/drogon-install/
COPY --from=server-deps /deps/opusenc-install/ deps/opusenc-install/

RUN test -f deps/drogon-install/lib/cmake/Drogon/DrogonConfig.cmake \
 && test -f deps/opusenc-install/lib/libopusenc.a \
 && test -f third_party/onnxruntime/include/onnxruntime_cxx_api.h \
 && test -f third_party/rknn/include/rknn_api.h

COPY CMakeLists.txt .gitmodules ./
COPY cmake/ cmake/
COPY include/ include/
COPY src/ src/
COPY server/ server/
COPY misaki-cpp/ misaki-cpp/

RUN cp third_party/librknnrt.so /usr/lib/librknnrt.so \
 && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_RKNN=ON \
      -DBUILD_SERVER=ON -DBUILD_CLI=OFF -DBUILD_TESTS=OFF \
      -DORT_ROOT=/src/third_party/onnxruntime \
 && cmake --build build -j"$(nproc)" --target kokoro-server \
 && strip --strip-unneeded build/kokoro-server \
 && mkdir -p /out/lib /out/bin \
 && cp build/kokoro-server /out/bin/ \
 && cp -a build/misaki-data /out/bin/misaki-data \
 && cp -L third_party/librknnrt.so /out/lib/ \
 && cp -L third_party/onnxruntime/lib/libonnxruntime.so.1.14.1 /out/lib/ \
 && ldd build/kokoro-server \
      | awk '/=> \// {print $3}' | sort -u \
      | while read -r lib; do \
           [ -f "$lib" ] || continue; \
           base="$(basename "$lib")"; \
           case "$base" in \
             ld-linux*|libc.so*|libm.so*|libpthread.so*|libdl.so*|librt.so*|libresolv.so*) continue ;; \
           esac; \
           cp -L "$lib" /out/lib/; \
         done \
 && ln -sf libonnxruntime.so.1.14.1 /out/lib/libonnxruntime.so

FROM debian:bookworm-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive \
    APP_DIR=/opt/kokoro-rknn \
    MODELS_DIR=/models \
    PORT=8848 \
    HOST=0.0.0.0 \
    LD_LIBRARY_PATH=/opt/kokoro-rknn/lib

COPY docker/apt.conf /etc/apt/apt.conf.d/99-docker-build

RUN apt-get update \
 && apt-get install -y --no-install-recommends ca-certificates curl ffmpeg \
 && rm -rf /var/lib/apt/lists/*

COPY --from=builder /out/bin/kokoro-server /opt/kokoro-rknn/bin/
COPY --from=builder /out/bin/misaki-data /opt/kokoro-rknn/bin/misaki-data
COPY --from=espeak-data /espeak-ng-data /opt/kokoro-rknn/bin/espeak-ng-data
COPY --from=builder /out/lib/ /opt/kokoro-rknn/lib/
COPY server/web-content/ /opt/kokoro-rknn/server/web-content/
COPY scripts/docker-entrypoint.sh scripts/download_models.sh /opt/kokoro-rknn/scripts/

RUN chmod +x /opt/kokoro-rknn/scripts/docker-entrypoint.sh \
             /opt/kokoro-rknn/scripts/download_models.sh

WORKDIR /opt/kokoro-rknn

EXPOSE 8848

HEALTHCHECK --interval=30s --timeout=10s --start-period=30s --retries=3 \
  CMD curl -fsS "http://127.0.0.1:${PORT}/health" || exit 1

CMD ["./scripts/docker-entrypoint.sh"]

# CI / local unit tests (no NPU, no server). Repo mounted at /app.
FROM gcc:12-bookworm AS dev

ENV DEBIAN_FRONTEND=noninteractive

COPY docker/apt.conf /etc/apt/apt.conf.d/99-docker-build

RUN apt-get update \
 && apt-get install -y --no-install-recommends \
      ca-certificates cmake curl git \
      libblas-dev libespeak-ng-dev libfmt-dev libopenblas-dev \
      libspdlog-dev nlohmann-json3-dev pkg-config \
 && rm -rf /var/lib/apt/lists/*

COPY --from=espeak-data /espeak-ng-data /opt/espeak-ng-data

WORKDIR /app

CMD ["bash"]
