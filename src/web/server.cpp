#include "amped/api.h"
#include "amped/controller.h"
#include "amped/hal.h"
#include "amped/net.h"
#include "amped/version.h"
#include "hal/board.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#if AMPED_MOCK
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#else
#include <LittleFS.h>
#include <SPI.h>
#include <WebServer.h>
#include <WiFi.h>
#if defined(ETH_PHY_W5500)
#include <ETH.h>
#endif
#if defined(__has_include)
#if __has_include("secrets.h")
#include "secrets.h"
#endif
#endif
#include "secrets.example.h"
#endif

namespace amped {
namespace {

const char* mime_for(const std::string& path) {
  if (path.size() >= 5 && path.compare(path.size() - 5, 5, ".html") == 0) return "text/html";
  if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".css") == 0) return "text/css";
  if (path.size() >= 3 && path.compare(path.size() - 3, 3, ".js") == 0) {
    return "application/javascript";
  }
  if (path.size() >= 5 && path.compare(path.size() - 5, 5, ".json") == 0) return "application/json";
  if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".svg") == 0) return "image/svg+xml";
  return "text/plain";
}

std::string header_value(const std::string& headers, const char* name) {
  std::string key = std::string(name) + ":";
  size_t pos = 0;
  while (pos < headers.size()) {
    size_t line = headers.find("\r\n", pos);
    if (line == std::string::npos) line = headers.size();
    std::string h = headers.substr(pos, line - pos);
    if (h.size() >= key.size()) {
      bool match = true;
      for (size_t i = 0; i < key.size(); ++i) {
        char a = static_cast<char>(std::tolower(static_cast<unsigned char>(h[i])));
        char b = static_cast<char>(std::tolower(static_cast<unsigned char>(key[i])));
        if (a != b) {
          match = false;
          break;
        }
      }
      if (match) {
        size_t v = key.size();
        while (v < h.size() && (h[v] == ' ' || h[v] == '\t')) ++v;
        return h.substr(v);
      }
    }
    pos = line + 2;
  }
  return {};
}

}  // namespace

#if AMPED_MOCK

namespace {

bool safe_rel_path(const std::string& rel) {
  if (rel.empty() || rel[0] == '/') return false;
  if (rel.find("..") != std::string::npos) return false;
  return true;
}

std::string read_file(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

void send_all(int fd, const std::string& data) {  // NOLINT(misc-no-recursion)
  size_t off = 0;
  while (off < data.size()) {
    ssize_t n = ::send(fd, data.data() + off, data.size() - off, 0);
    if (n <= 0) return;
    off += static_cast<size_t>(n);
  }
}

void handle_client(int fd, const std::string& www_root) {
  std::string req;
  char buf[2048];
  while (req.find("\r\n\r\n") == std::string::npos && req.size() < 65536) {
    ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) return;
    req.append(buf, static_cast<size_t>(n));
  }
  const size_t hdr_end = req.find("\r\n\r\n");
  if (hdr_end == std::string::npos) return;
  std::string headers = req.substr(0, hdr_end);
  std::string body = req.substr(hdr_end + 4);

  std::istringstream first(headers);
  std::string method, path, ver;
  first >> method >> path >> ver;
  const size_t q = path.find('?');
  if (q != std::string::npos) path = path.substr(0, q);

  uint32_t need = 0;
  const std::string cl = header_value(headers, "Content-Length");
  if (!cl.empty()) need = static_cast<uint32_t>(std::strtoul(cl.c_str(), nullptr, 10));
  while (body.size() < need && body.size() < 65536) {
    ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
    if (n <= 0) break;
    body.append(buf, static_cast<size_t>(n));
  }
  if (need && body.size() > need) body.resize(need);

  const std::string key = header_value(headers, "X-Api-Key");

  std::string status_line = "200 OK";
  std::string ctype = "text/plain";
  std::string resp;

  if (path.rfind("/api/", 0) == 0) {
    HttpResponse r = api_handle(method.c_str(), path.c_str(), body.c_str(), key.c_str());
    if (r.status == 401) status_line = "401 Unauthorized";
    else if (r.status == 404) status_line = "404 Not Found";
    else if (r.status == 405) status_line = "405 Method Not Allowed";
    else if (r.status >= 400) status_line = "400 Bad Request";
    ctype = r.content_type;
    resp = r.body;
  } else {
    std::string rel = (path == "/" || path.empty()) ? "index.html" : path.substr(1);
    if (!safe_rel_path(rel)) {
      status_line = "400 Bad Request";
      resp = "bad path";
    } else {
      const std::string file = www_root + "/" + rel;
      resp = read_file(file);
      if (resp.empty()) {
        status_line = "404 Not Found";
        resp = "not found";
        ctype = "text/plain";
      } else {
        ctype = mime_for(rel);
      }
    }
  }

  std::ostringstream out;
  out << "HTTP/1.1 " << status_line << "\r\n"
      << "Content-Type: " << ctype << "\r\n"
      << "Content-Length: " << resp.size() << "\r\n"
      << "Access-Control-Allow-Origin: *\r\n"
      << "Access-Control-Allow-Headers: Content-Type, X-Api-Key\r\n"
      << "Connection: close\r\n\r\n"
      << resp;
  send_all(fd, out.str());
}

}  // namespace

