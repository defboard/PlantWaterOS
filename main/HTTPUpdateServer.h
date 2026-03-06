#pragma once

#include <StreamString.h>
#include <Update.h>
#include <WebServer.h>

namespace {

    static const char serverIndex[] =
    R"(<!DOCTYPE html>
        <html lang='en'>
        <head>
            <meta charset='utf-8'>
            <meta name='viewport' content='width=device-width,initial-scale=1'/>
        </head>
        <body>
        <form method='POST' action='' enctype='multipart/form-data'>
            Firmware:<br>
            <input type='file' accept='.bin,.bin.gz' name='firmware'>
            <input type='submit' value='Update Firmware'>
        </form>
        </body>
        </html>)";
    static const char successResponse[] = "<META http-equiv=\"refresh\" content=\"15;URL=/\">Update Success! Rebooting...";


    uint32_t maxSketchSpace()
    {
      return (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    }
}

class HTTPUpdateServer {
public:

  void setup(WebServer *server, const char* path)
  {
    server->on(path, HTTP_GET,
        [server]() { getHandler(server); });

    server->on(path, HTTP_POST,
        [server]() { finishHandler(server); },
        [server]() { uploadHandler(server); });
  }

  static void getHandler(WebServer *server)
  {
    server->send(200, "text/html", serverIndex);
  }

  static void finishHandler(WebServer *server)
  {
    if (Update.hasError()) {
      server->send(200, "text/html", String("Update error: ") + Update.errorString());
      Update.clearError();
      return;
    }

    server->client().setNoDelay(true);
    server->send(200, "text/html", successResponse);
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
