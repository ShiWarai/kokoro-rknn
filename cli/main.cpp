// kokoro-cli — minimal CLI for the C++ pipeline. Reads --text/--phonemes,
// writes a WAV file. Matches infer.py's UX so the two can be sanity-checked
// against each other.

#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "g2p.hpp"
#include "kokoro.hpp"
#include "paths.hpp"
#include "phonemizer.hpp"

namespace {

void writeWav(const std::string& path, const std::vector<int16_t>& pcm, int sr) {
  std::ofstream f(path, std::ios::binary);
  if (!f) throw std::runtime_error("cannot open " + path);
  auto put32 = [&](uint32_t v){ f.write(reinterpret_cast<const char*>(&v), 4); };
  auto put16 = [&](uint16_t v){ f.write(reinterpret_cast<const char*>(&v), 2); };
  uint32_t pcm_bytes = static_cast<uint32_t>(pcm.size() * 2);
  f.write("RIFF", 4); put32(36 + pcm_bytes);
  f.write("WAVE", 4); f.write("fmt ", 4); put32(16); put16(1); put16(1);
  put32(sr); put32(sr * 2); put16(2); put16(16);
  f.write("data", 4); put32(pcm_bytes);
  f.write(reinterpret_cast<const char*>(pcm.data()), pcm_bytes);
}

void usage() {
  std::cerr <<
    "usage: kokoro-cli [--text STR | --phonemes STR] [opts]\n"
    "  --voice NAME         (default sveta)\n"
    "  --speed FLOAT        (default 1.0)\n"
    "  --british            use en-gb voice and remap\n"
    "  --out FILE           output wav (default out.wav)\n"
    "  --models-dir DIR     model repo root (or set KOKORO_MODELS_DIR)\n"
    "  --encoder FILE       pack file (default: <models-dir>/{pack}/ or packs/{pack}/)\n"
    "  --har-gen FILE       har generator ONNX\n"
    "  --decoder FILE       decoder .rknn\n"
    "  --vocab FILE         config.json\n"
    "  --voices-dir DIR     voices_npy/\n"
    "  --espeak-data DIR    espeak-ng-data (else next to executable)\n"
    "  --lexicon-dir DIR    misaki us/gb JSONs (else next to executable)\n"
    "  --accelerator STR    cuda | tensorrt | (empty)\n"
    "  --debug\n";
}

const char* packForVoice(const std::string& voice) {
  return voice == "dima" ? "dima" : "base";
}

} // namespace

int main(int argc, char** argv) {
  spdlog::set_default_logger(spdlog::stderr_color_st("kokoro"));

  std::string text, phonemes;
  std::string voice = "sveta";
  std::string out   = "out.wav";
  std::string encoderPath;
  std::string harGenPath;
  std::string decoderPath;
  std::string vocabPath;
  std::string voicesDir;
  std::string modelsDir;
  std::string accelerator = "";
  std::string espeakData  = "";
  std::string lexiconDir  = "";
  float speed = 1.0f;
  bool british = false;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    auto need = [&](){
      if (i + 1 >= argc) { usage(); std::exit(1); }
      return std::string(argv[++i]);
    };
    if      (a == "--text")        text = need();
    else if (a == "--phonemes")    phonemes = need();
    else if (a == "--voice")       voice = need();
    else if (a == "--speed")       speed = std::stof(need());
    else if (a == "--british")     british = true;
    else if (a == "--out")         out = need();
    else if (a == "--encoder")     encoderPath = need();
    else if (a == "--har-gen")     harGenPath  = need();
    else if (a == "--decoder")     decoderPath = need();
    else if (a == "--vocab")       vocabPath = need();
    else if (a == "--voices-dir")  voicesDir = need();
    else if (a == "--models-dir")  modelsDir = need();
    else if (a == "--accelerator") accelerator = need();
    else if (a == "--espeak-data") espeakData = need();
    else if (a == "--lexicon-dir") lexiconDir = need();
    else if (a == "--debug")       spdlog::set_level(spdlog::level::debug);
    else if (a == "-h" || a == "--help") { usage(); return 0; }
    else { std::cerr << "unknown arg: " << a << "\n"; usage(); return 1; }
  }

  if (text.empty() && phonemes.empty()) {
    text = "Привет, как дела?";
    spdlog::info("(no --text/--phonemes; using Russian demo string)");
  }

  if (!modelsDir.empty())
    kokoro::paths::setModelsDir(modelsDir);

  const char* pack = packForVoice(voice);
  auto pick = [&](const std::string& p, const char* file) {
    return p.empty() ? kokoro::paths::packFile(pack, file)
                     : kokoro::paths::resolveUserPath(p);
  };
  encoderPath = pick(encoderPath, "kokoro_encoder.onnx");
  harGenPath  = pick(harGenPath,  "har_generator.onnx");
  decoderPath = pick(decoderPath, "kokoro_decoder.rknn");
  vocabPath   = pick(vocabPath,   "config.json");
  voicesDir   = pick(voicesDir,   "voices_npy");

  if (espeakData.empty())
    espeakData = (kokoro::paths::exeDir() / "espeak-ng-data").string();
  kokoro::Phonemizer::init(espeakData);

  if (lexiconDir.empty())
    lexiconDir = (kokoro::paths::exeDir() / "misaki-data").string();
  kokoro::G2P::init(lexiconDir, espeakData);

  kokoro::EngineConfig cfg;
  cfg.decoderWorkers = (std::filesystem::path(decoderPath).extension() == ".rknn") ? 3 : 1;

  kokoro::Engine engine;
  engine.load(vocabPath, encoderPath, harGenPath, decoderPath, voicesDir,
              accelerator, cfg);

  std::vector<int16_t> pcm;
  auto emit = [&](const int16_t* d, std::size_t n) {
    pcm.insert(pcm.end(), d, d + n);
  };

  auto t0 = std::chrono::steady_clock::now();
  kokoro::SynthesisResult r;
  if (!text.empty()) r = engine.synthesizeText(text, voice, speed, british, emit);
  else               r = engine.synthesizePhonemes(phonemes, voice, speed, emit);
  auto t1 = std::chrono::steady_clock::now();
  double wall = std::chrono::duration<double>(t1 - t0).count();

  writeWav(out, pcm, engine.config().sampleRate);
  spdlog::info("wrote {} ({} samples, {:.2f}s audio)", out, pcm.size(),
               r.audioSeconds);
  spdlog::info("encoder={:.1f}ms har={:.1f}ms dec={:.1f}ms istft={:.1f}ms  wall={:.2f}s  RTF={:.2f}x",
               r.encoderSeconds * 1000, r.harSeconds * 1000,
               r.decoderSeconds * 1000, r.istftSeconds * 1000,
               wall, r.realTimeFactor);

  kokoro::G2P::terminate();
  kokoro::Phonemizer::terminate();
  return 0;
}
