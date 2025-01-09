#include "Config.hpp"
#include "PluginInterface.h"

class ClocksPlugin : public ITMPlugin {
 private:
  ClocksPlugin();

  static ClocksPlugin m_instance;

 public:
  static ClocksPlugin &Instance();

  virtual IPluginItem *GetItem(int index) override;
  virtual void DataRequired() override;
  virtual const wchar_t *GetInfo(PluginInfoIndex index) override;
};

extern "C" __declspec(dllexport) ITMPlugin *TMPluginGetInstance();
