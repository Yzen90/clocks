#include <PluginInterface.h>

#include <vector>

#include "ClockItem.hpp"
#include "StateStore.hpp"

using std::vector;

class ClocksPlugin : public ITMPlugin {
 private:
  ClocksPlugin();
  static ClocksPlugin instance;

  static StateStore *state;
  static vector<ClockItem> clocks;
  static ItemCount item_count;

  static void sync();

 public:
  static ClocksPlugin &Instance();

  virtual IPluginItem *GetItem(int index) override;
  virtual void DataRequired() override;
  virtual const wchar_t *GetInfo(PluginInfoIndex index) override;
  virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data) override;
};

extern "C" __declspec(dllexport) ITMPlugin *TMPluginGetInstance();
