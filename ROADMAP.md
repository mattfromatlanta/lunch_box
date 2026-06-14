# Roadmap

Plans beyond the current release. Timing is best-effort; items can move.

## 1.2

- **TEMPO firmware support** — CHOMPI's TEMPO firmware uses a different sample
  architecture from TAPE: a single bank (A) per sample type, `chroma_a1.wav` /
  `slice_a1.wav` naming, and samples played from internal memory with a 10-second
  maximum (16-bit 48kHz stereo WAV). Lunch Box will add a TEMPO export mode —
  category naming, slot limits, and duration validation — alongside the existing
  TAPE workflow. (TAPE and TEMPO also use different SD card layouts, so packs are
  not interchangeable between firmwares.)

## Later / unscheduled

- **First-class Linux and Windows support** — both currently compile (Linux in CI)
  but are untested and unpackaged; clipboard import from the system file manager
  is macOS-only.

Suggestions welcome — open a [discussion](https://github.com/mattfromatlanta/lunch_box/discussions).
