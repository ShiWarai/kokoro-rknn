#include "mp3_encoder.hpp"

#include <lame/lame.h>

#include <stdexcept>
#include <vector>

namespace kokoro_server {

namespace {

struct LameHandle {
  lame_global_flags* gfp = nullptr;
  LameHandle() { gfp = lame_init(); }
  ~LameHandle() {
    if (gfp)
      lame_close(gfp);
  }
  LameHandle(const LameHandle&) = delete;
  LameHandle& operator=(const LameHandle&) = delete;
};

} // namespace

std::vector<uint8_t> encodeMp3(const int16_t* pcm, std::size_t n_samples,
                               int sample_rate) {
  if (!pcm || n_samples == 0)
    throw std::runtime_error("empty PCM for mp3 encode");
  if (sample_rate <= 0)
    throw std::runtime_error("invalid sample rate for mp3 encode");

  const auto pcm_count = static_cast<int>(n_samples);
  if (pcm_count < 0 || static_cast<std::size_t>(pcm_count) != n_samples)
    throw std::runtime_error("PCM too large for mp3 encode");

  LameHandle lame;
  if (!lame.gfp)
    throw std::runtime_error("lame_init failed");

  lame_set_in_samplerate(lame.gfp, sample_rate);
  lame_set_num_channels(lame.gfp, 1);
  lame_set_mode(lame.gfp, MONO);
  lame_set_VBR(lame.gfp, vbr_default);
  lame_set_VBR_quality(lame.gfp, 4);

  if (lame_init_params(lame.gfp) < 0)
    throw std::runtime_error("lame_init_params failed");

  std::vector<uint8_t> mp3(static_cast<std::size_t>(pcm_count) +
                               static_cast<std::size_t>(pcm_count) / 4 + 7200,
                           0);

  const int encoded = lame_encode_buffer(
      lame.gfp, pcm, nullptr, pcm_count, mp3.data(),
      static_cast<int>(mp3.size()));
  if (encoded < 0)
    throw std::runtime_error("lame_encode_buffer failed");

  const int flushed = lame_encode_flush(
      lame.gfp, mp3.data() + encoded,
      static_cast<int>(mp3.size() - static_cast<std::size_t>(encoded)));
  if (flushed < 0)
    throw std::runtime_error("lame_encode_flush failed");

  mp3.resize(static_cast<std::size_t>(encoded + flushed));
  if (mp3.empty())
    throw std::runtime_error("mp3 encode produced empty output");
  return mp3;
}

} // namespace kokoro_server
