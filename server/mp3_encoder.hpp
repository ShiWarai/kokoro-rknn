#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace kokoro_server {

bool ffmpegAvailable();

// Encode mono int16 PCM to MP3 via ffmpeg pipe (CPU post-process after NPU synth).
std::vector<uint8_t> encodeMp3(const int16_t* pcm, std::size_t n_samples,
                               int sample_rate);

} // namespace kokoro_server
