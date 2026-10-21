#pragma once
#ifndef FileServerHTTP_h
#define FileServerHTTP_h

// Self-contained SD Wi-Fi file server (synchronous WebServer + FTP).
// Implemented in its own translation unit so <WebServer.h> is isolated from
// ESPAsyncWebServer (which EvilPortal pulls in elsewhere and whose HTTP_GET
// enum conflicts with WebServer's). Call runFileServer() to start it; it blocks
// (serving in its own loop) until the on-screen Stop button is tapped.
void runFileServer();

// Wi-Fi OTA update: join a saved network (/wifi_creds), fetch the latest GitHub release's
// .bin asset over HTTPS, flash it (auto-handles a merged 0x0 image), and reboot. Blocks.
// IMPORTANT: this is run EARLY in setup() (see requestWifiUpdate), while the heap is still clean,
// because the GitHub TLS handshake needs a large contiguous block the fully-loaded UI can't spare.
void runWifiUpdate();

// Called from the menu: flag a WiFi update and reboot. The actual update runs at the next boot,
// before the heavy firmware init, so TLS has the RAM it needs.
void requestWifiUpdate();

#endif
