#include "LogService.h"

#include <stdarg.h>
#include <stdio.h>

void LogService::begin() {
  next_ = 0;
  count_ = 0;
}

void LogService::add(const char* component, const char* format, ...) {
  const int prefixLength = snprintf(lines_[next_], kLineLength, "[%10lu] [%.12s] ",
                                    static_cast<unsigned long>(millis()), component);
  const size_t used = prefixLength > 0 && static_cast<size_t>(prefixLength) < kLineLength
                          ? static_cast<size_t>(prefixLength)
                          : kLineLength - 1;
  va_list arguments;
  va_start(arguments, format);
  vsnprintf(lines_[next_] + used, kLineLength - used, format, arguments);
  va_end(arguments);
  Serial.println(lines_[next_]);
  next_ = (next_ + 1) % kLineCount;
  if (count_ < kLineCount) ++count_;
}

void LogService::printTo(Stream& output) const {
  for (size_t index = 0; index < count_; ++index) {
    output.println(line(index));
  }
}

const char* LogService::line(size_t index) const {
  if (index >= count_) return "";
  const size_t first = count_ == kLineCount ? next_ : 0;
  return lines_[(first + index) % kLineCount];
}