bool http_serve(int port, const char* www_root) {
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return false;
  int opt = 1;
  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  sockaddr_in addr {};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(static_cast<uint16_t>(port));
  if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    close(fd);
    return false;
  }
  if (listen(fd, 8) < 0) {
    close(fd);
    return false;
  }
  std::cout << AMPED_PRODUCT << " mock " << AMPED_FW_VERSION << " http://127.0.0.1:" << port
            << "  www=" << www_root << std::endl;
  for (;;) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    timeval tv {};
    tv.tv_usec = 200000;
    const int ready = select(fd + 1, &rfds, nullptr, nullptr, &tv);
    controller().tick();
    if (ready > 0 && FD_ISSET(fd, &rfds)) {
      const int cfd = accept(fd, nullptr, nullptr);
      if (cfd >= 0) {
        handle_client(cfd, www_root);
        close(cfd);
      }
    }
  }
}

int native_main(int argc, char** argv) {
  int port = 8080;
  const char* www = "data/www";
  bool self_test = false;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--self-test") == 0) self_test = true;
    else if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = std::atoi(argv[++i]);
    else if (std::strcmp(argv[i], "--www") == 0 && i + 1 < argc) www = argv[++i];
  }
  if (self_test) {
    const bool ok = run_self_tests();
    std::cout << (ok ? "self-test: PASS\n" : "self-test: FAIL\n");
    return ok ? 0 : 1;
  }
  controller().begin();
  mqtt_begin();
  ota_begin();
  if (!http_serve(port, www)) {
    std::cerr << "failed to bind port " << port << std::endl;
    return 1;
  }
  return 0;
}

#else  // ESP32

namespace {

WebServer g_http(80);

String header_key() {
  if (!g_http.hasHeader("X-Api-Key")) return "";
  return g_http.header("X-Api-Key");
}

void send_api(const HttpResponse& r) {
  g_http.send(r.status, r.content_type, r.body.c_str());
}

void handle_api() {
  const String body = g_http.arg("plain");
  send_api(api_handle(g_http.method() == HTTP_POST ? "POST" : "GET", g_http.uri().c_str(),
                      body.c_str(), header_key().c_str()));
}

void handle_root() {
  if (LittleFS.exists("/www/index.html")) {
    File f = LittleFS.open("/www/index.html", "r");
    g_http.streamFile(f, "text/html");
    f.close();
    return;
  }
  if (LittleFS.exists("/index.html")) {
    File f = LittleFS.open("/index.html", "r");
    g_http.streamFile(f, "text/html");
    f.close();
    return;
  }
  g_http.send(200, "text/plain", "Amped VFD — upload LittleFS (data/www)");
}

bool begin_ethernet() {
#if defined(ETH_PHY_W5500)
  SPI.begin(AMPED_ETH_SCLK, AMPED_ETH_MISO, AMPED_ETH_MOSI);
  return ETH.begin(ETH_PHY_W5500, 1, AMPED_ETH_CS, AMPED_ETH_INT, AMPED_ETH_RST, SPI3_HOST,
                   AMPED_ETH_SCLK, AMPED_ETH_MISO, AMPED_ETH_MOSI);
#else
  Serial.println("[net] Arduino-ESP32 build lacks ETH_PHY_W5500; use Wi-Fi fallback");
  return false;
#endif
}

void begin_wifi_fallback() {
  if (!AMPED_WIFI_SSID[0]) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(AMPED_WIFI_SSID, AMPED_WIFI_PASS);
  Serial.printf("[net] Wi-Fi STA %s\n", AMPED_WIFI_SSID);
}

}  // namespace

bool http_serve(int, const char*) { return false; }
int native_main(int, char**) { return 1; }

void esp32_setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n%s %s\n", AMPED_PRODUCT, AMPED_FW_VERSION);

  LittleFS.begin(true);
  controller().begin();
  mqtt_begin();

  const bool eth = begin_ethernet();
  if (!eth) begin_wifi_fallback();

  static const char* kCollect[] = {"X-Api-Key"};
  g_http.collectHeaders(kCollect, 1);
  g_http.on("/api/status", HTTP_GET, handle_api);
  g_http.on("/api/temps", HTTP_GET, handle_api);
  g_http.on("/api/settings", HTTP_ANY, handle_api);
  g_http.on("/api/heartbeat", HTTP_POST, handle_api);
  g_http.on("/api/vfd/1", HTTP_POST, handle_api);
  g_http.on("/api/vfd/2", HTTP_POST, handle_api);
  g_http.on("/", HTTP_GET, handle_root);
  g_http.onNotFound([]() {
    String uri = g_http.uri();
    if (uri.startsWith("/api/")) {
      handle_api();
      return;
    }
    String path = uri;
    if (LittleFS.exists(("/www" + path).c_str())) {
      File f = LittleFS.open("/www" + path, "r");
      g_http.streamFile(f, mime_for(path.c_str()));
      f.close();
      return;
    }
    if (LittleFS.exists(path.c_str())) {
      File f = LittleFS.open(path, "r");
      g_http.streamFile(f, mime_for(path.c_str()));
      f.close();
      return;
    }
    g_http.send(404, "text/plain", "not found");
  });
  g_http.begin();
  ota_begin();
  Serial.println("[http] listening on :80");
}

void esp32_loop() {
  g_http.handleClient();
  controller().tick();
}

#endif

}  // namespace amped
