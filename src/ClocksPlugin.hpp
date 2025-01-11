#include <PluginInterface.h>

#include <vector>

#include "ClockItem.hpp"
#include "config.hpp"

using std::vector;

class ClocksPlugin : public ITMPlugin {
 private:
  ClocksPlugin();

  vector<ClockItem> clocks;
  int items;

  static ClocksPlugin instance;

 public:
  static ClocksPlugin &Instance();

  virtual IPluginItem *GetItem(int index) override;
  virtual void DataRequired() override;
  virtual const wchar_t *GetInfo(PluginInfoIndex index) override;
};

extern "C" __declspec(dllexport) ITMPlugin *TMPluginGetInstance();
