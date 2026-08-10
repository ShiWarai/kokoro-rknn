#include <cassert>
#include <iostream>
#include <string>

#include "phonemizer.hpp"
#include "ru_g2p.hpp"

int main() {
  using kokoro::RuG2P;

  const std::string folded = RuG2P::foldBrackets("текст [в скобках]");
  assert(folded.find('[') == std::string::npos);
  assert(folded.find('(') != std::string::npos);

  const std::string lower = RuG2P::toLowerRu("Москва");
  assert(lower == "москва");

  const std::string respelled = RuG2P::respell("здравствуйте");
  assert(respelled.find("здраств") != std::string::npos);

  const std::string norm = RuG2P::normalizeIpa("ɭɵʧʲ");
  assert(norm.find('l') != std::string::npos);
  assert(norm.find('o') != std::string::npos);

  const std::string reduced = RuG2P::reduceVowels("ɡəlɐvˈa");
  assert(reduced.find("\xC9\x99") != std::string::npos);

  const std::string longSch = RuG2P::lengthenSch("ɕa");
  assert(longSch.find("\xCA\x95\xCB\x90") != std::string::npos);

  assert(kokoro::isRussianVoice("sveta"));
  assert(kokoro::isRussianVoice("dima"));
  assert(!kokoro::isRussianVoice("af_heart"));

  const std::string espeakData = "espeak-ng-data";
  kokoro::Phonemizer::init(espeakData);
  const std::string ph = RuG2P::phonemize("Москва");
  kokoro::Phonemizer::terminate();

  if (ph.empty()) {
    std::cerr << "RuG2P::phonemize returned empty string\n";
    return 1;
  }
  std::cout << "moscow phonemes: " << ph << "\n";
  return 0;
}
