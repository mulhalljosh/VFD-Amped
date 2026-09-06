#include "amped/api.h"

#include "amped/controller.h"
#include "amped/hal.h"
#include "amped/json.h"
#include "amped/net.h"
#include "amped/version.h"

#include <cstdio>
#include <cstring>
#include <sstream>

namespace amped {
namespace {

std::string fmt_f(float v, int digits = 2) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), digits == 3 ? "%.3f" : "%.2f", static_cast<double>(v));
  return buf;
}

std::string status_json(const SystemStatus& s) {
  std::ostringstream o;
  o << "{\"ok\":true,\"version\":\"" << s.version << "\",\"mock\":" << (s.mock ? "true" : "false")
    << ",\"uptime_ms\":" << s.uptime_ms << ",\"heartbeat_ok\":" << (s.heartbeat_ok ? "true" : "false")
    << ",\"heartbeat_age_ms\":" << s.heartbeat_age_ms
    << ",\"network_ok\":" << (s.network_ok ? "true" : "false")
    << ",\"interlock_enabled\":" << (s.interlock_enabled ? "true" : "false")
    << ",\"interlock_blocking_cooler\":" << (s.interlock_blocking_cooler ? "true" : "false")
    << ",\"product\":\"" << AMPED_PRODUCT << "\",\"company\":\"" << AMPED_COMPANY << "\""
    << ",\"vfds\":[";
  for (int i = 0; i < kChannelCount; ++i) {
    const VfdStatus& v = s.vfd[i];
    if (i) o << ',';
    o << "{\"channel\":" << v.channel << ",\"name\":\"" << v.name << "\",\"mode\":\""
      << mode_name(v.mode) << "\",\"speed_pct\":" << fmt_f(v.speed_pct)
      << ",\"commanded_speed_pct\":" << fmt_f(v.commanded_speed_pct)
      << ",\"run\":" << (v.run ? "true" : "false")
      << ",\"commanded_run\":" << (v.commanded_run ? "true" : "false")
      << ",\"fault\":" << (v.fault ? "true" : "false") << ",\"ao_volts\":" << fmt_f(v.ao_volts, 3)
      << ",\"ao_ma\":" << fmt_f(v.ao_ma, 3) << ",\"failsafe\":\"" << failsafe_name(v.failsafe)
      << "\",\"analog_path\":\"" << analog_path_name(v.analog_path) << "\"}";
  }
  o << "],\"temps\":[";
  for (int i = 0; i < kTempCount; ++i) {
    const TempReading& t = s.temps[i];
    if (i) o << ',';
    o << "{\"id\":\"" << t.id << "\",\"label\":\"" << t.label
      << "\",\"celsius\":" << fmt_f(t.celsius) << ",\"valid\":" << (t.valid ? "true" : "false")
      << "}";
  }
  o << "]}";
  return o.str();
}

std::string temps_json() {
  controller().tick();
  const SystemStatus s = controller().status();
  std::ostringstream o;
  o << "{\"ok\":true,\"temps\":[";
  for (int i = 0; i < kTempCount; ++i) {
    const TempReading& t = s.temps[i];
    if (i) o << ',';
    o << "{\"id\":\"" << t.id << "\",\"label\":\"" << t.label
      << "\",\"celsius\":" << fmt_f(t.celsius) << ",\"valid\":" << (t.valid ? "true" : "false")
      << "}";
  }
  o << "]}";
  return o.str();
}

std::string settings_json() {
  const AppConfig& c = controller().config();
  std::ostringstream o;
  o << "{\"ok\":true,\"heartbeat_timeout_ms\":" << c.heartbeat_timeout_ms
    << ",\"interlock_cooler_requires_pump\":"
    << (c.interlock_cooler_requires_pump ? "true" : "false") << ",\"api_key_set\":"
    << (c.api_key[0] ? "true" : "false") << ",\"hostname\":\"" << json_escape(c.hostname)
    << "\",\"channels\":[";
  for (int i = 0; i < kChannelCount; ++i) {
    if (i) o << ',';
    o << "{\"channel\":" << (i + 1) << ",\"failsafe\":\"" << failsafe_name(c.ch[i].failsafe)
      << "\",\"preset_pct\":" << fmt_f(c.ch[i].preset_pct)
      << ",\"preset_run\":" << (c.ch[i].preset_run ? "true" : "false") << ",\"analog_path\":\""
      << analog_path_name(c.ch[i].analog_path) << "\"}";
  }
  o << "]}";
  return o.str();
}

