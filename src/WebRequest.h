#pragma once

#include <Arduino.h>

class WebRequest {
 public:
  bool hasArg(const char* name) const;
  String arg(const char* name) const;

 private:
  friend class WebService;
  using HasArgReader = bool (*)(void*, const char*);
  using ArgReader = String (*)(void*, const char*);

  WebRequest(HasArgReader hasArgReader, ArgReader argReader, void* context)
      : hasArgReader_(hasArgReader), argReader_(argReader), context_(context) {}

  HasArgReader hasArgReader_ = nullptr;
  ArgReader argReader_ = nullptr;
  void* context_ = nullptr;
};
