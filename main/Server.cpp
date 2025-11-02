#include "Server.hpp"

#include "Formatting.hpp"
#include "PlantWaterOS.hpp"

#include <WebServer.h>
#include <Stream.h>
#include <ElegantOTA.h>


// Globals
WebServer server(80);


// Forward declarations
void onHttpRoot();
void onHttpEventLog();


// Implementation

void initWebServer()
{
  server.on("/", onHttpRoot);
  server.on("/eventlog", onHttpEventLog);

  ElegantOTA.begin(&server);
  server.begin();
}

void handleServer(void* args)
{
  while (true) {
    server.handleClient();
    ElegantOTA.loop();
  }
}

void onHttpRoot()
{
  StreamString response;
  response << R"(<html>
<head>
  <title>PlantWaterOS</title>
</head>
<body>
  <h1>PlantWaterOS</h1>
  <div>Next pump time: )" << nextPumpTime
  << R"(</div>
</body>
</html>
)";

  server.send(200, "text/html", response);
}

void onHttpEventLog()
{
  server.send(200, "text/plain", eventLog);
}
