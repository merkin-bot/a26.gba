# TIA audio and GBA Direct Sound evidence

**Updated:** 2026-11-25 UTC  
**Status:** Non-silent mGBA output observed; TIA fidelity, gap-free playback, game-clock synchronization, and physical audibility are `UNVERIFIED`.

## Register setup correction

The first `A26_CAPTURE_AUDIO` run contained only zero-valued PCM. At the end of that run, mGBA reported `SOUNDCNT_H=0` and `SOUNDCNT_X=0`: the plugin had written `0x0B0C` to `SOUNDCNT_L` instead of routing Direct Sound FIFO A in `SOUNDCNT_H` and enabling the sound master in `SOUNDCNT_X`.

The current code disables DMA1/timer 0 before setup, enables the sound master (`SOUNDCNT_X=0x0080`), writes `SOUNDCNT_H=0x0B0C` (FIFO A route/volume and timer selection), configures DMA1 to FIFO A with control `0xB640`, and starts timer 0. The existing display pacing code later sets timer 0 to reload `0xFC00` (period 1,024 GBA cycles, approximately 16.4 kHz). The source buffer produces 272 samples on 14 of every 15 display callbacks and 288 on the fifteenth, averaging 273.07 samples per callback, close to the NTSC Atari frame rate at that timer setting.

This configuration change establishes only that the register path is no longer disabled. A 16-bit nonzero PCM capture is not evidence that the samples encode a faithful TIA waveform.

## mGBA capture

The runner supports streaming its audio buffer to a WAV file with `A26_CAPTURE_AUDIO=work/emulation-baseline/audio-stream.wav`. The capture used the shared exFAT Chis scenario, stopped after Atari TV counter 41, and used plugin SHA-256 `ab91d38db0ad10c6c93ebdf930018815e93559d088a8a8679eef729c103d4964`.

Observed WAV properties:

- 1,639,372 stereo sample frames at 32,768 Hz, 16-bit PCM; duration 50.0297 seconds.
- Peak absolute sample value 24,576; aggregate mean-square value 12,107,801 (runner diagnostic).
- One-second blocks through approximately second 37 were silent; nonzero PCM appeared around seconds 38–49. This is not gap-free audio.
- Final mGBA state: sound master enabled, `SOUNDCNT_H=0x030C`, timer 0 reload `0xFC00`, timer control `0x0080`, DMA1 control `0xB640`. Its next DMA source address remained inside the allocated EWRAM buffers in this run.
- Atari stop-boundary trace: TV counter 41, `clk=799816`, about 0.67 seconds of Atari color-clock time. mGBA's WAV timeline covered about 50 seconds.

The large timeline mismatch and long silent prefix show that this run does not have synchronized or continuous sound. The plugin's display/audio refill callbacks advance only when Atari emulation reaches display boundaries; while the emulator runs far below real time, GBA timer/DMA continue on the GBA timeline between those callbacks. The current ping-pong buffers therefore do not solve underrun or clock drift. Correctness needs a redesigned producer/consumer schedule tied to emulated Atari time, with repeatable before/after video and audio measurements.

## Limits and next work

- The capture is from mGBA's software GBA model and does not prove physical GBA output, card behavior, audible speaker/headphone output, or hardware timing.
- The current TIA generator samples tone state at buffer-fill time; it does not timestamp individual TIA register writes against the Atari color clock. Sound mode/LFSR fidelity and anti-aliasing are not established.
- The PCM's long silence and later nonzero section make the overall capture a failed continuity check despite its nonzero peak.
- Next, measure callback cadence and FIFO consumption against Atari color clocks, then redesign the audio buffer/refill path to preserve sample timing when rendered frames are skipped or delayed. Validate deterministic waveforms and synchronized game-state captures before claiming audio acceptance.

## Reproduction

Build the plugin and runner using `WORKSPACE.md`, create the shared image, and run the Chis profile with the environment expected by `scripts/verify-six-profile-image.sh`. Set `A26_CAPTURE_AUDIO=work/audio.wav`; use `A26_TRACE_ATARI_GLOBALS=1` and `A26_STOP_ATARI_TV_COUNTER=41` for the stop-boundary trace. Keep WAVs and other generated captures under ignored `work/`.
