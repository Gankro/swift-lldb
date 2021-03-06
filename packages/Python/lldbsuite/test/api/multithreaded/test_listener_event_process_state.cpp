
// LLDB C++ API Test: verify the event description as obtained by calling
// SBEvent::GetCStringFromEvent that is received by an
// SBListener object registered with a process with a breakpoint.

#include <atomic>
#include <iostream>
#include <string>
#include <thread>

#include "lldb-headers.h"

#include "common.h"

using namespace lldb;
using namespace std;

// listener thread control
extern atomic<bool> g_done;

multithreaded_queue<string> g_frame_functions;

extern SBListener g_listener;

void listener_func() {
  while (!g_done) {
    SBEvent event;
    bool got_event = g_listener.WaitForEvent(1, event);
    if (got_event) {
      if (!event.IsValid())
        throw Exception("event is not valid in listener thread");
        // send process description
        SBProcess process = SBProcess::GetProcessFromEvent(event);
        if (!process.IsValid())
            throw Exception("process is not valid");
        if (SBProcess::GetStateFromEvent(event) != lldb::eStateStopped || SBProcess::GetRestartedFromEvent(event))
            continue; // Only interested in "stopped" events.

        SBStream description;

        for (int i = 0; i < process.GetNumThreads(); ++i) {
            // send each thread description
            SBThread thread = process.GetThreadAtIndex(i);
            // send each frame function name
            uint32_t num_frames = thread.GetNumFrames();
            for(int j = 0; j < num_frames; ++j) {
                const char* function_name = thread.GetFrameAtIndex(j).GetSymbol().GetName();
                if (function_name)
                    g_frame_functions.push(string(function_name));
            }
        }
    }
  }
}

void check_listener(SBDebugger &dbg) {
  // check thread description
  bool got_description = false;
  string func_name = g_frame_functions.pop(5, got_description);
  
  if(got_description == false)
    throw Exception("Expected at least one frame function");
}
