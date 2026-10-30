# Video Recording

## Overview

The Windows desktop application can capture one selected RGB camera alongside tracker data. The camera stream is encoded as H.264 in an MP4 file and stored inside the recording session.

## Workflow

1. Open the `Video` tab on the left rail.
2. Press `Refresh Cameras` to enumerate Media Foundation video devices.
3. Select a camera row and wait until the status reports `Preview running.` Recording is blocked while the camera is still opening or after activation fails.
4. Start the normal tracker recording. If a camera preview is active, the application first prepares the H.264 writer in the background, then starts tracker and video capture together. The Video tab reports `Preparing video recording...` during this short preparation.
5. Stop recording. Tracker capture stops immediately, while MP4 finalization continues on the video encoder thread.

Camera selection and refresh are locked while recording so that the session's video source cannot change mid-take. A camera is never selected automatically.

While the Video tab shows `Finalizing video...`, starting another recording, changing cameras, loading or deleting a session, and exporting are temporarily blocked. The UI and viewport remain responsive. When finalization succeeds, `video.partial.mp4` is atomically renamed to `video.mp4`; automatic export then starts if enabled.

Selecting `Disable` updates the list selection immediately and releases the active camera in the background. If another camera is selected before that release finishes, the latest selection is queued and opens automatically afterward.

## Session playback

When a session manifest contains a video entry, loading the session also opens that MP4. The Video tab preview follows the session timeline during play, pause, and seek. Decoded RGB32 frames use the Media Foundation stride sign to preserve their actual top-down or bottom-up orientation. Older sessions without video remain supported.

Session-video opening and decoding errors remain visible in the Video tab. The application does not silently replace a failed session video with the live camera preview.

Closing a loaded session first cancels any pending Media Foundation video read, then joins the playback worker and releases the session files. This keeps session close responsive even when a decode request is in progress.

Recorded MP4 sample times come from Media Foundation camera timestamps rather than a synthetic frame counter. This preserves real capture gaps and keeps video duration aligned with the tracker timeline; start and end can still differ by approximately one camera frame. Playback clamps video seeks to the MP4's last playable frame and treats end-of-stream as normal completion, so reaching the session timeline end does not produce an invalid-position error.

## Export

- `File > Export Video` copies the loaded session video to the configured export location with a collision-safe file name.
- When `Export after recording` is enabled, a session video is exported together with the regular GLB and skeleton outputs.

## Runtime and limitations

- Windows Media Foundation is required.
- Recording uses an available system H.264 encoder. The application reports a visible error if camera negotiation or encoder creation fails.
- The first version records camera video without audio.
- Hardware transforms are requested, but the actual encoder selected by Windows depends on installed hardware and codecs.
- Run only one application instance when using a camera. A second instance can keep the device busy and prevent activation.
- Camera frame rows are normalized from the negotiated signed stride into top-down BGRA before preview and encoding. This avoids device-specific padding and orientation assumptions.
- If MP4 finalization fails, the incomplete `video.partial.mp4` is retained for diagnostics or possible recovery. When tracker data was saved successfully, the manifest is rewritten as a tracker-only session so the failed video cannot make the session unloadable.
