#ifndef WEBSERVER_H
#define WEBSERVER_H

#ifdef ENABLE_WEB_STATS

#include <ESPAsyncWebServer.h>

void setupWebServer();
void handleWebServerClient();

#endif // ENABLE_WEB_STATS

#endif // WEBSERVER_H
