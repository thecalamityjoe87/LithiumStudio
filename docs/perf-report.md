# Lithium Studio: performance and stability report

October 2026, milestone 12 code; the fixes in section 6 are milestone 13. Measured on the development laptop: Intel Core Ultra 7 155U (2 P-cores at
4.8 GHz with hyper-threading = CPUs 0-3, 8 E-cores at 3.8 GHz = CPUs 4-11, 2 low-power E-cores at 2.1 GHz =
CPUs 12-13), 14 GB RAM, Linux 7.3, PipeWire 1.6.8 through its ALSA plug-in, RealtimeKit, on AC power with the
"balanced" platform profile.

## Summary

**The crash at ~90% is fixed.** Once the audio thread ran for 200 ms without a break, the kernel killed the app
(SIGKILL; no core dump and nothing in the journal). The SIGXCPU handler meant to prevent this tried to drop the
thread's real-time priority in a way the kernel refuses (EPERM, because RealtimeKit had set
`SCHED_RESET_ON_FORK`). One flag fixes it. The app now survives sustained 200% overload with dropouts only, and
gets real-time priority back once the load drops (section 1).

**What the meter measures.** JUCE's figure, passed through by Tracktion: time spent in the whole audio callback
÷ buffer length, smoothed over ~5 callbacks and shown ~3 times a second. It's an average, so spikes are
invisible; and once it passes 98% Tracktion mutes every other block, which pins it at 90-100% however
overloaded the session really is (section 2).

**What drives the 50-70%** (realistic 16-track song, 256 samples; section 3):

1. **Which core the audio thread lands on.** The same song costs 30% on a P-core, 51% on an E-core and 93% (with
   thousands of xruns) on one of the two low-power cores; Linux picks one at random. That's the spread between
   "fine" and "50-70% with dropouts" on this laptop, and the cause of most dropout bursts found.
2. **Tracktion's 4OSC synth**: 7.7-12% of the buffer per synth track with the bundled patches (unison stacks,
   long releases, a reverb inside each instance). Three such tracks are about a third of the budget; sampled
   instruments cost 1-4.5% each, effects mostly under 1% (the guitar pedals 2-6%, Pitch Shift 9%).
3. **Silence costs 14%.** Instruments and effects keep processing with the transport stopped.
4. **Everything renders on one thread**, so one core is the ceiling (~16 synth or ~32 sampled tracks).

Plug-ins account for ~90% of the callback; the rest is Tracktion's mixing, file reading and metering.

