#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace kokoro_server {

// Encode mono int16 PCM to MP3 via libmp3lame (CPU post-process after NPU synth).
std::vector<uint8_t> encodeMp3(const int16_t* pcm, std::size_t n_samples,
                               int sample_rate);

} // namespace kokoro_server
