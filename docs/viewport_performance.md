# Viewport frame scheduling

Use the optimized Windows executable in `build/release/toyxyz_openvr_toolkit.exe`
for optimized recording and playback. Development testing uses
`build/vs2022/Debug/toyxyz_openvr_toolkit.exe` (`cmake --build --preset vs2022`).
`build/default` is a separate Ninja Debug build. Debug builds disable optimization
and enable additional runtime checks.

From an x64 Visual Studio developer environment with CMake and Ninja available:

```text
cmake --preset release
cmake --build --preset release --target OpenVRTrackerRecorderDesktop
ctest --preset release --output-on-failure
```

Camera orbit, pan and zoom handlers only update camera state. The normal frame
loop presents the newest camera state at its 90 Hz target, rather than sampling,
solving and rendering once per mouse event. Message dispatch yields when a frame
is due or after a 2 ms input-processing budget, so continuous input cannot starve
scheduled frames. Slow frames do not accumulate a catch-up backlog.

Session timeline dragging also updates only the requested position. File sampling,
mapping/IK and rendering occur in the next application update, so multiple mouse
messages can be replaced by the newest seek. It no longer presents an unsolved pose
from inside the mouse handler. GLB seeks use the same scheduled display path.

The UI loop blocks in `MsgWaitForMultipleObjectsEx` until input or the next deadline;
the independent pose worker sleeps until its sampling deadline. Neither loop spins
with `Sleep(0)` during the final 2 ms. Recording still targets 90 Hz.

Viewport drawing is demand driven: new nonempty tracker snapshots, device arrival
or removal, animation time changes, camera input and explicit invalidation request
a frame. Recording overlays stay animated. A static or paused viewport can show
View FPS 0 because its existing image remains displayed. Minimized windows skip
OpenGL rendering while application updates and recording continue.

Mapping IK, skeleton capture and VMC output run once in the application frame
update. Rendering each camera pane consumes the same solved pose; four-view mode,
window repaints and navigation do not multiply those side effects.

The timeline repaints during playback and on explicit playback controls. A paused
session reuses its already loaded pose frame and does not repeatedly read it from
disk or trigger continuous timeline repaint. Timeline/status-only paints
skip unrelated panels, and main-window fonts are reused.

The recorder retains an independent pose-sampling worker with the same target rate.
The 90 Hz display target is a limit, not a guarantee: complex scenes, hardware,
driver synchronization and video decoding can still limit the actual frame rate.

## Performance investigation (2026-10-31)

Eight-second process-time and Windows PDH GPU-engine samples on this 32-logical-CPU
machine gave the following results. CPU values below use one logical CPU as 100%;
divide by 32 for the overall CPU percentage shown by Task Manager.

| State | CPU before | CPU after | GPU 3D before / after |
| --- | ---: | ---: | ---: |
| Empty visible window | 24.6–29.7% | 2.93–7.42% | 0.87% / 0% |
| Empty minimized window | 10.5–11.5% | 4.88% | 0.67% / 0% |

The before/after idle comparison supports reduced unnecessary CPU/GPU work. It does
not establish that all perceived system slowdown is solved. The after range includes
an instrumented run and the final build with capture disabled; the final visible and
minimized samples used 7.42% and 4.88% of one CPU (0.23% and 0.15% of the whole CPU).
These short samples varied with other machine activity. The empty scene did not
saturate this GPU. A before sample of a paused 11,823-frame session used 18.94% of one
CPU; a later interactive sample used 14.65%, but these different activities are not
a controlled comparison.

A captured real interaction run contained 2,030 timeline move events: handler p95
0.266 ms, max 0.582 ms. The first render returning from SwapBuffers for a new input
sequence had p95 6.639 ms and max 12.778 ms (242 presented input sequences; intermediate
positions are coalesced). Read-frame p95 was 0.042 ms. These are CPU-side timestamps
starting when a message handler runs: they exclude time waiting in the input queue,
GPU completion, compositor scheduling and monitor scanout. They are not an end-to-end
latency benchmark. One 59 ms render occurred more than 53 seconds after the last
timeline input; its cause is unconfirmed. MP4 playback, physical trackers and heavy
animated meshes were not controlled benchmarks in this investigation.

Raw measurements are in local `build/performance_*.json` and
`build/performance_trace_before.csv` / `build/performance_trace_after.csv`.

## Repeat measurements

Run `python tools/measure_viewport_process.py LABEL 8` while the desired toolkit state
is visible, idle, playing or minimized. This only reads CPU time and GPU counters;
Windows must allow querying the target process and PDH counters.

For optional UI-thread tracing, create `performance_capture.flag` beside the EXE
before launching. It writes `viewport_performance.csv` beside that EXE, with frame
update, rendering, GDI painting, session frame reads and timeline-input durations.
Use `python tools/summarize_viewport_capture.py PATH_TO_CSV` to summarize a copied
capture. The log is buffered, flushed once per second and stops after three minutes.
Remove the flag and
restart to disable tracing; no capture file is opened on a normal launch. Timing
capture itself adds overhead. Normal FPS counts SwapBuffers calls and does not
describe frame-time spikes or input latency by itself.
