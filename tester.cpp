#include <Windows.h>
#undef ERROR

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnontrivial-memcall"
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Globalization.DateTimeFormatting.h>
#pragma clang diagnostic pop

#include <chrono>
#include <iostream>
#include <locale>
#include <nowide/convert.hpp>
#include <thread>

#include "PluginInterface.h"
#include "src/ClocksPlugin.hpp"

using nowide::narrow;
using std::cout;
using std::endl;
using std::format;
using std::remove;
using std::string;
using std::wcout;
using winrt::clock;
using namespace std::chrono;
using namespace winrt::Windows::Globalization;
using namespace winrt::Windows::Globalization::DateTimeFormatting;

vector<uint32_t> get_utf8_codepoints(const string& sample) {
  vector<uint32_t> codepoints;
  const auto size = sample.size();
  for (size_t i = 0; i < size;) {
    uint8_t lead = static_cast<uint8_t>(sample[i]);

    // Determine the number of bytes in the code point
    size_t len = 1;
    if ((lead >> 3) == 0x1E)
      len = 4;  // 11110xxx (4 bytes)
    else if ((lead >> 4) == 0x0E)
      len = 3;  // 1110xxxx (3 bytes)
    else if ((lead >> 5) == 0x06)
      len = 2;  // 110xxxxx (2 bytes)
    else if ((lead >> 7) == 0x00)
      len = 1;  // 0xxxxxxx (1 byte)

    // Check if valid length remains
    if (i + len > sample.size()) len = size - i;

    // Extract code point
    uint32_t cp = 0;
    switch (len) {
      case 4:
        cp |= (static_cast<uint8_t>(sample[i]) & 0x07) << 18;
        cp |= (static_cast<uint8_t>(sample[i + 1]) & 0x3F) << 12;
        cp |= (static_cast<uint8_t>(sample[i + 2]) & 0x3F) << 6;
        cp |= static_cast<uint8_t>(sample[i + 3]) & 0x3F;
        break;
      case 3:
        cp |= (static_cast<uint8_t>(sample[i]) & 0x0F) << 12;
        cp |= (static_cast<uint8_t>(sample[i + 1]) & 0x3F) << 6;
        cp |= static_cast<uint8_t>(sample[i + 2]) & 0x3F;
        break;
      case 2:
        cp |= (static_cast<uint8_t>(sample[i]) & 0x1F) << 6;
        cp |= static_cast<uint8_t>(sample[i + 1]) & 0x3F;
        break;
      default:
        cp = static_cast<uint8_t>(sample[i]);
    }
    codepoints.push_back(cp);
    i += len;
  }
  return codepoints;
}

void remove_lrm(wstring& target) { target.erase(remove(target.begin(), target.end(), L'\u200E'), target.end()); }

