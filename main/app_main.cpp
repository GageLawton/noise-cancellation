#include "anc/node_runtime.hpp"

namespace {
// Static storage: the runtime outlives app_main's stack frame and
// nothing here should be heap-allocated.
anc::NodeRuntime g_runtime;
}  // namespace

extern "C" void app_main(void) {
    g_runtime.start();
}
