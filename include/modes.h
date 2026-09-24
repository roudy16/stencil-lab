#ifndef SCR_MODES_H
#define SCR_MODES_H

#include "stencil.h"

namespace scr {

void run_benchmark(const LabContext& ctx);

// Throws std::runtime_error on SDL failure.
void run_visual(const LabContext& ctx);

} // namespace scr

#endif // SCR_MODES_H
