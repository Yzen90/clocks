#include <plog/Log.h>

#include <boost/nowide/convert.hpp>
#include <chrono>
#include <iostream>
#include <thread>

#include "PluginInterface.h"
#include "plog/Initializers/RollingFileInitializer.h"
#include "src/ClocksPlugin.hpp"

using std::cout;
using std::chrono::current_zone;
using std::chrono::days;
using std::chrono::floor;
using std::chrono::hh_mm_ss;
using std::chrono::locate_zone;
using std::chrono::seconds;
using std::chrono::system_clock;
using std::chrono::zoned_time;

int main() {
  auto xd = TMPluginGetInstance()->GetInfo(ITMPlugin::TMI_NAME);

  plog::init(plog::debug, "main.log");

  PLOGD << "XD XD " + boost::nowide::narrow(xd);

  auto now = system_clock::now();
  auto minutes = hh_mm_ss{now - floor<days>(now)}.minutes();

  const zoned_time local_time{current_zone(), now};
  const zoned_time utc_time{locate_zone("UTC"), now};

  cout << "Hola " << local_time << " -- " << utc_time << " -- " << minutes << std::endl;

  std::this_thread::sleep_for(seconds(30));

  now = system_clock::now();
  auto later = hh_mm_ss{now - floor<days>(now)}.minutes();

  cout << minutes << " - " << later << " - is equal: " << (later == minutes) << std::endl;

  std::this_thread::sleep_for(seconds(30));

  now = system_clock::now();
  minutes = later;
  later = hh_mm_ss{now - floor<days>(now)}.minutes();

  cout << minutes << " - " << later << " - is equal: " << (later == minutes) << std::endl;

  return 0;
}