**Other headline problems:** a use-after-free on every quit (AddressSanitizer) and one in the SFZ loader when a big
library is still loading (seen once in the logs); the SFZ loader thread races sfizz's globals with the audio
thread (ThreadSanitizer); the audio thread allocates, takes ~160 locks and sometimes makes a system call every block, all of
it inside Tracktion/JUCE (Lithium Studio's own audio code is clean); seeking and tempo changes during playback cause
real dropouts; the region drawing under the moving playhead is half the UI's paint time. Track count scales
linearly; sends, buses, automation (for audio), autosave, recording and UI activity do not disturb playback.

**Against the targets** (256 samples, 48 kHz):

| Target | Result |
|---|---|
| Realistic 16 tracks: avg < 40%, peaks < 70%, no xruns | Met on a P-core (31% / p99 46%); not on an E-core (51% / max 94%) or a low-power core (93%, 2948 xruns). Quick wins 1-3 in section 6 should meet it everywhere. |
| 32 tracks: peaks < 90%, no xruns | Not met with one render thread (mix: 56-60% avg, peaks 104-112%, 1-4 xruns); met with 4 threads (26% avg, 55% max, 0 xruns). |
| Stopped: audio load near zero | 2% for an empty session; **14%** for the realistic song. Not met (finding C3). |
| Stopped: message thread near idle | Met: 1.6% of a core. |
| UI cost modest, no audio spikes from UI | Met: 8-16% of a core, no correlation with callback times. |
| No crashes or sanitizer errors; no growth over an hour | One hour: no crash, no handle/thread growth, no slowdown; memory +23 MB/hour after warm-up (C6). Sanitizers: one use-after-free at quit (A6) and sfizz races from the SFZ loader (A5); none during playback in Lithium Studio's own code. |

## 1. The crash at ~90% CPU

### What happens

The app is killed with **SIGKILL** by the kernel. There is no core dump, no backtrace and no message in the
journal: the process just disappears ("Killed" on a terminal). That is why `journalctl` showed nothing for these
crashes. `coredumpctl` isn't installed and the core size limit is 0 anyway, but a SIGKILL never leaves a core.

There is no meaningful backtrace to show: SIGKILL can't be caught, and the thread that was "at fault" was simply
doing its normal work (rendering audio) when the kernel's time ran out. The relevant "backtrace" is the failed
system call in the signal handler, which the experiment below shows directly.

### Reproduction

1. The release binary built this morning before any change (`build-release`, 09:44), run headless with its audio
   going to a silent PipeWire sink. Device: ALSA "default" (PipeWire's ALSA plug-in), 512 samples at 44.1 kHz,
   the settings the AppImage uses. The audio thread is made real-time through RealtimeKit (SCHED_RR 20) and
   `/proc/<pid>/limits` shows `Max realtime timeout 100000 / 200000 us`.
2. Extra busy work added on the audio thread (an `LD_PRELOAD` that spins before each `snd_pcm_writei`), stepped
   up every 6 seconds:

   | Extra load (share of the buffer period) | 50% | 80% | 95% | 105% |
   |---|---|---|---|---|
   | App | alive | alive | alive | **killed within 6 s** |

   It reproduces every time. Any work that keeps the audio thread busy without a break for 200 ms does it:
   real DSP load does too (see "Real overload" below).

Why the meter says "about 90%" rather than 100%: the toolbar shows JUCE's smoothed load (section 2), sampled
about 3 times a second, and once that smoothed figure passes 98% Tracktion mutes every other block (it skips
rendering), so the reading bounces between roughly 90% and 120% while the real work per rendered block is far
above 100%. Whichever value the toolbar happens to sample is what the user sees.

### Root cause

`src/platform/AudioHardware.cpp` already knew about this limit. When the audio thread is made real-time, the
code sets `RLIMIT_RTTIME` to 100 ms soft / 200 ms hard and installs a SIGXCPU handler: at the soft limit the
kernel sends SIGXCPU and the handler puts the thread back to ordinary priority, which stops the count before the
hard limit's SIGKILL.

The handler did:

```cpp
sched_setscheduler (tid, SCHED_OTHER, &ordinary);
```

RealtimeKit makes threads real-time with `SCHED_RR | SCHED_RESET_ON_FORK` (and so does `makeThreadRealtime`'s
own direct path), and **only a privileged process may clear `SCHED_RESET_ON_FORK`**. So the call fails with
`EPERM`, the thread stays real-time, and 100 ms later the kernel's hard limit kills the whole process.

A standalone test program (rtkit-promoted thread, same limits, same handler) shows it:

```
burning 0.50s, policy=SCHED_RR|SCHED_RESET_ON_FORK
t=0.114 handler on tid 182997: sched_setscheduler(SCHED_OTHER) = -1, errno=1 (EPERM), policy unchanged
Killed  (exit status 137)
```

and with the flag kept (`SCHED_OTHER | SCHED_RESET_ON_FORK`):

```
t=0.101 handler: result=0, policy now SCHED_OTHER|SCHED_RESET_ON_FORK
survived a 2 s busy loop, exit normally
```

Things that were checked and ruled out: PipeWire's client RT module does not change the limits in-process
(soft 200 / hard 200 ms before the app sets its own 100 / 200 ms); RealtimeKit's watchdog didn't demote anything
(no "canary" messages in the journal); the PipeWire server did not drop the client.

### Fix (applied, not committed)

* `src/platform/AudioHardware.cpp`, `onCpuLimit`: demote with `SCHED_OTHER | SCHED_RESET_ON_FORK`. One flag;
  it's what the kernel requires, and the thread being demoted is ours.
* `src/engine/RealtimeAudioThread.cpp`, `timerCallback`: before, a demoted thread stayed at ordinary priority for
  the rest of the session. Now, once the CPU meter has stayed under 70% for 5 seconds, it asks for real-time
  again; the wait doubles after each re-promotion (up to 2 minutes), so a load the meter can't see can't cause
  a 100 ms stall every few seconds.

### Verified: sustained overload is now survivable

* Same synthetic test on the fixed build: 50 → 95 → 105 → 150 → 200 → 200% for 6 s each, then 30%. The app
  stays up throughout; the audio thread is demoted at the first overload and becomes real-time again once the
  load drops.
* Real overload (no synthetic load): a Debug build playing a generated 24-track mix at 128 samples averaged
  313% of the buffer (peaks 1069%). The audio thread was demoted (journal: no RealtimeKit action, so it was our
  SIGXCPU handler: the old code would have been killed here) and the app ran to the end and quit normally.
* Optimized build, realistic 16-track song at 96 kHz / 256 samples (average 92%, peaks 227%): demoted, no crash.
* Optimized build, synthetic load ramped 0 → 200% → 0 over a minute on top of the realistic song, at 256 and at
  64 samples: thousands of over-budget callbacks, no crash; real-time priority back ~5 s after the load fell
  (section 3.7). In all the runs for this report, many of them overloaded for minutes, the app never crashed.

### Other crashes found in the logs (not load-related)

* **SIGSEGV in the "SFZ loader" thread** (journal, Oct 23 06:02): `segfault at 448 ... error 6` on an atomic
  store, 18 s after JUCE logged `!! killing thread by force !!`. `SfzPlugin::Loader` (`src/engine/SfzPlugin.cpp`)
  loads an SFZ synchronously inside `run()` (sfizz can't be interrupted) and `~Loader` calls `stopThread (10000)`.
  If the plug-in is deleted while a big library is still loading (sound changed, track deleted, session closed)
  and loading takes over 10 s, JUCE force-kills the thread with `pthread_cancel`. That cancel is deferred, so the
  thread keeps running until its next cancellation point, while `~Loader` returns and the `SfzPlugin` is freed;
  the thread then writes `owner.numRegions` / `owner.incoming` into freed memory. Proposed fix in section 5.
* **`terminate called after throwing std::system_error ... write: Broken pipe`** at quit (3 times on Oct 23;
  also "read: End of file", "read: Connection reset by peer" earlier). These are asio error messages. The only
  binaries here containing asio are the third-party TONE3000 plug-ins (`~/.lv2/TONE3000.lv2`,
  `~/.vst3/TONE3000.vst3`); each crash is preceded by a JUCE "run() method mustn't throw" assertion from a JUCE
  thread, i.e. the plug-in's own JUCE copy. A third-party bug triggered at shutdown; Lithium Studio can only protect
  itself by hosting plug-ins out of process (section 5).

## 2. What the in-app CPU meter measures

* The toolbar's "CPU nn%" comes from `AudioEngine::getSystemStats` → `te::DeviceManager::getCpuUsage()` →
  JUCE's `AudioDeviceManager::getCpuUsage()`. Tracktion doesn't measure anything itself; it passes JUCE's value
  through (and reads it once per block to decide when to mute, see below).
* JUCE times the **whole** device callback: every registered `AudioIODeviceCallback` (Tracktion's render, and
  Lithium Studio's tuner, live waveform, recording guard, preview player and real-time helper), from the first to the
  last, as one span (`AudioProcessLoadMeasurer::ScopedTimer` in `AudioDeviceManager::audioDeviceIOCallbackInt`).
  Since Lithium Studio renders on one thread there's no waiting on workers; with more render threads it would include
  waiting for them. It does **not** include the time the ALSA/PipeWire plug-in spends inside `snd_pcm_writei`
  or `snd_pcm_readi`, nor scheduling delay before the callback starts.
* Value = callback time ÷ buffer duration, **smoothed** with an exponential moving average: each callback moves
  the value 20% of the way to the new measurement (`filterAmount = 0.2`). At 256 samples / 48 kHz that's a time
  constant of ~5 callbacks ≈ 27 ms, so it is an **average**, not a peak: a single callback at 150% between ones
  at 40% shows as ~62% for one callback, then fades.
* The UI polls it at 30 Hz but only redraws the number ~3 times a second (`Toolbar::update`), so most spikes are
  never shown at all.
* JUCE also counts callbacks that took longer than the buffer (`getXRunCount`, added to the device's own count);
  the app doesn't show that.
* Tracktion uses the same smoothed value as a safety valve (`cpuLimitBeforeMuting = 0.98`): above 98% it
  outputs silence for that block instead of rendering, then decays the value by 1% per muted block (floored at
  90%). Under heavy overload this alternates rendered and muted blocks — heard as crackle/stutter — and keeps the
  meter hovering around 90-100% however bad the real load is.

Defaults: `AudioEngine::Impl` opens the ALSA "default" device with 2 inputs and 2 outputs and JUCE's defaults for
that device (44.1 kHz, 512 samples on PipeWire). On the very first run `chooseSmallBufferOnce` picks the smallest
size of at least 5 ms (224 samples at 44.1 kHz). The user can change the device, sample rate and buffer size in
Preferences › Audio (JUCE's `AudioDeviceSelectorComponent`); Tracktion remembers the choice. The AppImage on
this machine runs at 512 / 44.1 kHz (from its saved settings).

## 3. Measurements

All runs: optimized build (`build-perf`, `-O3`), headless, 48 kHz unless noted, PipeWire through ALSA. "Callback %"
is the time from the start to the end of each audio callback as a share of the buffer's length (PerfMonitor),
over the whole measured run: avg, p99 (1 callback in 100 is slower) and max. "Over budget" = callbacks longer
than the buffer; "device xruns" = JUCE's count (over-budget callbacks plus what the device reports). "Meter" is
what the toolbar would have shown on average. "UI thread" = the message thread's CPU, % of one core. "RT" = the
audio thread stayed real-time throughout (**no** = the overload safety net of section 1 kicked in, and the app
survived). Each run measures one pass of the 16-bar loop (32 s) after a 6 s warm-up, except where noted.

**The songs.** `realistic` (16 tracks) is the kind of session the meter reads 50-70% on: 2 Drummer tracks, 2 sampled
electric basses, 2 sampled CP80 electric grands, 2 "Warm Pad" synths, "Chamber Strings" (synth), "Bright Lead"
(synth), a sampled electric piano, a guitar and a bass guitar through the Amp, a vocal through Tune + Compressor
+ EQ, and two audio loops (one through Compressor + Echo); 12 tracks send to the Reverb bus and 6 to the Echo
bus. Instrument tracks play a held chord plus an eighth-note arpeggio every bar; audio tracks loop library
loops. `audio`, `synth`, `sampled` are N tracks of one kind, `mix` is N tracks cycling through the realistic
song's 12 kinds.

### 3.1 Scenario × buffer size

These runs were made before the per-callback core tracking existed, and section 3.2 shows that the core the
audio thread happens to run on changes the load by up to 3×. Read single rows with that in mind (e.g. "mix 32" at
512 samples doing worse than at 256 is placement, not buffer size); section 3.3 repeats the scaling runs with
the app kept on one kind of core.

| Run | Tracks | Rate / buffer | Callback avg % | p99 % | max % | Over budget | Device xruns | Meter avg % | UI thread % | Paint ms/s | Memory MB (start→end) | RT |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| realistic | 16 | 48k / 64 | 57.6 | 84.0 | 158.5 | 68 | 80 | 54.2 | 8.8 | 62.5 | 607→628 | yes |
| realistic | 16 | 48k / 128 | 40.2 | 74.5 | 125.1 | 11 | 11 | 39.6 | 8.7 | 62.9 | 617→636 | yes |
| realistic | 16 | 48k / 256 | 50.6 | 72.5 | 96.3 | 0 | 0 | 49.2 | 8.4 | 59.9 | 607→627 | yes |
| realistic | 16 | 48k / 512 | 49.2 | 66.0 | 75.3 | 0 | 0 | 48.1 | 8.2 | 58.2 | 611→630 | yes |
| realistic | 16 | 96k / 256 | 70.4 | 147.0 | 334.9 | 1998 | 2105 | 75.2 | 7.9 | 55.0 | 670→688 | **no** |
| audio 8 | 8 | 48k / 256 | 3.0 | 4.5 | 7.6 | 0 | 0 | 3.0 | 7.3 | 54.3 | 198→216 | yes |
| synth 8 | 8 | 48k / 256 | 48.1 | 63.0 | 97.8 | 0 | 0 | 44.0 | 8.2 | 64.1 | 124→124 | yes |
| sampled 8 | 8 | 48k / 256 | 18.1 | 29.5 | 48.7 | 0 | 0 | 16.5 | 8.3 | 64.7 | 464→470 | yes |
| mix 8 | 8 | 48k / 256 | 28.9 | 39.0 | 68.8 | 0 | 0 | 26.7 | 7.6 | 57.3 | 322→338 | yes |
| audio 16 | 16 | 48k / 256 | 4.9 | 7.0 | 15.4 | 0 | 0 | 4.9 | 6.8 | 48.7 | 210→243 | yes |
| synth 16 | 16 | 48k / 256 | 91.5 | 130.5 | 154.4 | 2117 | 2126 | 89.1 | 8.2 | 60.9 | 153→153 | yes |
| sampled 16 | 16 | 48k / 256 | 41.0 | 63.0 | 80.9 | 0 | 0 | 34.7 | 9.2 | 69.2 | 642→646 | yes |
| mix 16 | 16 | 48k / 256 | 28.5 | 39.0 | 62.2 | 0 | 0 | 26.9 | 8.7 | 62.5 | 635→665 | yes |
| audio 32 | 32 | 48k / 256 | 5.8 | 11.0 | 14.2 | 0 | 0 | 5.9 | 6.9 | 47.2 | 236→302 | yes |
| synth 32 | 32 | 48k / 256 | 94.9 | 158.0 | 250.3 | 4372 | 4416 | 91.6 | 7.4 | 51.5 | 210→211 | **no** |
| sampled 32 | 32 | 48k / 256 | 74.0 | 120.0 | 165.2 | 607 | 644 | 70.7 | 8.9 | 63.7 | 1130→1151 | yes |
| mix 32 | 32 | 48k / 256 | 61.9 | 133.5 | 161.7 | 339 | 340 | 55.3 | 6.8 | 44.2 | 870→924 | **no** |
| audio 64 | 64 | 48k / 256 | 9.1 | 14.5 | 22.9 | 0 | 0 | 9.3 | 6.6 | 41.6 | 257→390 | yes |
| synth 64 | 64 | 48k / 256 | 125.4 | 335.0 | 504.7 | 2312 | 2331 | 99.9 | 7.6 | 47.5 | 326→326 | **no** |
| sampled 64 | 64 | 48k / 256 | 92.2 | 181.0 | 304.4 | 2646 | 2732 | 90.7 | 8.8 | 57.3 | 2022→1965 | **no** |
| mix 64 | 64 | 48k / 256 | 96.3 | 182.0 | 326.7 | 4504 | 4630 | 92.3 | 8.5 | 55.2 | 1570→1628 | **no** |
| mix 32 | 32 | 48k / 64 | 69.3 | 178.5 | 368.9 | 1129 | 1151 | 69.0 | 8.1 | 55.5 | 912→954 | **no** |
| mix 32 | 32 | 48k / 128 | 63.4 | 138.5 | 248.7 | 476 | 480 | 60.0 | 7.5 | 51.3 | 878→942 | **no** |
| mix 32 | 32 | 48k / 256 | 59.6 | 84.5 | 104.3 | 4 | 5 | 58.5 | 8.0 | 56.0 | 857→891 | yes |
| mix 32 | 32 | 48k / 512 | 59.2 | 96.0 | 142.1 | 24 | 26 | 58.6 | 6.8 | 45.2 | 890→922 | **no** |
| mix 32 | 32 | 96k / 256 | 97.7 | 206.5 | 398.6 | 8399 | 8516 | 90.8 | 7.7 | 53.4 | 983→1027 | **no** |
| mix 64 | 64 | 48k / 64 | 110.8 | 267.5 | 970.6 | 12731 | 12806 | 97.4 | 7.8 | 47.6 | 1544→1597 | **no** |
| mix 64 | 64 | 48k / 128 | 102.5 | 203.5 | 402.7 | 7878 | 7936 | 95.4 | 8.8 | 57.9 | 1536→1584 | **no** |
| mix 64 | 64 | 48k / 256 | 97.5 | 168.0 | 369.9 | 4460 | 4490 | 93.4 | 9.3 | 62.0 | 1607→1653 | **no** |
| mix 64 | 64 | 48k / 512 | 94.9 | 155.5 | 216.8 | 2330 | 2354 | 92.4 | 9.1 | 60.6 | 1592→1658 | **no** |
| mix 64 | 64 | 96k / 256 | 128.7 | 323.5 | 554.5 | 4432 | 4472 | 99.9 | 7.2 | 41.5 | 1618→1649 | **no** |

### 3.2 Which core the audio thread runs on (the biggest single factor)

This laptop's CPU has three kinds of cores. Linux places real-time threads without regard to core speed, so
Lithium Studio's audio thread lands wherever, and stays there for seconds to minutes. Same song, same settings:

| Realistic song, 256 samples | Callback avg % | p99 % | max % | Over budget (32 s) |
|---|---|---|---|---|
| App kept on P-cores (CPUs 0-3, 4.8 GHz) | 30.9 | 46.0 | 139.5 | 1 |
| App kept on E-cores (CPUs 4-11, 3.8 GHz) | 50.6 | 71.0 | 93.5 | 0 |
| App kept on low-power E-cores (CPUs 12-13, 2.1 GHz) | **93.3** | **135.0** | **290.2** | **2948** |
| Not pinned (this run: 94% of callbacks on CPU 1, a P-core) | 29.4 | – | – | 0 |

The matrix's realistic rows (≈50%) are E-core numbers; the user's 50-70% readings fit E-cores, with the higher end
when the low-power cores are involved. Every burst of xruns found in the feature runs that wasn't explained by
the feature itself happened while the thread was on CPU 12 or 13: the "autosave during playback" run's 413
over-budget callbacks were all in the 5 seconds the thread spent on CPUs 12-13 (93% average there, 29% before
and after on a P-core, with autosave running throughout).

### 3.3 Track scaling (app kept on the E-cores, so placement can't skew it)

| Tracks | audio avg / max % | synth (4OSC) avg / max % | sampled avg / max % | mix avg / max % |
|---|---|---|---|---|
| 8 | 3.5 / 8 | 48.8 / 86 | 17.7 / 45 | 29.1 / 60 |
| 16 | 5.0 / 10 | 91.1 / 140 (2049 over budget) | 40.4 / 81 | 49.2 / 88 |
| 32 | 8.9 / 16 | 111.7 / 278 (overloaded) | 78.0 / 226 (440) | 92.5 / 175 (2584) |
| 64 | 11.1 / 27 | 218.7 / 586 (overloaded) | 114.6 / 342 (overloaded) | 123.3 / 337 (overloaded) |

Scaling is linear, nothing worse: ~5.7% per synth track, ~2.4% per sampled track, ~2.9% per mixed track,
~0.15% per audio track (on top of a ~2% base) until the single render thread runs out. On an E-core that's
about 16 mixed tracks at 256 samples; a P-core gives ~1.7× more.

### 3.4 Tracktion's render threads

(Since changed: 2 threads, the worker real-time; section 6.) Lithium Studio rendered everything on the audio thread (`LithiumEngineBehaviour::getNumberOfCPUsToUseForAudio() = 1`; see
the 2026-10-16 change). With `LITHIUM_PERF_AUDIO_THREADS`, 32-track mix:

| Render threads | 64 samples: avg / p99 / max %, over budget | 256 samples: avg / p99 / max %, over budget | Extra CPU on worker threads |
|---|---|---|---|
| 1 (now) | 66.6 / 124 / 337, **487** | 56.3 / 75 / 112, 1 | – |
| 2 | 50.2 / 86 / 225, 87 (E-core) | 30.6 / 52 / 93, 0 (P-core) | 36-51% of a core |
| 4 | 32.3 / 51 / 113, **2** | 25.8 / 39 / 55, 0 (E-core) | 66-83% of a core |

So the work does spread across threads (Tracktion's graph is per-track parallel; the master/bus nodes are the
only serial part). The price is CPU: Tracktion's workers spin-wait between blocks. They also run at ordinary
priority, which is what caused the October dropouts on a busy desktop; that is solvable (section 6).

### 3.5 Features

| Run (256 samples) | Tracks | Callback avg / p99 / max % | Over budget | UI thread % | Notes |
|---|---|---|---|---|---|
| Drummer × 8 | 8 | 11.5 / 18 / 24 | 0 | 7.5 | ~1.2% per Drummer track |
| Tune × 8 (vocal loops) | 8 | 13.2 / **27 / 45** | 0 | 7.5 | cheap on average, spiky (block-wise pitch analysis) |
| Heavy chains × 8 (Comp, EQ, Reverb, Echo, Chorus, +Tune on 4) | 8 | 14.7 / 23 / 42 | 0 | 6.9 | |
| Mix 8, no sends | 8 | 28.9 / 39 / 69 | 0 | 7.6 | (from 3.1) |
| Mix 8 + both sends on every track | 8 | 29.5 / 39 / 75 | 0 | 7.9 | sends and buses cost almost nothing |
| Mix 8 + dense automation (6 settings per track, a point every 16th) | 8 | 33.0 / 43 / 54 | 0 | **25.8** | the audio cost is small, the **message thread triples** |
| Realistic + tempo changes every 2 bars + a 3/4 section | 16 | 48.7 / 73 / 90 | 0 | 8.7 | the tempo map itself is free |
| TONE3000 (LV2) × 4 | 4 | 3.3 / 7 / 8 | 0 | 6.7 | no model loaded, so close to a pass-through; no heavy third-party plug-in was available |
| Empty session, stopped | 1 | 2.0 / 3.5 / 4 | 0 | 1.6 | the baseline |
| **Realistic, stopped** | 16 | **13.9** / 19 / 25 | 0 | 1.6 | see 4.C3: instruments and effects process silence |

Editing during playback (realistic song, 256 samples; LP-E = over-budget callbacks in seconds when the thread was
on a low-power core):

| Action, while playing | avg / p99 / max % | Over budget | of which on LP-E | UI thread % | Memory MB |
|---|---|---|---|---|---|
| Play/stop every 0.5 s | 35 / 64 / **157** | 1 | 0 | 5.8 | 605→618 |
| Loop on/off every second | 30 / 41 / 61 | 0 | 0 | 8.5 | |
| **Seek** 4 times a second | **75** / 102 / 131 | **51** | 0 | 7.1 | |
| Add + remove a track every 2 s | 46 / 73 / 121 | 1 | 0 | 9.2 | **596→673** |
| Add + remove a Reverb every second | 50 / 68 / 91 | 0 | 0 | 9.1 | |
| Undo/redo 10 steps each way, 10 per second | 46 / 66 / 105 | 1 | 0 | 11.1 | |
| Autosave every 2 s | 50 / 130 / 190 | 413 | **412** | 7.9 | |
| **Tempo change** every second | 58 / 137 / **460** | **88** | 0 | 11.5 | |
| Recording an audio track (silent input) | 51 / 73 / 89 | 0 | 0 | 10.5 | |
| Recording with monitoring through the vocal chain | 51 / 75 / 113 | 1 | 0 | 11.2 | |

### 3.6 UI under load

Realistic song at 256 samples unless noted. Paint = time inside the instrumented `paint()` methods.

| UI activity while playing | Callback avg / max % | Over budget | UI thread % | Paint ms/s | Frames/s | Longest frame |
|---|---|---|---|---|---|---|
| Nothing (P-core) | 28.9 / 61 | 0 | 8.8 | 64 | 31 | 6 ms |
| Scrolling the timeline 20×/s (E-core) | 51.5 / 95 | 0 | 12.6 | 89 | 41 | 7 ms |
| Zooming in/out 2×/s (E-core) | 51.8 / 94 | 0 | 8.1 | 54 | 33 | 8 ms |
| Mixer open, all meters (E-core) | 50.0 / 88 | 0 | 9.5 | 70 | 31 | 6 ms |
| Resizing the window 2×/s (E-core) | 50.7 / 83 | 0 | 11.4 | 73 | 33 | 19 ms |
| Light/dark every second (E-core) | 50.6 / 94 | 0 | 9.6 | 67 | 31 | 18 ms |
| Library sidebar on/off every second (P-core) | 30.2 / 55 | 0 | 8.6 | 58 | 32 | 18 ms |
| Mixer + scrolling, 8 / 16 / 32 / 64 tracks | – | – | 12.9 / 13.2 / 14.7 / 15.6 | 99 / 98 / 104 / 106 | 41-42 | 8-10 ms |

UI activity never lined up with audio spikes: each run's callback figures match the plain run on the same kind of
core, the RT thread pre-empts the UI, and `rtcheck` counted the same locks per callback with and
without UI activity (section 4.A4). UI cost grows only slightly with track count. The top painter during playback is
`RegionComponent` (~170 paints/s, half of all paint time; see 4.D1). The message thread is 1.6% busy when
nothing is playing.

### 3.7 Overload and recovery

Realistic song plus synthetic load ramped from 0 to 200% of the buffer over 30 s and back down over 30 s:

| Buffer | Callback avg / p99 / max % | Over budget | Result |
|---|---|---|---|
| 256 | 112 / 223 / 234 | 4717 | survived; RT dropped at ~70% extra load, back ~5 s after the load fell below 70% |
| 64 | 117 / 230 / 353 | 20242 | survived; RT dropped, not yet back when the run ended (load was still ~50-90%) |

### 3.8 Endurance: one hour of playback

The realistic song looped at 256 samples for 60 minutes (674,897 callbacks). Two other programs ran during part
of it: the AddressSanitizer and ThreadSanitizer builds of this report (at `nice 19`) overlapped roughly minutes
6-26. Five-minute windows:

| Minutes | Callback avg % | worst p99 % | max % | Over budget | Slow-core callbacks % | Memory MB | Open files | Threads | UI thread % |
|---|---|---|---|---|---|---|---|---|---|
| 0-5 | 40.9 | 104.0 | 182.1 | 3 | 26 | 596→633 | 55→55 | 64→64 | 7.8 |
| 5-10 | 70.1 | 188.0 | 194.9 | 182 | 92 | 633→637 | 55→55 | 64→64 | 8.3 |
| 10-15 | 69.1 | 153.0 | 168.1 | 140 | 91 | 637→638 | 55→55 | 64→64 | 8.2 |
| 15-20 | 69.3 | 259.5 | 299.7 | 1277 | 90 | 638→642 | 55→55 | 64→64 | 8.2 |
| 20-25 | 69.2 | 145.0 | 179.9 | 100 | 96 | 642→646 | 55→55 | 64→64 | 8.0 |
| 25-30 | 43.0 | 100.0 | 167.6 | 6 | 66 | 646→646 | 55→55 | 64→64 | 8.6 |
| 30-35 | 47.2 | 97.5 | 717.6 | 3 | 85 | 646→646 | 55→55 | 64→64 | 8.8 |
| 35-40 | 36.7 | 89.0 | 98.2 | 0 | 40 | 646→648 | 55→55 | 64→64 | 9.0 |
| 40-45 | 44.6 | 91.0 | 222.5 | 3 | 75 | 648→650 | 55→55 | 64→64 | 8.8 |
| 45-50 | 45.7 | 115.5 | 167.7 | 9 | 80 | 650→650 | 55→55 | 64→64 | 8.8 |
| 50-55 | 40.2 | 142.5 | 144.2 | 95 | 53 | 650→653 | 55→55 | 64→64 | 8.7 |
| 55-60 | 49.5 | 90.5 | 101.1 | 1 | 100 | 653→654 | 55→55 | 64→64 | 8.6 |
| 60-65 | 49.1 | 82.5 | 88.8 | 0 | 100 | 654→654 | 55→55 | 64→64 | 7.9 |

* **No crash, no stall.** The harness finished and quit normally.
* **No handle or thread leaks.** 55 open files and 64 threads from the first minute to the last.
* **Memory:** +37 MB in the first 5 minutes (caches and sample preloading filling up), then a slow, steady
  +21 MB over the remaining 55 minutes (~0.4 MB a minute, ~23 MB an hour). Small, but it doesn't level off, so
  something accumulates while playing. Since identified (section 6, C6): malloc's per-thread arenas, not the
  app's own data; the live heap stays flat.
* **No slowdown over time.** Windows on the same kind of core cost the same at the end as at the start (E-core
  windows ~45-50%, mostly-P-core windows ~37-41%); the over-budget bursts line up with the compiler windows and
  with time on the low-power cores, not with elapsed time.
* **Background load matters on this laptop.** While the compilers ran (niced, on other cores), the same song
  went from ~45% to ~70% average, with 100-1300 over-budget callbacks per 5 minutes, even on P-cores: the 15 W
  chip shares one power budget, so busy E-cores slow the P-core the audio thread is on. `nice` doesn't help;
  real-time priority doesn't either. Anything heavy running beside Lithium Studio (a build, a browser tab) costs audio
  headroom this way.

### 3.9 AddressSanitizer and ThreadSanitizer

Builds with `-DLITHIUM_SANITIZE=address` / `thread` (`-O1`, everything instrumented: JUCE, Tracktion, sfizz,
guitarix). Scenarios: the realistic song with add/remove tracks, add/remove effects, undo/redo, seeking, tempo
changes, autosave, play/stop, scrolling and the mixer for 90 s, then recording with monitoring for 30 s
(ASan); all of that plus recording in one 60 s run (TSan).

**AddressSanitizer:** no errors during any of the playing and editing. One **heap-use-after-free at every quit**
(finding A6), which stops the process there, so LeakSanitizer's leak check at exit never ran.

**ThreadSanitizer:** 80 reports, all with the same origin, `SfzPlugin::Loader::run()` (finding A5): sfizz keeps
some state in globals (its `fast_rand` random generator, the SIMD dispatch table, the sinc interpolators'
tables), and the loader thread builds a new `sfz::Sfizz` and renders 128 probe notes on it while the audio
thread renders the other sfizz instances. Besides those: a few benign-looking races in Tracktion (the plug-in
CPU-meter fields written by `Plugin::baseClassInitialise` while plug-ins are added during playback;
`AudioFileInfo` copied while a proxy renders), one inside libasound's PipeWire plug-in, and the same
use-after-free at quit. **No races on Lithium Studio's own data** (the FIFOs, atomics and try-locks between the UI,
audio, MIDI and background threads all came out clean). Not covered: sound-pack downloads (`PackStore`), which
need the network; the plug-in scan runs in separate processes.

## 4. Findings, most severe first

Effort: S = under half a day, M = 1-2 days, L = several days or upstream work.

### A. Crashes, races and real-time safety

**A1. Overload kills the app (SIGKILL). Fixed.** Section 1. `platform/AudioHardware.cpp` `onCpuLimit`,
`engine/RealtimeAudioThread.cpp` `timerCallback`. S, done.

**A2. Use-after-free in the SFZ loader when a big library is still loading.** Evidence: the Oct 23 segfault in
"SFZ loader" 18 s after `!! killing thread by force !!`. `engine/SfzPlugin.cpp`, `SfzPlugin::Loader::~Loader`
and `run()`: `stopThread (10000)` force-kills a thread that is still inside sfizz's `loadSfzFile`, and the
deferred cancel lets it keep running into a freed plug-in. Proposed fix: never kill it. Put what the loader
writes (`incoming`, `outgoing`, `numRegions`, key range, `loadedRequest`) in a small `std::shared_ptr` state that
the plug-in and loader share; when the plug-in goes, it marks the state abandoned and hands the `Loader` to a
background "finisher" that waits for it and deletes it (the abandoned result is simply discarded). M.

**A3. Third-party plug-in throws at quit.** `terminate called after throwing std::system_error` from the TONE3000
plug-in's asio code (section 1). Lithium Studio can't catch exceptions on another library's threads. Options: hosting
plug-ins in a separate process (L, also protects against plug-ins crashing during playback), or a terminate
handler installed at the start of `shutdown()` that exits quietly once the session is safe (S, cosmetic).

**A4. The audio thread allocates, locks and makes system calls, every block.** Evidence: `rtcheck` on the
realistic song (6,111 callbacks in 20 s): *every* callback has events; per callback ~113 mutex locks, ~48
read/write-lock acquisitions, ~0.9 `malloc`, ~0.6 `realloc`, and a `write()` system call on one callback in 3.
All but one source are in Tracktion or JUCE, not Lithium Studio's own DSP (none of Lithium Studio's plug-ins or callbacks
showed up):

| Where (from the symbolized stacks) | What |
|---|---|
| `te::DeviceManager::audioDeviceIOCallbackInternal` | a `HeapBlock` malloc + free per callback; a `CriticalSection` and a `shared_mutex` per callback |
| a graph node's `AsyncUpdater::triggerAsyncUpdate()` | posts a JUCE message from the audio thread: `realloc` + `write()` to wake the message thread |
| `te::LevelMeasurer::Client::ensureNumChannels` | `std::vector` growth in the level meters |
| `te::WaveInputDevice::consumeNextAudioBlock` | `AudioFifo::setSize` and `std::map::operator[]` allocate; a `CriticalSection` per block |
| `te::FourOscPlugin::applyToBuffer` | `juce::MPESynthesiser`'s `CriticalSection` and a `shared_mutex`, per 4OSC per block |
| `te::AudioFileCache::LockedReaderFinder` | `juce::ReadWriteLock`, whose internal `SpinLock` spins and calls `sched_yield()` when the cache thread holds it |
| `juce::AudioSourcePlayer` (Lithium Studio's `AudioPreview`) | a `CriticalSection` per callback, plus `AudioTransportSource`'s, even when nothing is previewed |
| `juce::AudioDeviceManager` | holds `audioCallbackLock` for the whole callback (taken by the message thread when callbacks are added/removed) |

Uncontended, these cost microseconds per block. They become dropouts when a non-real-time thread holds one of
the locks (priority inversion): this was **observed** for the audio file cache (3% of the audio thread's time
spent in `sched_yield` inside `LockedReaderFinder` during the tempo-change run; see B2/B3). Proposed: for
Lithium Studio's own (`AudioPreview`), only attach the preview player to the device while something is previewing (S);
for Tracktion, small patches in `external/patches` like the existing ones (the `AsyncUpdater` and per-callback
`HeapBlock` are the easy ones, M) and report the rest upstream (L). With Clang 20 installed, a
`-DLITHIUM_SANITIZE=realtime` build would catch any new ones automatically.

**A5. sfizz's globals are raced by the SFZ loader thread.** Evidence: all 80 ThreadSanitizer reports (3.9).
`engine/SfzPlugin.cpp` `Loader::run`: constructing `sfz::Sfizz` (line 67) initializes sfizz's static tables
(SIMD dispatch, sinc interpolators) and loading/probing (lines 71, 85) uses its global `fast_rand`, while the
audio thread renders other instances. Mostly harmless in practice (a random generator's state, tables being
written with the values they already have), but it is undefined behavior, and a half-written interpolation
table for one block would be a click. Proposed: create and destroy one `sfz::Sfizz` on the message thread at
startup, before any audio, so the static tables exist before threads race for them (S); and find the playable
key range from the loaded regions instead of rendering 128 probe notes on the loader thread (S-M).

**A6. Use-after-free at every quit.** Evidence: AddressSanitizer, both runs, identical stack:
`AudioEngine::~AudioEngine` destroys the Edit (`impl->edit.reset()`, `engine/AudioEngine.cpp`), and then
`Impl::~Impl` calls `autosaver.detach()`, which calls `edit->getUndoManager().removeChangeListener (this)` on
the freed Edit (`engine/Autosaver.cpp`, `Autosaver::detach`). Usually it reads memory nobody has reused yet and
nothing visible happens; it can crash on quit. Proposed fix: in `~AudioEngine`, call `impl->autosaver.detach()`
before `impl->edit.reset()` (`detach` sets its own `edit` to nullptr, so the later call in `~Impl` does nothing).
Note that this also removes the working copy at that point, as `~Impl` does now. S.

### B. Dropouts and xruns

**B1. The audio thread runs on slow cores.** Section 3.2: 30% on a P-core, 51% on an E-core, 93% and thousands
of xruns on a low-power E-core, chosen by the scheduler at random. This is the biggest single cause of both the
50-70% readings and the "random" dropouts on this laptop. `platform/AudioHardware.cpp` `makeThreadRealtime`:
on Linux, when the thread is made real-time, also restrict it with `sched_setaffinity` to the fastest cores
(those whose `cpufreq/cpuinfo_max_freq` is the highest; on non-hybrid CPUs that's all of them, so nothing
changes). Conservative variant: only exclude cores below ~60% of the fastest (the 2.1 GHz pair). Re-applied
when the device restarts. S-M (half a day with testing on the desktop, where the P-cores are shared with
GNOME).

**B2. Seeking while playing.** 4 seeks a second raise the average from ~50% to 75% and caused 51 over-budget
callbacks. Each jump makes every audio clip miss Tracktion's file cache; the audio thread then contends for the
cache's reader locks with the cache thread (A4) while the instruments restart their notes. One-off seeks give a
one-off spike. Structural (Tracktion's cache), L; mitigation: a larger cache read-ahead (M).

**B3. Tempo changes while playing.** One change a second: spikes up to 460% and 88 over-budget callbacks. Every
change makes Tracktion re-render the stretched copies ("proxies") of all auto-tempo audio clips with Rubber Band
on background threads (11% of the process's CPU in the profile; the clips are silent meanwhile), and the audio
thread fights those threads for the file cache's locks (one callback made 248,850 lock/allocation calls). Proposed:
while playing, apply tempo edits to auto-tempo clips only once the user lets go (most tempo edits are drags), or
keep the old proxy playing until the new one is ready. M-L.

**B4. One-off spikes when the graph changes.** Starting playback (157%), adding/removing a track (121%),
undo/redo (105%): one over-budget callback each, from the first block of a rebuilt graph (instruments
initializing, sfizz flushing its MIDI state). Low priority. M.

**B5. Small buffers need more than one render thread.** At 64 samples the realistic song gets 80 xruns in 32 s and
a 32-track mix 487 (1 thread) vs 2 (4 threads). Structural, see C2.

### C. High audio load

**C1. Tracktion's 4OSC synth patches dominate.** Per instance (one track playing chords + arpeggio, section 5):
Warm Pad 12.2%, Chamber Strings 11.4%, Basic Synth 8.7%, Glass Pluck 8.5% (E-core), Soft Keys 8.3%, Vintage Organ
7.7% (P-core) of the buffer; most sampled instruments cost 1-4.5%, the sampled Grand Piano 8.6%. In the realistic song, the two Warm Pads and Chamber Strings
alone are ~32% of the buffer at their busiest. Causes, from the patch definitions (`engine/SoundLibrary.cpp`
`synthPatches`) and the profile: unison stacks (Warm Pad 3+2 oscillators per voice, Chamber Strings 4+2), long
releases that keep many voices alive, a chorus and a reverb inside every instance (also processed when silent),
and inside Tracktion: per-sample atomic operations in `FourOscVoice::renderNextBlock` (9% of the audio
thread's time) and rendering in 32-sample slices. Quick win (S, but it's sound design, so your call): trim the
unison (e.g. 2+1), cap the polyphony, and turn off the per-instance reverb in favor of the Reverb send bus that
every session already has. Structural: an Lithium Studio synth or patches to 4OSC (L). (Since measured: the patch
changes save little; the cost is 4OSC's per-voice overhead. See section 6, quick win 2.)

**C2. Everything renders on one thread.** One core's worth of DSP is the ceiling: ~16 synth tracks, ~32 sampled.
Section 3.4. Proposed: 2-3 Tracktion render threads, made real-time the same way as the audio thread (find
Tracktion's worker threads and pass them to `platform::makeThreadRealtime`; RealtimeKit allows that), so they
don't fall behind on a busy desktop as in October. Accept the extra CPU of their spin-waiting, or only enable
the workers above a load threshold. M.

**C3. Instruments and effects keep processing silence.** With the realistic song stopped, the audio thread is 14%
busy (an empty session: 2%). Per instance: Amp 1.9%, Tune 1.2%, 4OSC 0.6%, Sound Shaper 0.35%, sfizz 0.3%, master
Limiter 0.5%. Tracktion keeps calling plug-ins while stopped (so tails and live input still work). Proposed for
Lithium Studio's own plug-ins (`AmpPlugin`, `TunePlugin`, `SoundShaperPlugin`, `GxPedalPlugin`, `BuiltInEffects`): skip
the DSP once the input has been silent for longer than the effect's tail (count silent blocks; any non-silent
sample resumes at once). S-M.

**C4. Guitar pedals process mono guitar twice.** `engine/PortedPedals.cpp` `GxPedalPlugin`: mono guitarix stages
run once per channel, and several have 2× or 4× linear-phase oversampling, so Golden Boost (5.7%), Stack Drive
(5.6%), Super Fuzz (4.1%) and Vintage Fuzz (3.8%) cost as much as a whole sampled instrument. A guitar track
carries the same signal on both channels; the Amp already detects that and processes once. Doing the same in
`GxPedalPlugin::applyToBuffer` would halve it. S.

**C5. Pitch Shift costs 8.8%** (Tracktion's real-time pitch shifter). Upstream; worth a note in the effect's
description if it stays.

**C6. Memory.** RSS is about 0.6 GB for the realistic song, 2 GB for 64 sampled-instrument tracks (sfizz preloads the
start of every sample). Adding and removing a track every 2 s grew RSS by 77 MB in 32 s, and plain playback grows
~23 MB an hour after warm-up (3.8). Since identified (section 6): no leaks and a flat live heap; both are
glibc's per-thread malloc arenas keeping freed memory, and both level off. M.

**C7. 96 kHz.** Everything costs about twice as much (realistic: 70% average, 2105 xruns unpinned). Expected; not viable
for this session size on this laptop without B1 and C2.

### D. UI and background

**D1. Regions repaint under the playhead.** `ui/arrangement/RegionComponent.cpp` `paint`: ~170 calls per second
while playing, 26.5% of the message thread's time in the profile (14.5% in `paintMidiNotes`), because each
30 Hz playhead move dirties a strip of every region it crosses, and each region then rebuilds a rounded-rectangle
`Path` clip and draws all its notes. Proposed: cache each region's drawing (`setBufferedToImage (true)`; the
cache is redrawn only when the region changes), or skip the path clip and the notes outside
`g.getClipBounds()`. S.

**D2. Dense automation floods the message thread** (8% → 26% of a core for 8 heavily automated tracks). Tracktion
writes every automated value into the session tree; the engine already holds mixer-only changes to ~30 Hz.
Proposed: also coalesce plug-in parameter changes while playing (M).

**D3. "Match system" theme polls the desktop portal over D-Bus every 2 s, synchronously, on the message thread**
(`ui/theme/ThemeManager.cpp`, `platform::systemPrefersDark`). A slow portal would freeze the UI for up to the D-Bus
timeout. Proposed: subscribe to the portal's `SettingChanged` signal (S).

**D4. `PitchEditor` scans every audio clip's tree 10 times a second** even when no pitch editing is in use
(`engine/PitchEditor.cpp` `timerCallback`). Cheap now, grows with session size. Make it event-driven (S).

**D5. The toolbar's CPU meter hides what matters** (section 2). Proposed: show the peak of the last second as
well (PerfMonitor's figure, or JUCE's xrun count) and flash on xruns (S).

**D6. Log noise.** sfizz prints "Cannot set current thread scheduling parameters" hundreds of times while loading;
Tracktion logs "*** Cache miss" from the audio thread in Debug builds (the Oct 20 log has one every 4 ms).
Silence sfizz's message (S); the Debug one doesn't affect release builds.

**D7. Worth knowing, measured as negligible:** Lithium Studio's five extra JUCE callbacks (tuner, live waveform,
recording guard, preview, real-time helper) each clear and mix a buffer (microseconds); Tracktion also enables
every output pair the device lists (64 on PipeWire's "default") and updates a level meter for each enabled one;
the master Limiter is ~0.8% always; sends and buses are almost free; denormals are already flushed for the whole
audio thread by Tracktion (`disableDenormalisedNumberSupport` in every callback), so the per-plug-in
`ScopedNoDenormals` are harmless but redundant; autosave encodes and writes on a background thread (only
`flushState` and a `ValueTree` copy run on the message thread).

## 5. Per-instrument and per-effect cost

Tracktion's own measurement of each plug-in (`Plugin::getCpuUsage`, smoothed), as % of a 256-sample buffer at
48 kHz, average over the run / highest one-second value. One track each, playing the same chords and
arpeggio (instruments) or a library loop (effects). Core: P = the run was on a P-core, E = on an E-core (×~1.6).
Every sampled instrument also has a hidden Sound Shaper (0.5-0.6%), and the master Limiter adds ~0.8%.

| Instrument | Plug-in | avg % | peak % | Core |
|---|---|---|---|---|
| Warm Pad [Synth] | 4OSC | 12.2 | 13.4 | E |
| Chamber Strings [Synth] | 4OSC | 11.4 | 12.3 | E |
| Basic Synth [Synth] | 4OSC | 8.7 | 11.2 | E |
| Grand Piano [Sampled] | SFZ Instrument | 8.6 | 10.2 | P |
| Glass Pluck [Synth] | 4OSC | 8.5 | 11.2 | E |
| Soft Keys [Synth] | 4OSC | 8.3 | 10.4 | P |
| Vintage Organ [Synth] | 4OSC | 7.7 | 9.2 | P |
| Violas [Sampled] | SFZ Instrument | 4.5 | 5.2 | E |
| Violins [Sampled] | SFZ Instrument | 3.9 | 4.6 | E |
| Cellos [Sampled] | SFZ Instrument | 3.8 | 5.9 | E |
| Electric Guitar [Sampled] | SFZ Instrument | 3.6 | 5.3 | E |
| Solo Violin [Sampled] | SFZ Instrument | 3.6 | 4.4 | E |
| Hollow Body Guitar [Sampled] | SFZ Instrument | 3.4 | 5.0 | E |
| Upright Piano [Sampled] | SFZ Instrument | 3.4 | 5.1 | P |
| Bright Lead [Synth] | 4OSC | 2.9 | 3.2 | E |
| Electric Bass [Sampled] | SFZ Instrument | 2.6 | 3.7 | P |
| Pizzicato Strings [Sampled] | SFZ Instrument | 2.4 | 3.6 | E |
| Red Zeppelin Kit [Sampled] | SFZ Instrument | 2.1 | 2.8 | P |
| Synth Bass [Synth] | 4OSC | 2.1 | 2.9 | E |
| Rusty Kit [Sampled] | SFZ Instrument | 2.0 | 3.0 | P |
| Double Basses [Sampled] | SFZ Instrument | 2.0 | 3.0 | E |
| Wurlitzer [Sampled] | SFZ Instrument | 1.8 | 2.3 | P |
| Sub Bass [Synth] | 4OSC | 1.6 | 2.1 | E |
| Pianet [Sampled] | SFZ Instrument | 1.6 | 2.4 | P |
| Drummer | SFZ Instrument | 1.6 | 2.7 | E |
| CP80 Electric Grand [Sampled] | SFZ Instrument | 1.5 | 2.0 | P |
| Staccato Bass [Sampled] | SFZ Instrument | 1.1 | 1.7 | P |
| Electric Piano [Sampled] | Sampler | 1.0 | 1.2 | P |
| Soft Piano [Sampled] | Sampler | 0.9 | 1.3 | E |
| Clean Electric [Sampled] | Sampler | 0.9 | 1.3 | E |
| Picked Bass [Sampled] | Sampler | 0.8 | 1.4 | E |
| Nylon Guitar [Sampled] | Sampler | 0.8 | 1.2 | E |
| Pizzicato [Sampled] | Sampler | 0.8 | 1.3 | E |
| Electronic Kit [Sampled] | Sampler | 0.1 | 0.3 | P |

| Effect | Plug-in | avg % | peak % | Core |
|---|---|---|---|---|
| Pitch Shift | Pitch Shifter | 8.8 | 10.3 | E |
| Golden Boost | Golden Boost | 5.7 | 6.1 | E |
| Stack Drive | Stack Drive | 5.6 | 5.8 | E |
| Amp | Amp | 4.5 | 4.7 | E |
| Super Fuzz | Super Fuzz | 4.1 | 4.3 | E |
| Vintage Fuzz | Vintage Fuzz | 3.8 | 4.0 | E |
| Tube Drive | Tube Drive | 2.5 | 2.7 | E |
| Tune | Tune | 2.4 | 3.1 | E |
| Mellow Drive | Mellow Drive | 2.3 | 2.5 | E |
| Hot Plate | Hot Plate | 1.4 | 1.6 | E |
| Limiter | Limiter | 0.9 | 1.0 | E |
| Envelope Wah | Envelope Wah | 0.7 | 0.9 | E |
| Distortion | Distortion | 0.7 | 0.8 | E |
| Sun Fuzz | Sun Fuzz | 0.5 | 0.6 | E |
| Reverb | Reverb | 0.5 | 0.6 | E |
| Phaser | Phaser | 0.4 | 0.5 | E |
| Spring Tank | Spring Tank | 0.4 | 0.4 | E |
| Yellow Drive | Yellow Drive | 0.3 | 0.3 | E |
| Tremolo | Tremolo | 0.2 | 0.3 | E |
| Jet Flanger | Jet Flanger | 0.2 | 0.3 | E |
| Chorus | Chorus | 0.2 | 0.3 | E |
| Noise Gate | Noise Gate | 0.2 | 0.2 | E |
| Low/High-Pass Filter | LPF/HPF | 0.1 | 0.1 | E |
| Compressor | Compressor | 0.1 | 0.1 | E |
| Echo | Delay | 0.1 | 0.1 | E |
| Slow Swell | Slow Swell | 0.0 | 0.1 | E |

Not listed: EQ (Tracktion's 4-band) measured under 0.05%. "Sampler" = Tracktion's sampler, used by the small
bundled instruments; "SFZ Instrument" = sfizz. Lithium Studio's ports: Golden Boost, Stack Drive, Super/Vintage/Sun Fuzz,
Tube/Mellow/Yellow Drive and Hot Plate are the guitarix pedals (`PortedPedals.cpp`), Amp is `AmpPlugin.cpp`,
Tune is fat1 (`TunePlugin.cpp`); the rest are Tracktion's.

## 6. Quick wins vs structural work

**Done since this report was written** (re-measured with the same harness, 256 samples, 48 kHz):

* Quick win 1, keeping the audio thread on the fastest cores (`platform::keepOnFastestCores`, called by
  `RealtimeAudioThread`): realistic song 29.9% average, p99 42%, max 63%, no over-budget callbacks, every measured
  callback on a fast core. This meets the 16-track target.
* Quick win 3, skipping DSP on silent input (`engine/SilenceSkip.h`, in all of Lithium Studio's own effects and the Sound
  Shaper): with the realistic song stopped, none of them shows up any more (they were ~7% together); what remains
  (8.9% in all) is Tracktion's 4OSC (~0.9% each) and sfizz (~0.3% each). Smoke test: "Effects rest during silence,
  but never cut sound off" (Pedals section).
* Quick win 9, the use-after-free at quit (A6): clean under AddressSanitizer.
* Structural 2 (first half), an SFZ loader that's never force-killed (A2): a loader still busy when its plug-in
  goes finishes on its own and is deleted afterwards (`FinishingThreads` in `SfzPlugin.cpp`).
* Structural 2 (second half), the key range without rendering (the rest of A5): the loader asks sfizz for its
  `/key/slots` (the keys the loaded regions cover, after `note_offset`/`octave_offset`) instead of playing all
  128 keys. It now counts every mapped key, whatever its velocity layer or keyswitch; the probe only found what a
  velocity-100 note started. Smoke test: "an SFZ instrument's key range is read from its regions, whatever their
  velocities". Rendering wasn't the only user of sfizz's random generator: each new synth seeds its 64 voices
  from it, and loaders for several SFZ tracks make synths at once. `external/patches/sfizz-thread-local-random.patch`
  gives each thread its own. ThreadSanitizer, same scenario: 14 reports (29 after quick win 10); none from
  `fast_rand` (14 before). Left: sfizz's `std::cerr` (3, D6), sfizz's `FilePool` teardown (1), libasound (2),
  Tracktion's `baseClassInitialise` fields (3 data races plus 1 under the SfzPlugin's own `sampleRate` read),
  and 4 heap-use-after-free reports in Tracktion's `AudioProxyGenerator::findJob` on the message thread (a proxy
  job freed while `AudioClipBase::timerCallback` still looks it up; 0 in the first run, 3 after quick win 10,
  depending on when the tempo changes land; fixed since, see below).
* Quick win 10, sfizz's shared tables (A5, first half): `SfzPlugin::prepareSfizz` makes one `sfz::Sfizz` on the
  message thread when the engine starts. That alone wasn't enough: sfizz re-ran its table setup in *every*
  synth's constructor, rewriting the SIMD dispatch table and the sinc-table pointers (with the same values)
  while the audio thread read them. `external/patches/sfizz-init-once.patch` (applied by CMake like the JUCE and
  Tracktion ones) makes that setup run once. ThreadSanitizer, same scenario as 3.9: 29 reports instead of 80;
  none left for the SIMD dispatch (44 before), the sinc interpolators (8) or the silent wavetable (1). Left:
  sfizz's `fast_rand` (14, the next item), sfizz writing to `std::cerr` from several threads (5, D6), one sfizz
  `FilePool` teardown, and Tracktion's and libasound's as before.
* Quick win 5, the preview player only while previewing (A4): `AudioPreview` adds JUCE's `AudioSourcePlayer` to
  the device when a file starts and removes it when the file ends or is stopped. `rtcheck`, realistic song, 20 s,
  nothing previewed: the player's per-callback lock is gone from the sampled stacks (76 samples before, 0 after);
  with the player no longer called, neither is `AudioTransportSource`, so its lock goes too. Locks per callback went from 117 to 110, but that total moves from
  run to run with what's playing, so the stacks are the evidence. The load is unchanged (29.6% vs 30.3%, both
  runs entirely on the fast cores). Smoke test: "Previewing library files" (Preferences section).
* Quick win 4, mono guitar through the pedals once (C4): `GxPedalPlugin` compares the two channels before each
  mono stage; when they're the same it runs the left channel's DSP copy only and copies the result. When they
  start to differ, the right copy first takes the left one's state (a plain member copy, `gx::DspType::copyState`;
  none of the mono DSPs owns buffers), so it carries on exactly where it should: no click, the Jet Flanger's sweeps
  stay in step. Stereo input is processed as before. `effect-each`, each pedal's own share, before → after:
  Stack Drive 3.77 → 1.91%, Tube Drive 1.88 → 1.08, Hot Plate 1.10 → 0.59, Sun Fuzz 0.63 → 0.35, Envelope Wah
  0.67 → 0.38, Jet Flanger 0.28 → 0.15. The oversampled ones barely move (Super Fuzz 3.48 → 3.12, Vintage Fuzz
  3.39 → 3.13, Mellow Drive 1.77 → 1.52): their oversampling filters, which still run on both channels so
  the right channel's history is right when it's needed, are most of their cost. Golden Boost (4.9%) isn't a
  `GxPedalPlugin`, and its ChowCentaur circuit models can't be copied the same way; unchanged. All runs on the
  fast cores. Smoke test: "Mono guitar goes through a pedal once, and stereo stays stereo" (Pedals section).
* Quick win 6, region drawing under the playhead (D1): `RegionComponent::paint` now draws only what meets
  `g.getClipBounds()`. The rounded-shape clip is used only when the repainted area reaches a corner (elsewhere
  the shape is the plain rectangle); the name (its measured width is kept, not the whole strip's), icons,
  take badge, fades and MIDI notes are skipped outside it; where the strip crosses a note between its rounded
  ends, or the outline away from the corners, they're filled as plain rectangles instead of paths. A repaint of
  the whole region draws exactly as before. `ui` set, `RegionComponent` paint time while playing, before →
  after (calls per second unchanged, ~170-210): plain playback 24.4 → 7.2 ms/s, mixer open 28.3 → 6.9,
  library on/off 37.4 → 7.6, theme switching 31.3 → 7.9, zoom 32.6 → 9.3, resize 33.3 → 11.7; scrolling
  39.6 → 22.6 (scrolling repaints whole regions, which cost the same as before). All paint during plain
  playback 51.2 → 40.5 ms/s, message thread 7.2 → 6.4% of a core. The message thread isn't kept on any
  core, and the meters' own paint (unchanged code) was ~20% slower in the after runs, so these are, if
  anything, low. All runs on the fast cores. Checked under Xvfb, light and dark, with a selected region:
  after the playhead sweeps over the regions they differ from their first full paint by at most 2 of 255
  in a few anti-aliased note edges, as in the old build (the same rounding; no seams).
* Quick win 7, load spikes and xruns in the toolbar (D5): `engine/AudioLoadMeter` times every block on its own,
  over the same span as JUCE's figure (a callback pair added before Tracktion's, bracketing the whole callback
  like PerfMonitor's; on the audio thread two clock reads and one compare-and-swap), and counts xruns from
  `AudioDeviceManager::getXRunCount`, leaving out each device start's first second. The toolbar keeps the
  average ("CPU 34%") and adds what it hid: the number turns amber for a second when a block took 90% of its
  time or more, and "2 xruns" appears after the first one, the whole line red for 3 s after each. The stats
  area is 130 px instead of 110. With `LITHIUM_PERF` on, PerfMonitor already brackets the callback, so the
  toolbar takes its last-second maximum instead (two pairs can't both run first and last). Not a load
  change: every harness run already had this layout (Tracktion no longer JUCE's first callback, so its output
  is copied once more per block), and in the app the pair costs a few microseconds a block. Checked under
  Xvfb, light and dark, with `LITHIUM_PERF_BURN` at 0, 92 and 130% and back to 0: gray, amber, red with the
  count, then gray with the count kept; no xrun shown after startup. Smoke test: "Load meter" (main section).
* Quick win 8, the theme follows the portal's signal instead of polling (D3): `platform::SystemAppearance`
  reads the portal's color scheme once at startup (subscribing to `SettingChanged` first, so a change in
  between isn't missed), then keeps it from the signal. Its sd-bus socket is watched by JUCE's event loop
  (`LinuxEventLoop::registerFdCallback`), so the signal is handled on the message thread without waiting and
  without a thread of its own; if the bus goes away it stops watching and keeps the last value. `ThemeManager`
  no longer has a timer. `dbus-monitor`, app running 26 s with Match system: one `ReadOne` at startup and none
  after (before: one every 2 s, each able to hold the UI for up to the 250 ms timeout). Checked under Xvfb on
  the real session bus: switching GNOME's style to dark and back with `gsettings` switched the app within the
  2 s between screenshots each way; with no session bus the app starts as before and falls back to JUCE's
  check. Not covered by the smoke test (app-only code).
* Quick win 2, lighter synth patches (C1): measured, and not done; the bundled patches are unchanged. The cost
  isn't in what the patches ask for. `instrument-each` with 30 variants beside the originals (4OSC's own share
  of the buffer, %): Warm Pad 9.9, with 2+1 unison 9.8, 12 voices 9.6, 8 voices 9.3, no reverb 9.7, all of
  those at once 8.5; Chamber Strings 9.5 → 8.3 with all of them; Basic Synth (one sine) 7.5 → 6.7 with its
  filter and reverb off; Glass Pluck, Soft Keys and Vintage Organ 6-10% less with everything off; one change
  alone is within the ±0.3 noise. Moving the reverb to the bus saves 0.2-0.6% an instance. The profile showed
  why: the time goes in 4OSC's per-voice bookkeeping, the same for a sine as for a 3+2 unison saw. Each voice
  reads ~50 parameters every 32 samples, and each read copied a reference-counted pointer (two atomic
  operations, 16% of the audio thread) and did two map lookups (~11%). Instead,
  `external/patches/tracktion-fourosc-param-by-reference.patch` passes the pointer by reference in both
  `paramValue` functions. Same output: every built-in patch rendered through a standalone 4OSC (chords and
  arpeggio, fixed blocks, the oscillators' random start phases given a fixed seed for the comparison only) is
  byte-identical before and after. `instrument-each`, before → after: Basic Synth 7.7 → 6.5, Sub Bass 1.6 →
  1.4, Soft Keys 8.5 → 7.8, Glass Pluck 7.5 → 7.0, Vintage Organ 7.5 → 7.1, Synth Bass 1.8 → 1.7, Bright
  Lead 2.5 → 2.3; Warm Pad (9.6 → 9.7) and Chamber Strings (9.3 → 9.2), where the oscillators weigh most,
  unchanged within noise. Six synth tracks: audio thread 5.3 → 4.7 s of CPU in 20 s, callback 21.6 → 19.9%;
  the atomics are gone from the profile, which had overstated what they cost. All runs on the fast cores
  (one 121% callback in the after run's Warm Pad case, no xrun; one-off). Left in 4OSC: the map lookups,
  the oscillators (~10%), its filter and per-instance reverb. Not covered by the smoke test (no behavior
  change).
* Structural 1, real-time render workers (C2, B5): Lithium Studio now renders on **2 threads**, the audio thread and
  one Tracktion worker, and the worker gets the audio thread's treatment: real-time priority (RealtimeKit),
  kept on the fast cores, asked again once the load is light. Tracktion couldn't do that itself on Linux:
  it asks for priority 99 directly, which an ordinary user can't get, and its other call does nothing
  there. `external/patches/tracktion-render-thread-hook.patch` names the workers "Render worker" and calls
  a function of the app's on each one as it starts and ends; `RealtimeAudioThread` notes their thread IDs
  and promotes them from the message thread. Only playback's workers: exports (and Tracktion's other
  offline renders) use the same thread count, but render flat out, and a real-time worker running flat
  out would trip the CPU limit and cost the live audio thread its real-time too. The patch lets the
  playback player mark itself live (`LockFreeMultiThreadedNodePlayer::setRendersLive`); the others'
  workers are named "Offline worker" and stay ordinary. The CPU-limit handler (`platform/AudioHardware.cpp`) used to
  remember only the last thread made real-time, so with a worker promoted after the audio thread it would
  have left the audio thread real-time and the kernel would have killed the app (the crash in section 1).
  It now keeps every promoted thread and puts them all back to ordinary priority when any one runs too long.
  `overload` with 4 threads: all four lost real-time together at ~108% and the app ran on through 200%.
  The toolbar's real-time badge now means the audio thread and the worker both are.
  `threads` set (32 s, 48 kHz, all audio-thread callbacks and every worker sample on CPUs 0-3 when
  real-time; ordinary workers wandered over all 14 cores):

  | Idle desktop | 32-track mix, 64: avg / max %, xruns | 32-track mix, 256 | Realistic, 64 | Worker CPU, % of a core |
  |---|---|---|---|---|
  | 1 thread (before) | 61.8 / 161, **59** | 52.6 / 117, **4** | 33.8 / 91, 0 | – |
  | 2, worker at ordinary priority | 36.5 / 222, **16** | 28.2 / 111, **3** | 21.7 / 90, 0 | 20-33 |
  | **2, worker real-time** | 36.1 / 86, 0 | 28.3 / 55, 0 | 20.3 / 49, 0 | 18-33 |
  | 4, workers real-time | 30.6 / 56, 0 | 25.8 / 46, 0 | 17.8 / 36, 0 | 44-74 |

  With a build running beside the app (`BUSY=1`: 8 compiles of sfizz's sources at a time, normal
  priority), the numbers swing between attempts, because the load slows every core (the shared 15 W power
  budget, and hyper-threading: CPUs 0-3 are two physical cores shared with the compilers), and one
  overloaded stretch of 100 ms costs real-time for the rest of a run. Three passes, xruns in 32 s:
  realistic at 64: 1 thread 9911 and 16553, 2 threads 189, 1 and 2, 4 threads 0 and 0; 32-track mix at
  256: 1 thread 533 and 3827, 2 threads 1691, 0 and 1121, 4 threads 302 and 0. Run side by side (same
  load, back to back) 1 thread lost real-time and 2 didn't: 16553 vs 2 and 3827 vs 1121. Every run with
  thousands of xruns had lost real-time partway. A 32-track mix with a build running is past what one
  pair of P-cores can do whatever the thread count; the 16-track song holds with 2.
  Why 2 and not 4: 4 is a few percent lighter on the audio thread but its workers spin for 2-3 times the
  CPU while playing, and with the transport stopped (realistic song) the process uses 13.7% of a core on
  1 thread, 15.7% on 2 and 23.7% on 4. Left: once real-time is lost under a busy desktop it rarely comes
  back during play (the load must stay under 70% for 5 s first); worth revisiting. Smoke test: "Render
  workers" (main section) and "an export's workers aren't made real-time" (Export section).
* Memory growth (C6): investigated, not fixed. **Nothing leaks, and the app doesn't keep more as it runs: the
  growth is glibc's malloc holding on to freed memory in its per-thread arenas.**
  LeakSanitizer (ASan build, add/remove for 4 minutes, normal quit): 43 KB in 256 allocations, all inside
  libasound (`snd_pcm_open` while JUCE probes the device at startup, 36 KB) or thread start-up records (7 KB);
  none in Lithium Studio, Tracktion or sfizz. heaptrack (build-perf) shows the live heap flat while RSS climbs:
  add/remove, minutes 1-8, heap 472-480 MB while RSS went 474 → 730 MB (peak heap 581 MB after 3 minutes, 597
  after 10, the difference being which instruments happened to be loaded); plain playback, 20 minutes, heap
  451-457 MB the whole time. Then the same runs with glibc's tunables, nothing else changed:

  | | Default (19 arenas while playing) | `GLIBC_TUNABLES=glibc.malloc.arena_max=1` |
  |---|---|---|
  | Add/remove, RSS after each 24 s round of track types | 541, 562, 589, 615, 617, 646, 668 ... 691, 677 (levels off at ~690 after ~3 minutes) | 545, 550, 550, 550 ... 551 (flat from the second round) |
  | Plain playback, 15 min, anonymous memory | 443 → 509 MB, +2 MB in the last 7 minutes | 399 → 411 MB, flat from minute 2 (+0.5 MB in 13 minutes) |

  Memory-mapped files (the audio loops) level off at 59 MB in both, so they aren't the growth. A fixed mmap
  threshold (`glibc.malloc.mmap_threshold=262144`) didn't help (add/remove still rising, 687 MB after 5 minutes).
  Why the arenas fill up: each thread that allocates gets its own arena, and an arena other than the main one
  only gives memory back from its very top, so freed blocks in the middle stay counted in RSS. Instruments
  load on short-lived threads (26 SFZ loader threads in 5 minutes of add/remove, sfizz's file pool, Tracktion's
  jobs), and while playing the audio thread, render worker and message thread make ~35,000 short-lived
  allocations a second between them, each in its own arena. Both curves flatten: add/remove within a few
  minutes, playback towards the ~20 MB an hour seen in 3.8. Options, for later: `mallopt(M_ARENA_MAX, ...)`
  at startup with a small number (not 1: every thread, the audio thread included while Tracktion still
  allocates there (A4), would share one malloc lock; the tunable runs above ran two at a time, so they say
  nothing about load), a `malloc_trim(0)` on the message thread after an instrument is removed, or one
  long-lived SFZ loader thread instead of one per load. Not covered by the smoke test (no code change).
* The heap-use-after-free in `AudioProxyGenerator::findJob` (the 4 reports left in Structural 2): when a
  clip's proxy changes, `AudioClipBase::timerCallback` deletes the old one with `deleteProxy`, which
  removes its job from the thread pool. A job still waiting in the queue is deleted without ever
  running, so it never reached the `removeFinishedJob` at the end of `runJob` and stayed in
  `activeJobs`; the next `beginJob` read it in `findJob`. Tempo changes make proxies change, so it
  depended on when they landed. `external/patches/tracktion-proxy-job-list.patch` takes the job off
  the list in its destructor (removing it twice does nothing). ThreadSanitizer, same scenario as 3.9
  (60 s): no `findJob` or proxy-generator reports and no use-after-free (4 before); 15 reports, all
  known kinds (`baseClassInitialise` fields, also read by the Delay; `AudioFileInfo` copied while
  playback reads it; sfizz's `std::cerr`; libasound; the SfzPlugin's `sampleRate` read). Not covered by
  the smoke test (it depends on timing). Tracktion has since been updated to `develop` of 2026-10-24
  (JUCE 8.0.13 → 9.0.5); upstream hasn't touched this code, so the patch is still needed.

**Quick wins** (each about half a day or less; all measured above):

| # | Change | Where | Expected effect |
|---|---|---|---|
| 1 | Keep the audio thread on the fastest cores (or off the low-power ones) | `platform/AudioHardware.cpp` | 30-60% lower load and no LP-core xrun bursts on hybrid laptops (B1) |
| 2 | Lighter synth patches: less unison, capped polyphony, no per-instance reverb (use the bus) | `engine/SoundLibrary.cpp` patches | Warm Pad / Strings / Keys from 7-12% to a few % each (C1); your sound-design call |
| 3 | Skip DSP on silent input in Lithium Studio's own plug-ins | Amp, Tune, Sound Shaper, pedals, built-in effects | stopped-transport load from 14% towards the 2% baseline; also saves on silent tracks while playing (C3) |
| 4 | Process mono guitar once in the pedals | `engine/PortedPedals.cpp` | halves pedal cost (C4) |
| 5 | Attach the preview player only while previewing | `engine/AudioPreview.cpp` | two fewer locks per callback (A4) |
| 6 | Cache region drawing / clip-aware region paint | `ui/arrangement/RegionComponent.cpp` | most of the UI's paint time while playing (D1) |
| 7 | Show peak load and xruns in the toolbar | `ui/toolbar/Toolbar.cpp` | the meter stops hiding spikes (D5) |
| 8 | Theme: portal signal instead of polling | `ui/theme/ThemeManager.cpp` | no synchronous D-Bus on the UI thread (D3) |
| 9 | Detach the autosaver before destroying the Edit | `engine/AudioEngine.cpp` `~AudioEngine` | the use-after-free at quit (A6); one line |
| 10 | Make one `sfz::Sfizz` on the message thread at startup | `engine/AudioEngine.cpp` or `SfzPlugin.cpp` | most of the sfizz races (A5) |

**Structural** (days, or changes inside Tracktion):

| # | Change | Why |
|---|---|---|
| 1 | Real-time render workers (2-3 threads, promoted like the audio thread) | removes the one-core ceiling and makes 64-128 samples workable (C2, B5) |
| 2 | SFZ loader that is never force-killed, and finds the key range without rendering | the use-after-free crash (A2) and the rest of the sfizz races (A5) |
| 3 | Tracktion patches for the audio-thread allocations and the posted message (A4); upstream reports for the locks | real-time safety |
| 4 | Tempo edits during playback without re-rendering every proxy at once | the tempo-change dropouts (B3) |
| 5 | Seek handling / cache read-ahead | the seek dropouts (B2) |
| 6 | Out-of-process plug-in hosting | third-party crashes (A3), slow plug-ins can't stall the session |
| 7 | Coalesce automation changes on the message thread | UI load with heavy automation (D2) |

## 7. The tools, and how to rerun everything

Everything below is in the codebase and does nothing unless the app is started with `LITHIUM_PERF=1`
(`LITHIUM_PERF_TOOLS=OFF` in CMake compiles it out).

| Piece | Where | What it does |
|---|---|---|
| Performance monitor | `src/engine/PerfMonitor.*` | Times every audio callback start to end (two extra JUCE callbacks that bracket all the others), pushes one record per callback through a lock-free `juce::AbstractFifo`; the message thread turns them into min/avg/p50/p99/max per second and per run, counts over-budget callbacks, late starts and the device's xruns, notes which core each callback ran on, reads Tracktion's own figures and every plug-in's share, CPU per thread from `/proc/self/task` (like `top -H`), memory, open files and threads. One CSV line per second (`LITHIUM_PERF_CSV`, default `~/lithium-perf/perf-<time>.csv`) plus `.plugins.csv` and `.threads.csv`. |
| Synthetic overload | same | `LITHIUM_PERF_BURN=<percent>` (or a number in the file named by `LITHIUM_PERF_BURN_FILE`, re-read every second) spins on the audio thread for that share of each buffer. |
| Render threads | `AudioEngine.cpp` | `LITHIUM_PERF_AUDIO_THREADS=N` overrides Tracktion's render thread count (normally 2); `LITHIUM_PERF_WORKERS_RT=0` leaves the workers at ordinary priority. |
| Overlay | `src/ui/widgets/PerfOverlay.*` | Live numbers in the top-right corner; **Ctrl+Shift+P** shows/hides it (`LITHIUM_PERF_OVERLAY=0` starts hidden). |
| Paint timing | `src/ui/widgets/PaintProbe.*` | `LITHIUM_PERF_PAINT ("Name");` at the top of a `paint()`: calls and milliseconds per component per second, frames and the longest frame (to `MainComponent::paintOverChildren`). In the main components' paint methods. |
| Repaint flashing | CMake `-DLITHIUM_REPAINT_DEBUGGING=ON` | Sets `JUCE_ENABLE_REPAINT_DEBUGGING` (a full rebuild; use its own build folder). |
| Stress harness | `src/app/PerfHarness.*` | `--perf-scenario=NAME` builds a session from scratch, plays its 16-bar loop, optionally does things meanwhile (`--perf-actions=`), appends a result line to `--perf-summary=FILE`, quits. `--perf-tracks`, `--perf-buffer`, `--perf-rate`, `--perf-seconds`, `--perf-warmup`, `--perf-plugin`, `--perf-burn-ramp`. |
| Scenario runner | `tools/perf/run_matrix.sh` | Runs sets of scenarios headless (Xvfb) and silent (a private `HOME` whose `.asoundrc` routes ALSA to a PipeWire null sink and a silent virtual source; never the speakers or microphone). Sets: `quick`, `matrix`, `features`, `ui`, `overload`, `cores`, `threads`, `rtcheck`, `profile`, `endurance`, `one`. `BUSY=1` runs a build (compiles of sfizz's sources into the run folder) beside the app the whole time. |
| Tables | `tools/perf/summarize.py` | Markdown tables from a run folder's `summary.csv`, or per plug-in from a `.plugins.csv`; `--threads` shows where the audio thread and the render workers ran, and whether they stayed real-time. |
| Real-time checker | `tools/perf/rtcheck.c` | `LD_PRELOAD` library: counts allocations, frees, mutex/rwlock/condition waits, semaphores, sleeps and file I/O made on the audio thread inside the callback (PerfMonitor calls its enter/leave hooks), with sampled stacks. The GCC stand-in for Clang's RealtimeSanitizer. |
| Sampling profiler | `tools/perf/sampler.c` | `LD_PRELOAD` library: per-thread CPU-time timers (like `perf`'s task-clock), stacks via `backtrace()`; for when `perf` isn't allowed (`kernel.perf_event_paranoid=4` here). |
| Symbolizer | `tools/perf/symbolize.py` | Turns rtcheck/sampler output into per-thread hot functions and stacks (addr2line). |
| Sanitizers | CMake `-DLITHIUM_SANITIZE=address\|thread\|realtime` | Instruments everything (JUCE, Tracktion, libraries). `realtime` needs Clang 20+ (`apt install clang`): PerfMonitor then wraps the callback in `__rtsan_realtime_enter/exit`. |

Typical use:

```sh
# One-off build for measuring (optimized, with symbols; leaves build/ and build-release/ alone):
cmake -S . -B build-perf -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      "-DCMAKE_CXX_FLAGS_RELWITHDEBINFO=-O3 -g1 -DNDEBUG" "-DCMAKE_C_FLAGS_RELWITHDEBINFO=-O3 -g1 -DNDEBUG"
cmake --build build-perf --target LithiumStudio -j3

tools/perf/run_matrix.sh build-perf quick        # the realistic song, ~1 minute
tools/perf/run_matrix.sh build-perf matrix       # every scenario x buffer size, ~25 minutes
python3 tools/perf/summarize.py ~/lithium-perf/runs/<date-time>

# Live, on the desktop, with the overlay:
LITHIUM_PERF=1 "build-perf/LithiumStudio_artefacts/RelWithDebInfo/Ikaros Studio"
```

The sound packs used are the ones in `lithium-packs/dist`, unzipped to `~/lithium-perf/sounds` (the user's own
library in `~/Documents/Ikaros Studio/Sounds` is never read). Run outputs are in `~/lithium-perf/runs/`. The two
PipeWire nodes the runner creates stay until `tools/perf/run_matrix.sh cleanup`.
