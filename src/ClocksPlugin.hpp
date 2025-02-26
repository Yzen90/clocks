#include <PluginInterface.h>

#include <vector>

#include "ClockItem.hpp"
#include "StateStore.hpp"

using std::vector;

class ClocksPlugin : public ITMPlugin {
 private:
  ClocksPlugin();
  ClocksPlugin(const ClocksPlugin &) = delete;
  ClocksPlugin &operator=(const ClocksPlugin &) = delete;

  StateStore &state;
  vector<ClockItem> clocks;

 public:
  static ClocksPlugin &instance();

  virtual IPluginItem *GetItem(int index) override;
  virtual void DataRequired() override;
  virtual const wchar_t *GetInfo(PluginInfoIndex index) override;
  virtual void OnExtenedInfo(ExtendedInfoIndex index, const wchar_t *data) override;
  virtual OptionReturn ShowOptionsDialog(void *parent) override;
};

extern "C" __declspec(dllexport) ITMPlugin *TMPluginGetInstance();
