#pragma once

#include <Arduino.h>

class WebResponse {
 public:
  void sendJson(const char* body, uint16_t statusCode = 200);
  void sendText(const char* body, uint16_t statusCode = 200);
  bool beginPage(const char* title);
  void write(const char* html);
  void endPage();
  bool sent() const { return sent_; }

 private:
  friend class WebService;
  using Sender = void (*)(void*, uint16_t, const char*, const char*);
  using PageBegin = void (*)(void*, const char*);
  using PageWrite = void (*)(void*, const char*);
  using PageEnd = void (*)(void*);

  WebResponse(Sender sender, PageBegin pageBegin, PageWrite pageWrite,
              PageEnd pageEnd, void* senderContext)
      : sender_(sender), pageBegin_(pageBegin), pageWrite_(pageWrite),
        pageEnd_(pageEnd), senderContext_(senderContext) {}
  void send(uint16_t statusCode, const char* contentType, const char* body);

  Sender sender_ = nullptr;
  PageBegin pageBegin_ = nullptr;
  PageWrite pageWrite_ = nullptr;
  PageEnd pageEnd_ = nullptr;
  void* senderContext_ = nullptr;
  bool sent_ = false;
  bool pageOpen_ = false;
};

using WebRouteHandler = void (*)(WebResponse&, void*);