HttpResponse json_ok(const std::string& body) { return {200, "application/json", body}; }

HttpResponse json_err(int code, const char* msg) {
  return {code, "application/json", std::string("{\"ok\":false,\"error\":\"") + msg + "\"}"};
}

int path_vfd_channel(const char* path) {
  if (std::strcmp(path, "/api/vfd/1") == 0) return 0;
  if (std::strcmp(path, "/api/vfd/2") == 0) return 1;
  return -1;
}

}  // namespace

bool api_authorized(const char* api_key_header) {
  const char* need = controller().config().api_key;
  if (!need || !need[0]) return true;
  if (!api_key_header || !api_key_header[0]) return false;
  return std::strcmp(need, api_key_header) == 0;
}

HttpResponse api_handle(const char* method, const char* path, const char* body,
                        const char* api_key_header) {
  if (!method) method = "GET";
  if (!path) path = "/";
  if (!body) body = "";

  if (std::strncmp(path, "/api/", 5) == 0 && !api_authorized(api_key_header)) {
    return json_err(401, "unauthorized");
  }

  if (std::strncmp(path, "/api/", 5) == 0) {
    controller().touch_heartbeat();
  }

  if (std::strcmp(method, "GET") == 0 && std::strcmp(path, "/api/status") == 0) {
    controller().tick();
    return json_ok(status_json(controller().status()));
  }
  if (std::strcmp(method, "GET") == 0 && std::strcmp(path, "/api/temps") == 0) {
    return json_ok(temps_json());
  }
  if (std::strcmp(method, "GET") == 0 && std::strcmp(path, "/api/settings") == 0) {
    return json_ok(settings_json());
  }
  if (std::strcmp(method, "POST") == 0 && std::strcmp(path, "/api/heartbeat") == 0) {
    controller().touch_heartbeat();
    return json_ok("{\"ok\":true,\"heartbeat\":\"ok\"}");
  }

  const int vfd_ch = path_vfd_channel(path);
  if (vfd_ch >= 0) {
    if (std::strcmp(method, "POST") != 0) return json_err(405, "method_not_allowed");
    VfdCommand cmd = controller().command(vfd_ch);
    std::string mode, run_s;
    float speed = cmd.speed_pct;
    bool run = cmd.run;
    if (json_get_string(body, "mode", mode)) cmd.mode = parse_mode(mode.c_str());
    if (json_get_float(body, "speed_pct", speed)) cmd.speed_pct = speed;
    if (json_get_bool(body, "run", run)) cmd.run = run;
    if (!controller().set_vfd(vfd_ch, cmd)) return json_err(400, "bad_channel");
    controller().tick();
    return json_ok(status_json(controller().status()));
  }

  if (std::strcmp(method, "POST") == 0 && std::strcmp(path, "/api/settings") == 0) {
    AppConfig& c = controller().config();
    uint32_t hb = c.heartbeat_timeout_ms;
    if (json_get_uint32(body, "heartbeat_timeout_ms", hb)) c.heartbeat_timeout_ms = hb;
    bool interlock = c.interlock_cooler_requires_pump;
    if (json_get_bool(body, "interlock_cooler_requires_pump", interlock)) {
      c.interlock_cooler_requires_pump = interlock;
    }
    std::string key;
    if (json_get_string(body, "api_key", key)) {
      std::snprintf(c.api_key, sizeof(c.api_key), "%s", key.c_str());
    }
    std::string host;
    if (json_get_string(body, "hostname", host) && host.size() < sizeof(c.hostname)) {
      std::snprintf(c.hostname, sizeof(c.hostname), "%s", host.c_str());
    }
    for (int i = 0; i < kChannelCount; ++i) {
      // Accept either nested objects (ignored by this tiny parser) or flat keys
      // failsafe_1 / preset_pct_1 / analog_path_1.
      char fk[32], pk[32], ak[32];
      std::snprintf(fk, sizeof(fk), "failsafe_%d", i + 1);
      std::snprintf(pk, sizeof(pk), "preset_pct_%d", i + 1);
      std::snprintf(ak, sizeof(ak), "analog_path_%d", i + 1);
      std::string fs, ap;
      float preset = c.ch[i].preset_pct;
      if (json_get_string(body, fk, fs)) c.ch[i].failsafe = parse_failsafe(fs.c_str());
      if (json_get_float(body, pk, preset)) c.ch[i].preset_pct = clampf(preset, 0, 100);
      if (json_get_string(body, ak, ap)) c.ch[i].analog_path = parse_analog_path(ap.c_str());
    }
    controller().apply_config();
    nvs_save();
    return json_ok(settings_json());
  }

#if AMPED_MOCK
  if (std::strcmp(method, "POST") == 0 && std::strcmp(path, "/api/mock/fault") == 0) {
    float chf = 1;
    bool fault = false;
    json_get_float(body, "channel", chf);
    json_get_bool(body, "fault", fault);
    int ch = static_cast<int>(chf) - 1;
    if (ch < 0 || ch >= kChannelCount) return json_err(400, "bad_channel");
    controller().mock_set_fault(ch, fault);
    controller().tick();
    return json_ok(status_json(controller().status()));
  }
#endif

  return json_err(404, "not_found");
}

