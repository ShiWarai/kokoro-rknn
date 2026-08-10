#include "ru_g2p.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "phonemizer.hpp"
#include "utf8.hpp"

namespace kokoro {
namespace {

constexpr char32_t kAcute = 0x0301;

bool isRuVowel(char32_t c) {
  return c == 0x0430 || c == 0x0435 || c == 0x0451 || c == 0x0438 ||
         c == 0x043E || c == 0x0443 || c == 0x044B || c == 0x044D ||
         c == 0x044E || c == 0x044F;
}

bool isRuLetter(char32_t c) {
  return (c >= 0x0430 && c <= 0x044F) || c == 0x0451 || c == kAcute;
}

std::string replaceAll(std::string s, const std::string& from, const std::string& to) {
  if (from.empty()) return s;
  std::size_t p = 0;
  while ((p = s.find(from, p)) != std::string::npos) {
    s.replace(p, from.size(), to);
    p += to.size();
  }
  return s;
}

const std::unordered_set<std::string>& ogoBlacklist() {
  static const std::unordered_set<std::string> k = {
      "много",     "немного",   "намного",   "ненамного", "строго",
      "нестрого",  "настрого",  "дорого",    "недорого",  "полого",
      "убого",     "лего",      "диего",     "ого",       "огого",
  };
  return k;
}

const std::unordered_map<std::string, std::string>& respellWords() {
  static const std::unordered_map<std::string, std::string> k = {
      {"дорого", "дорога"},       {"недорого", "недорога"},
      {"настрого", "настрога"},   {"конечно", "конешно"},
      {"скучно", "скушно"},       {"скучный", "скушный"},
      {"нарочно", "нарошно"},     {"яичница", "яишница"},
      {"скворечник", "скворешник"},{"девичник", "девишник"},
      {"двоечник", "двоешник"},   {"троечник", "троешник"},
      {"прачечная", "прашечная"},{"горчичник", "горчишник"},
      {"пустячный", "пустяшный"},
  };
  return k;
}

const std::vector<std::pair<std::string, std::string>>& respellSubstr() {
  static const std::vector<std::pair<std::string, std::string>> k = {
      {"солнц", "сонц"},     {"чувств", "чуств"},     {"здравств", "здраств"},
      {"счастлив", "счаслив"},{"завистлив", "завислив"},{"участлив", "учаслив"},
      {"совестлив", "совеслив"},
  };
  return k;
}

std::string stripAcute(const std::string& word) {
  std::vector<char32_t> cps = utf8::decode(word);
  std::vector<char32_t> out;
  out.reserve(cps.size());
  for (char32_t c : cps)
    if (c != kAcute) out.push_back(c);
  return utf8::encode(out);
}

int acuteVowelIndex(const std::vector<char32_t>& cps) {
  for (std::size_t i = 0; i < cps.size(); ++i) {
    if (cps[i] == kAcute && i > 0) {
      int n = 0;
      for (std::size_t j = 0; j < i; ++j)
        if (isRuVowel(cps[j])) ++n;
      return n;
    }
  }
  return 0;
}

std::string reseatAcute(const std::string& bare, int vowelN) {
  if (vowelN <= 0) return bare;
  std::vector<char32_t> cps = utf8::decode(bare);
  std::vector<char32_t> out;
  out.reserve(cps.size() + 1);
  int n = 0;
  for (char32_t c : cps) {
    out.push_back(c);
    if (isRuVowel(c)) {
      ++n;
      if (n == vowelN) out.push_back(kAcute);
    }
  }
  return utf8::encode(out);
}

bool endsWithOgo(const std::string& bare) {
  std::vector<char32_t> cps = utf8::decode(bare);
  if (cps.size() < 3) return false;
  auto isOe = [](char32_t c) {
    return c == 0x043E || c == 0x0435 || c == 0x0451;
  };
  return isOe(cps[cps.size() - 3]) && cps[cps.size() - 2] == 0x0433 &&
         cps[cps.size() - 1] == 0x043E;
}

std::string applyOgoRespell(std::string word) {
  std::string bare = stripAcute(word);
  if (!endsWithOgo(bare)) return word;
  std::vector<char32_t> cps = utf8::decode(word);
  std::vector<char32_t> out;
  out.reserve(cps.size());
  for (std::size_t i = 0; i + 2 < cps.size(); ++i) {
    if (cps[i + 1] == 0x0433 && cps[i + 2] == 0x043E &&
        (cps[i] == 0x043E || cps[i] == 0x0435 || cps[i] == 0x0451)) {
      out.push_back(cps[i]);
      out.push_back(0x0432);
      out.push_back(0x043E);
      if (i + 3 < cps.size() && cps[i + 3] == kAcute) out.push_back(kAcute);
      for (std::size_t j = i + 3; j < cps.size(); ++j) out.push_back(cps[j]);
      return utf8::encode(out);
    }
    out.push_back(cps[i]);
  }
  return word;
}

std::string respellWord(std::string word) {
  std::string bare = stripAcute(word);
  auto it = respellWords().find(bare);
  if (it != respellWords().end()) {
    int vn = acuteVowelIndex(utf8::decode(word));
    return reseatAcute(it->second, vn);
  }
  if (!ogoBlacklist().count(bare)) word = applyOgoRespell(word);
  bare = stripAcute(word);
  bool hit = false;
  for (const auto& [oldS, newS] : respellSubstr()) {
    if (bare.find(oldS) != std::string::npos) {
      hit = true;
      break;
    }
  }
  if (!hit) return word;
  int vn = acuteVowelIndex(utf8::decode(word));
  for (const auto& [oldS, newS] : respellSubstr())
    bare = replaceAll(bare, oldS, newS);
  return reseatAcute(bare, vn);
}

bool isIpaVowel(char32_t c) {
  static const std::unordered_set<char32_t> k = {
      U'a', U'ɑ', U'o', U'e', U'i', U'u', U'y', U'ʌ', U'ə', U'ɪ', U'ɐ', U'ɛ', U'ɨ',
  };
  return k.count(c) != 0;
}

bool isReducible(char32_t c) {
  static const std::unordered_set<char32_t> k = {U'a', U'ɑ', U'o', U'ʌ', U'ə'};
  return k.count(c) != 0;
}

std::string reduceToken(const std::string& tokUtf8) {
  std::vector<char32_t> chars = utf8::decode(tokUtf8);
  std::vector<int> vowels;
  for (int i = 0; i < static_cast<int>(chars.size()); ++i)
    if (isIpaVowel(chars[static_cast<std::size_t>(i)])) vowels.push_back(i);
  if (vowels.empty()) return tokUtf8;

  int primary = -1;
  for (int i = 0; i < static_cast<int>(chars.size()); ++i)
    if (chars[static_cast<std::size_t>(i)] == U'ˈ') {
      primary = i;
      break;
    }

  std::unordered_set<int> stressed;
  for (int i = 0; i < static_cast<int>(chars.size()); ++i) {
    char32_t c = chars[static_cast<std::size_t>(i)];
    if (c != U'ˈ' && c != U'ˌ') continue;
    if (c == U'ˌ' && primary != -1 && i > primary) continue;
    auto nxt = std::find_if(vowels.begin(), vowels.end(), [&](int v) { return v > i; });
    if (nxt != vowels.end()) stressed.insert(*nxt);
  }

  std::vector<int> stressedOrd;
  for (int v : vowels)
    if (stressed.count(v))
      stressedOrd.push_back(static_cast<int>(
          std::find(vowels.begin(), vowels.end(), v) - vowels.begin()));

  for (std::size_t ordinal = 0; ordinal < vowels.size(); ++ordinal) {
    int i = vowels[ordinal];
    if (stressed.count(i)) continue;
    char32_t& ch = chars[static_cast<std::size_t>(i)];
    if (!isReducible(ch)) continue;
    if (i > 0 && (chars[static_cast<std::size_t>(i - 1)] == U'ʲ' ||
                  chars[static_cast<std::size_t>(i - 1)] == U'j')) {
      ch = U'ɪ';
      continue;
    }
    int nextStress = -1;
    for (int so : stressedOrd)
      if (so > static_cast<int>(ordinal)) {
        nextStress = so;
        break;
      }
    ch = (nextStress == static_cast<int>(ordinal) + 1 || i == 0) ? U'ɐ' : U'ə';
  }

  if (primary != -1) {
    std::vector<char32_t> out;
    out.reserve(chars.size());
    for (int i = 0; i < static_cast<int>(chars.size()); ++i) {
      char32_t c = chars[static_cast<std::size_t>(i)];
      if (c == U'ˌ' && i > primary) continue;
      out.push_back(c);
    }
    chars.swap(out);
  }
  return utf8::encode(chars);
}

struct E2M { const char* from; const char* to; };
const E2M kEspeakE2M[] = {
    {"a^\xC9\xAA", "I"},          {"a^\xCA\x8A", "W"},
    {"d^z", "\xCA\xA3"},          {"d^\xCA\x92", "\xCA\xA4"},
    {"e^\xC9\xAA", "A"},          {"o^\xCA\x8A", "O"},
    {"\xC9\x99^\xCA\x8A", "Q"},   {"s^s", "S"},
    {"t^s", "\xCA\xA6"},          {"t^\xCA\x83", "\xCA\xA7"},
    {"\xC9\x94^\xC9\xAA", "Y"},
};

std::string punctPhoneme(char32_t c) {
  switch (c) {
    case ';': case ':': case ',': case '.': case '!': case '?':
    case '(': case ')':
      return std::string(1, static_cast<char>(c));
    case 0x2014: return "\xE2\x80\x94";
    case 0x2026: return "\xE2\x80\xA6";
    case '-': case 0x2013: return "\xE2\x80\x94";
    case '"': return "\"";
    case 0x201C: return "\xE2\x80\x9C";
    case 0x201D: return "\xE2\x80\x9D";
    default: return "";
  }
}

std::vector<std::pair<std::string, std::string>> splitClauses(const std::string& text) {
  std::vector<std::pair<std::string, std::string>> out;
  std::vector<char32_t> cps = utf8::decode(text);
  std::string seg;
  auto flush = [&](const std::string& delim) {
    out.push_back({seg, delim});
    seg.clear();
  };
  for (char32_t c : cps) {
    std::string pp = punctPhoneme(c);
    bool isDelim = (c == ';' || c == ':' || c == ',' || c == '.' || c == '!' ||
                    c == '?' || c == 0x2026);
    if (isDelim) flush(pp);
    else utf8::append(seg, c);
  }
  if (!seg.empty()) flush("");
  return out;
}

std::string espeakRussianIpa(const std::string& text) {
  std::string out;
  for (auto& [seg, delim] : splitClauses(text)) {
    bool allWs = true;
    for (char c : seg)
      if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r')) allWs = false;
    if (!allWs) {
      std::string s = replaceAll(seg, "(", "\xC2\xAB");
      s = replaceAll(s, ")", "\xC2\xBB");
      std::string ps = Phonemizer::espeakRawIPA(s, "ru");
      for (const auto& e : kEspeakE2M) ps = replaceAll(ps, e.from, e.to);
      ps = replaceAll(ps, "^", "");
      ps = replaceAll(ps, "-", "");
      std::string cleaned;
      bool inFlag = false;
      for (char c : ps) {
        if (c == '(') {
          inFlag = true;
          continue;
        }
        if (c == ')') {
          inFlag = false;
          continue;
        }
        if (!inFlag) cleaned.push_back(c);
      }
      ps = replaceAll(cleaned, "\xC2\xAB", "(");
      ps = replaceAll(ps, "\xC2\xBB", ")");
      while (!ps.empty() && ps.back() == ' ') ps.pop_back();
      out += ps;
    }
    out += delim;
    out += ' ';
  }
  while (!out.empty() && out.back() == ' ') out.pop_back();
  return out;
}

} // namespace

bool isRussianVoice(const std::string& voiceName) {
  static const std::unordered_set<std::string> kNames = {"sveta", "masha", "dima"};
  if (kNames.count(voiceName)) return true;
  return !voiceName.empty() && voiceName[0] == 'r';
}

std::string RuG2P::foldBrackets(std::string text) {
  for (char32_t ch : {U'[', U'{'}) {
    std::string from = utf8::encode({ch});
    text = replaceAll(text, from, "(");
  }
  for (char32_t ch : {U']', U'}'}) {
    std::string from = utf8::encode({ch});
    text = replaceAll(text, from, ")");
  }
  return text;
}

std::string RuG2P::toLowerRu(std::string text) {
  std::vector<char32_t> cps = utf8::decode(text);
  for (char32_t& c : cps) {
    if (c >= 0x0410 && c <= 0x042F) c += (0x0430 - 0x0410);
    else if (c == 0x0401) c = 0x0451;
  }
  return utf8::encode(cps);
}

std::string RuG2P::respell(std::string text) {
  std::vector<char32_t> cps = utf8::decode(text);
  std::string out;
  out.reserve(text.size());
  std::string word;
  auto flushWord = [&]() {
    if (!word.empty()) {
      out += respellWord(word);
      word.clear();
    }
  };
  for (char32_t c : cps) {
    if (isRuLetter(c)) word += utf8::encode({c});
    else {
      flushWord();
      utf8::append(out, c);
    }
  }
  flushWord();
  return out;
}

std::string RuG2P::normalizeIpa(std::string ipa) {
  static const std::pair<const char*, const char*> kNorm[] = {
      {"u\"", "u"}, {"ɭ", "l"}, {"ɵ", "o"}, {"ʑ", "ʒ"},
      {"ʐ", "ʒ"},   {"ʧʲ", "ʧ"},
  };
  for (const auto& [from, to] : kNorm) ipa = replaceAll(ipa, from, to);
  return ipa;
}

std::string RuG2P::reduceVowels(const std::string& ipa) {
  std::string out;
  std::string tok;
  for (char c : ipa) {
    if (c == ' ') {
      if (!tok.empty()) {
        if (!out.empty()) out.push_back(' ');
        out += reduceToken(tok);
        tok.clear();
      }
    } else {
      tok.push_back(c);
    }
  }
  if (!tok.empty()) {
    if (!out.empty()) out.push_back(' ');
    out += reduceToken(tok);
  }
  return out;
}

std::string RuG2P::lengthenSch(std::string ipa) {
  std::string out;
  out.reserve(ipa.size() + 8);
  for (std::size_t i = 0; i < ipa.size(); ++i) {
    if (ipa[i] == '\xCA' && i + 1 < ipa.size() && static_cast<unsigned char>(ipa[i + 1]) == 0x95) {
      bool hasLen = i + 2 < ipa.size() && ipa[i + 2] == '\xCB' &&
                    i + 3 < ipa.size() && static_cast<unsigned char>(ipa[i + 3]) == 0x90;
      if (!hasLen) {
        out += "\xCA\x95\xCB\x90";
        i += 1;
        continue;
      }
    }
    out.push_back(ipa[i]);
  }
  return out;
}

std::string RuG2P::phonemize(const std::string& text) {
  std::string marked = respell(toLowerRu(foldBrackets(text)));
  std::string ps = normalizeIpa(espeakRussianIpa(marked));
  ps = lengthenSch(reduceVowels(ps));
  return ps;
}

} // namespace kokoro
