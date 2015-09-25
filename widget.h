#ifndef LAPS2_WIDGET_H_
#define LAPS2_WIDGET_H_

#include "util.h"
#include <list>

struct Widget;
class WidgetList final : public std::list<Widget*>, public util::Singleton<WidgetList> {};

struct Widget {
  virtual void Init(int argc, char** argv) = 0;
  virtual const uint8_t* GetState() = 0;
  virtual std::list<int> GetPollFd() = 0;
  virtual void Activate() = 0;
  virtual void Handle() = 0;

  Widget() {
    WidgetList::Get().push_back(this);
  }
};

#endif  // LAPS2_WIDGET_H_
