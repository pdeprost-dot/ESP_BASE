#pragma once

#include <Arduino.h>

class LogService {
 public:
  static constexpr size_t kLineCount = 16;
  static constexpr size_t kLineLength = 96;

  void begin();
  void add(const char* component, const char* format, ...);
  void printTo(Stream& output) const;
  size_t count() const { return count_; }
  const char* line(size_t index) const;

 private:
  char lines_[kLineCount][kLineLength]{};
  size_t next_ = 0;
  size_t count_ = 0;
};

