#pragma once

#include <string>

namespace kokoro {

// Russian grapheme-to-phoneme for Kokoro-82M (zaakirio/kokoro-ru).
// Uses acute-aware espeak-ng data (data/espeak-data) plus orthoepic
// respelling, IPA normalization, and positional vowel reduction from ru_g2p.py.
// Stress marking relies on espeak's Russian dictionary when RUAccent is not
// available at runtime.
class RuG2P {
 public:
  static std::string foldBrackets(std::string text);
  static std::string toLowerRu(std::string text);
  static std::string respell(std::string text);
  static std::string normalizeIpa(std::string ipa);
  static std::string reduceVowels(const std::string& ipa);
  static std::string lengthenSch(std::string ipa);

  // Full pipeline: text -> Kokoro-vocab phoneme string.
  static std::string phonemize(const std::string& text);
};

bool isRussianVoice(const std::string& voiceName);

} // namespace kokoro
