# Editor appearance

The desktop application uses a Blender-inspired dark editor theme: neutral gray
panels, subtle borders, compact controls and blue selected states. Red is reserved
for recording. Device, session, streaming, video and right-side tabs use complete
rotated words instead of stacked letters. Main editor labels and menus use the
Windows Tahoma font; diagnostic values use Consolas.

The timeline stays visible when there is enough room, including before loading a
session. Its toolbar contains first frame, play/pause, last frame and Close controls.
Transport is disabled until a session or GLB scene is loaded.
An imported GLB has a separate filename bar above the playback toolbar,
with its own Close button. When a session is also loaded, the toolbar Close closes
only the session; the upper GLB Close closes only the imported scene. The filename
is shortened with an ellipsis when necessary to keep its button clear. The ruler shows frame
numbers in a dark header with spacing adapted to the clip length and available
width. The lighter track area has broad vertical divisions and no horizontal
grid or fine ticks. A rounded blue frame badge with a downward pointer and blue
line indicates the current position; the empty timeline shows frame 0 without
overlaying instructional text on the track. Clicking or dragging the ruler seeks
using the existing session/GLB playback path, including linked session video.

When both a session and an animated GLB are open, they share one playback clock.
Play/pause, first/last frame and timeline dragging control both. The timeline range
is the longer duration in seconds, and its frame labels use that source's existing
frame convention (recorded session frames or the GLB's 60 fps display grid).
Each source retains its original speed: at shared time 2 seconds, both sample time
2 seconds. A shorter source holds its final frame until the longer one finishes;
neither loops or stretches to fill the range. Linked session video follows the
clamped session time. Loading a second source joins the current time and playback
state. Closing either source preserves the surviving source's time, clamped to its
duration; closing both clears playback. Export timing remains source specific.

Device Labels, Smooth, camera Lock and model visibility retain their existing
behavior. Active display toggles use blue; recording keeps its red indicator.

The viewport uses a neutral gray background (default RGB 61/61/61) with a subtle
vertical gradient, darker at the top. The grid defaults to RGB 71/71/71. Long muted
red and green lines cross the floor origin in both directions, replacing the short
three-axis origin marker. In the recorder's Y-up space these floor directions are
X and Z; tracker poses, device orientation markers and export coordinates retain
their existing conventions. The background is one untextured quad per rendered
pane and retains demand-driven rendering.

The development Debug configuration's saved background and grid colors were
updated for this appearance request. Other saved colors and grid dimensions are
retained. Other installations retain their saved colors; Settings > Appearance
can change the background and grid individually, or reset all colors to defaults.

The common UI palette and standard button surface are defined in
`src/platform/win32/UiTheme.h`. Windows title-bar styling is isolated in
`UiWindowFrame.h`; unsupported OS decoration features produce diagnostic output.
Native Windows file pickers and system dialogs retain their OS styling.