bool run_self_tests() {
  controller().begin();

  HttpResponse st = api_handle("GET", "/api/status", "", "");
  if (st.status != 200 || st.body.find("\"vfds\"") == std::string::npos) return false;
  if (st.body.find("\"pump\"") == std::string::npos || st.body.find("\"cooler\"") == std::string::npos) {
    return false;
  }

  HttpResponse t = api_handle("GET", "/api/temps", "", "");
  if (t.status != 200 || t.body.find("onboard") == std::string::npos) return false;

  HttpResponse p = api_handle("POST", "/api/vfd/1",
                              "{\"mode\":\"manual\",\"speed_pct\":42,\"run\":true}", "");
  if (p.status != 200 || p.body.find("42.00") == std::string::npos) return false;
  if (controller().status().vfd[0].ao_volts < 4.1f || controller().status().vfd[0].ao_volts > 4.3f) {
    return false;
  }

  HttpResponse c = api_handle("POST", "/api/vfd/2",
                              "{\"mode\":\"auto\",\"speed_pct\":10,\"run\":true}", "");
  if (c.status != 200 || controller().status().vfd[1].mode != VfdMode::Auto) return false;

  // Interlock: cooler must drop when pump is off.
  AppConfig& cfg = controller().config();
  cfg.interlock_cooler_requires_pump = true;
  api_handle("POST", "/api/vfd/1", "{\"run\":false,\"speed_pct\":0}", "");
  api_handle("POST", "/api/vfd/2", "{\"run\":true,\"speed_pct\":80}", "");
  controller().tick();
  if (controller().status().vfd[1].commanded_run) return false;

  // Failsafe zero after heartbeat timeout.
  cfg.interlock_cooler_requires_pump = false;
  cfg.ch[0].failsafe = FailsafeAction::Zero;
  cfg.heartbeat_timeout_ms = 1;
  api_handle("POST", "/api/vfd/1", "{\"run\":true,\"speed_pct\":90}", "");
  // Age the heartbeat without touching it.
  const uint32_t start = now_ms();
  while (now_ms() - start < 5) {
  }
  controller().tick();
  if (controller().status().vfd[0].commanded_speed_pct > 0.1f) return false;
  if (controller().status().vfd[0].commanded_run) return false;

  // Auth stub: empty key is open; set key and reject.
  std::snprintf(cfg.api_key, sizeof(cfg.api_key), "%s", "lab-key");
  HttpResponse denied = api_handle("GET", "/api/status", "", "");
  if (denied.status != 401) return false;
  HttpResponse allowed = api_handle("GET", "/api/status", "", "lab-key");
  if (allowed.status != 200) return false;
  cfg.api_key[0] = 0;

  // GP8413 + companion current DAC: 100% → 10 V / 20 mA codes.
  if (DacGp8413::pct_to_code(100.0f) != DacGp8413::kFullScale) return false;
  if (DacGp8413::pct_to_ma(0.0f) < 3.99f || DacGp8413::pct_to_ma(0.0f) > 4.01f) return false;
  if (DacGp8413::pct_to_ma(100.0f) < 19.99f || DacGp8413::pct_to_ma(100.0f) > 20.01f) return false;

  // Current path is a companion DAC write (does not touch GP8413 VOUT).
  if (DacGp8413::kAddrCurrentPump != 0x59 || DacGp8413::kAddrCurrentCooler != 0x5A) return false;
  dac().write_speed(0, 50.0f, AnalogPath::Voltage);
  const uint16_t v_before = dac().code(0);
  dac().write_speed(0, 25.0f, AnalogPath::Current);
  if (dac().code(0) != v_before) return false;
  if (dac().milliamps(0) < 7.9f || dac().milliamps(0) > 8.1f) return false;
  dac().write_speed(0, 0.0f, AnalogPath::Both);

  return true;
}

}  // namespace amped
