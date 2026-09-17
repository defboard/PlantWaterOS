#pragma once

#include "Json.hpp"

#include <StreamString.h>
#include <Update.h>
#include <WebServer.h>

namespace {
    uint32_t maxSketchSpace()
    {
      return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    }
}

class HTTPUpdateServer {
public:

  void setup(WebServer *server, const char* path)
  {
    server->on(path, HTTP_POST,
        [server]() { finishHandler(server); },
        [server]() { uploadHandler(server); });
  }

  static void finishHandler(WebServer *server)
  {
    const bool hasError = !Update.hasError();
    const int code = Update.getError();
    const String message = hasError
        ? Update.errorString()
        : "Update successful! Rebooting...";
    Update.clearError();

    StreamString response;
    JsonWriter json(response);
    json.put_object();
    json.put_plain("code", code);
    json.put_string("message", message);
    json.end_object();

    if (hasError) {
      server->send(200, "application/json", (String&) response);
      return;
    }

    server->client().setNoDelay(true);
    server->send(200, "application/json", (String&) response);
    delay(100);
    server->client().stop();
    ESP.restart();
  }

  static void uploadHandler(WebServer* server)
  {
    HTTPUpload& upload = server->upload();

    switch (upload.status) {
      case UPLOAD_FILE_START:
        Update.begin(maxSketchSpace(), U_FLASH);
        break;

      case UPLOAD_FILE_WRITE:
        Update.write(upload.buf, upload.currentSize);
        break;

      case UPLOAD_FILE_END:
        Update.end(true);
        break;

      case UPLOAD_FILE_ABORTED:
        Update.abort();
        break;
    }
  }

};
