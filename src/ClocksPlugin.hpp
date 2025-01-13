#include <PluginInterface.h>

#include <memory>
#include <vector>

#include "ClockItem.hpp"
#include "StateStore.hpp"
#include "config.hpp"

using std::unique_ptr;
using std::vector;

class ClocksPlugin : public ITMPlugin {
 private:
  ClocksPlugin();

  static unsigned short int item_count;

  static ClocksPlugin instance;
  static unique_ptr<StateStore> state;
  static vector<ClockItem> clocks;

 public:
  static ClocksPlugin &Instance();

  virtual IPluginItem *GetItem(int index) override;
  virtual void DataRequired() override;
  virtual const wchar_t *GetInfo(PluginInfoIndex index) override;
};

extern "C" __declspec(dllexport) ITMPlugin *TMPluginGetInstance();
