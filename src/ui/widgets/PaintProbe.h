#pragma once

#include "engine/PerfMonitor.h"
#include <juce_gui_basics/juce_gui_basics.h>

/*  Debug-only paint timing for the performance monitor (LITHIUM_PERF=1, see
    engine/PerfMonitor.h).

    Put LITHIUM_PERF_PAINT ("Name"); first thing in a paint() method. It times
    the method (not the children JUCE paints after it) and counts the calls.
    With LITHIUM_PERF unset it costs one bool test; with CMake's
    LITHIUM_PERF_TOOLS=OFF it compiles to nothing.

    Paint only ever runs on the message thread, so nothing here needs locks.
*/

#ifndef LITHIUM_PERF_TOOLS
 #define LITHIUM_PERF_TOOLS 1
#endif

namespace lithium::paintprobe
{

/** Times one paint() call (use the macro below). */
class Scope
{
public:
    explicit Scope (const char* name) noexcept;
    ~Scope();

private:
    const char* name;          // a string literal, so the pointer itself is a fine map key
    juce::int64 start = 0;     // 0 = not timing (monitor off)
};

/** Call at the end of the top-level component's paintOverChildren: everything
    painted since the first timed paint after the previous call is one frame. */
void frameEnd();

/** The figures since the last call (the performance monitor asks once a second). */
PerfMonitor::UiSample takeSample();

} // namespace lithium::paintprobe

#if LITHIUM_PERF_TOOLS
 // JUCE_JOIN_MACRO pastes the line number on, so the variable's name is unique.
 #define LITHIUM_PERF_PAINT(name) const ::lithium::paintprobe::Scope JUCE_JOIN_MACRO (lithiumPaintProbe_, __LINE__) (name)
#else
 #define LITHIUM_PERF_PAINT(name)
#endif
