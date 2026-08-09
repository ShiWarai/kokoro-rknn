#include "mp3_encoder.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>

namespace kokoro_server {

bool ffmpegAvailable() {
  std::array<char, 128> buf{};
  FILE* pipe = popen("ffmpeg -version 2>/dev/null", "r");
  if (!pipe) return false;
  const bool ok = fgets(buf.data(), static_cast<int>(buf.size()), pipe) != nullptr;
  pclose(pipe);
  return ok;
}

std::vector<uint8_t> encodeMp3(const int16_t* pcm, std::size_t n_samples,
                               int sample_rate) {
  if (!pcm || n_samples == 0)
    throw std::runtime_error("empty PCM for mp3 encode");

  int in_pipe[2]{};
  int out_pipe[2]{};
  if (pipe(in_pipe) != 0 || pipe(out_pipe) != 0)
    throw std::runtime_error("pipe() failed");

  const pid_t pid = fork();
  if (pid < 0)
    throw std::runtime_error("fork() failed");

  if (pid == 0) {
    dup2(in_pipe[0], STDIN_FILENO);
    dup2(out_pipe[1], STDOUT_FILENO);
    close(in_pipe[0]);
    close(in_pipe[1]);
    close(out_pipe[0]);
    close(out_pipe[1]);

    const std::string sr = std::to_string(sample_rate);
    execlp("ffmpeg", "ffmpeg", "-hide_banner", "-loglevel", "error",
           "-f", "s16le", "-ar", sr.c_str(), "-ac", "1", "-i", "pipe:0",
           "-f", "mp3", "-q:a", "4", "pipe:1", nullptr);
    _exit(127);
  }

  close(in_pipe[0]);
  close(out_pipe[1]);

  const auto* bytes = reinterpret_cast<const char*>(pcm);
  const std::size_t nbytes = n_samples * sizeof(int16_t);
  std::size_t written = 0;
  while (written < nbytes) {
    const ssize_t n = write(in_pipe[1], bytes + written, nbytes - written);
    if (n <= 0) break;
    written += static_cast<std::size_t>(n);
  }
  close(in_pipe[1]);

  std::vector<uint8_t> mp3;
  std::array<char, 4096> chunk{};
  for (;;) {
    const ssize_t n = read(out_pipe[0], chunk.data(), chunk.size());
    if (n <= 0) break;
    mp3.insert(mp3.end(), chunk.data(), chunk.data() + n);
  }
  close(out_pipe[0]);

  int status = 0;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || mp3.empty())
    throw std::runtime_error("ffmpeg mp3 encode failed");
  return mp3;
}

} // namespace kokoro_server
