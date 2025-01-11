#include <PluginInterface.h>

#include <cstdint>

class ClockItem : public IPluginItem {
 private:
  uint8_t index;

 public:
  ClockItem(uint8_t index);

  virtual const wchar_t* GetItemName() const override;
  virtual const wchar_t* GetItemId() const override;
  virtual const wchar_t* GetItemLableText() const override;
  virtual const wchar_t* GetItemValueText() const override;
  virtual const wchar_t* GetItemValueSampleText() const override;
};
