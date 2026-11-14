#pragma once

namespace bl90 {
constexpr int BANK_COUNT = 11;
// Stable sample-bank indices: append future entries, never reorder saved selections.
static const char* const BANK_FILES[BANK_COUNT] = {
  "break01.wav", "warehouse_run.wav", "chrome_break.wav", "redline_break.wav",
  "neon_shuffle.wav", "dust_circuit.wav", "razor_step.wav", "basement_snap.wav",
  "ghost_runner.wav", "concrete_swing.wav", "subway_funk.wav"
};
static const char* const BANK_NAMES[BANK_COUNT] = {
  "Original Break", "Warehouse Run", "Chrome Break", "Redline Break", "Neon Shuffle",
  "Dust Circuit", "Razor Step", "Basement Snap", "Ghost Runner", "Concrete Swing", "Subway Funk"
};
constexpr unsigned MAX_DECODED_FRAMES = 8u * 1024u * 1024u; // two float channels: 64 MiB total
}