int main() {
  std::locale::global(std::locale{".utf-8"});
  SetConsoleOutputCP(CP_UTF8);

  try {
    auto throws = locate_zone("Colabola");
  } catch (...) {
    cout << "Invalid timezone" << endl;
  }

  TMPluginGetInstance()->OnExtenedInfo(ITMPlugin::EI_CONFIG_DIR, L"./");

  auto plug_name = TMPluginGetInstance()->GetInfo(ITMPlugin::TMI_NAME);
  auto item_name = TMPluginGetInstance()->GetItem(0)->GetItemName();

  cout << "Plugin name: " + narrow(plug_name) << endl;
  cout << "Item name: " + narrow(item_name) << endl;

  TMPluginGetInstance()->ShowOptionsDialog(GetConsoleWindow());

  // ----------- STD -----------

  auto now = system_clock::now();

  const zoned_time local_time{current_zone(), now};
  const zoned_time utc_time{locate_zone("UTC"), now};
  const zoned_time jst_time{locate_zone("Asia/Tokyo"), now};

  cout << "\nSTD -----------" << endl;

  cout << "Local 24h: " << format("{:%X}", local_time) << endl;
  cout << "Local 12h: " << format("{:%r}", local_time) << endl;
  cout << "UTC 24h: " << format("{:%X}", utc_time) << endl;
  cout << "UTC 12h: " << format("{:%r}", utc_time) << endl;
  cout << "JST 24h: " << format("{:%X}", jst_time) << endl;
  cout << "JST 12h: " << format("{:%r}", jst_time) << endl;

  auto now_floored = floor<days>(now);
  auto day = year_month_day{now_floored}.day();
  auto time = hh_mm_ss{now - now_floored};
  auto hour = time.hours();
  auto minutes = time.minutes();

  cout << "SYS Day: " << day << " - SYS Hour: " << hour << " - SYS Minutes: " << minutes << endl;

  auto local = local_time.get_local_time();
  auto local_floored = floor<days>(local);
  day = year_month_day{local_floored}.day();
  time = hh_mm_ss{local - local_floored};
  hour = time.hours();
  minutes = time.minutes();

  cout << "LOC Day: " << day << " - LOC Hour: " << hour << " - LOC Minutes: " << minutes << endl;

  auto jst = jst_time.get_local_time();
  auto jst_floored = floor<days>(jst);
  day = year_month_day{jst_floored}.day();
  time = hh_mm_ss{jst - jst_floored};
  hour = time.hours();
  minutes = time.minutes();

  cout << "JST Day: " << day << " - JST Hour: " << hour << " - JST Minutes: " << minutes << endl;

  // ----------- WIN -----------

  DateTimeFormatter formatter{L"shorttime"};
  DateTimeFormatter formatter_24h(
      L"shorttime", formatter.Languages(), formatter.GeographicRegion(), formatter.Calendar(),
      ClockIdentifiers::TwentyFourHour()
  );
  DateTimeFormatter formatter_12h(
      L"shorttime", formatter.Languages(), formatter.GeographicRegion(), formatter.Calendar(),
      ClockIdentifiers::TwelveHour()
  );

  auto now_win = clock::now();
  auto now_converted = clock::from_sys(now);

  cout << "\nWIN -----------" << endl;

  auto local_win = wstring{formatter.Format(now_win)};
  wcout << L"Local auto: [" << local_win << L"]" << endl;

  auto local_utf8 = narrow(wstring{local_win});
  auto local_codepoints = get_utf8_codepoints(local_utf8);
  cout << "Local utf8 codepoints:" << endl;

  for (auto& codepoint : local_codepoints) {
    cout << format("{:x}", codepoint) << endl;
  }

  remove_lrm(local_win);
  wcout << endl << L"Local auto clean: [" << local_win << L"]" << endl;

  wcout << L"Local 24h: [" << formatter_24h.Format(now_win) << L"]" << endl;
  wcout << L"Local 12h: [" << formatter_12h.Format(now_win) << L"]" << endl;

  wcout << L"UTC auto: [" << formatter.Format(now_win, L"UTC") << L"]" << endl;
  wcout << L"UTC 24h: [" << formatter_24h.Format(now_win, L"UTC") << L"]" << endl;
  wcout << L"UTC 12h: [" << formatter_12h.Format(now_win, L"UTC") << L"]" << endl;

  wcout << L"Converted Local: [" << formatter.Format(now_converted) << L"]" << endl;
  wcout << L"Converted UTC: [" << formatter.Format(now_converted, L"UTC") << L"]" << endl;

  wcout << L"JST auto: [" << formatter.Format(now_win, L"Asia/Tokyo") << L"]" << endl;

  // ---------------------------

  cout << "\n---------------" << endl;

  cout << "Hola " << local_time << " -- " << utc_time << " -- " << minutes << endl;

  std::this_thread::sleep_for(seconds(30));

  now = system_clock::now();
  auto later = hh_mm_ss{now - floor<days>(now)}.minutes();

  cout << std::boolalpha;
  cout << minutes << " - " << later << " - is equal: " << (later == minutes) << endl;

  std::this_thread::sleep_for(seconds(30));

  now = system_clock::now();
  minutes = later;
  later = hh_mm_ss{now - floor<days>(now)}.minutes();

  cout << minutes << " - " << later << " - is equal: " << (later == minutes) << endl;

  // ---------- TZDB -----------

  cout << "\n---------------" << endl;

  auto& tzdb = get_tzdb();

  vector<string> timezones;

  for (const auto& zone : tzdb.zones) {
    timezones.push_back(string{zone.name()});
  }

  for (const auto& tz : timezones) {
    std::cout << tz << std::endl;
  }

  return 0;
}
