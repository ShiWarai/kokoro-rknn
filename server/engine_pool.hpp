#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "kokoro.hpp"

namespace kokoro_server {

// Loads every available model pack (base, dima) on NPU and routes voice/model
// requests to the correct Engine instance.
class EnginePool {
 public:
  void load(const std::string& accelerator = "");

  bool hasPack(const std::string& pack) const;
  std::vector<std::string> loadedPacks() const;
  std::vector<std::string> listVoices() const;

  // Resolve OpenAI model id or pack name to internal pack id ("base"|"dima").
  std::string resolveModelId(const std::string& model) const;

  // Pick pack from optional model + voice; throws on unknown voice / mismatch.
  kokoro::Engine& engineFor(const std::string& voice,
                            const std::optional<std::string>& model = std::nullopt);

  std::string packForVoice(const std::string& voice) const;

  int sampleRate() const;

 private:
  static bool packAvailable(const char* pack);
  static kokoro::EngineConfig makeConfig(const std::string& decoderPath);

  std::map<std::string, std::unique_ptr<kokoro::Engine>> packs_;
  std::map<std::string, std::string> voice_to_pack_;
};

} // namespace kokoro_server
