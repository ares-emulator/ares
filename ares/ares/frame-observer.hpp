#pragma once
#include <functional>

namespace ares::Video {
// Optional host observer, invoked on the emulation thread after the previous
// refresh finishes and before this screen's next frame is queued. Install before
// running a core and remove after unload. No capture work occurs by default.
inline std::function<void(const void*)> frameSubmittedObserver;
}
