#pragma once

namespace bl90 {
// MPC forwards the track's pad-mode notes to VST instruments. A scale layout
// therefore needs its own map; MIDI note order and physical pad order differ.
// Layout 0 retains the historical note 36..51 mapping for saved projects.
enum PadLayout { LegacyPads = 0, ChromaticPads, MajorPads, MinorPads, DrumPads };

constexpr unsigned pack_pad_map(int layout, int root, int stride) {
  return (unsigned)layout | ((unsigned)root << 3) | ((unsigned)stride << 10);
}
inline int map_layout(unsigned map) { return map & 7u; }
inline int map_root(unsigned map) { return (map >> 3) & 127u; }
inline int map_stride(unsigned map) { return (map >> 10) & 15u; }
inline bool valid_chromatic_map(int root, int stride) {
  return root >= 0 && stride >= 4 && stride <= 12 && root + 3 * stride + 3 <= 127;
}

inline int pad_note(int layout, int root, int pad, int stride = 4) {
  if (pad < 0 || pad >= 16) return -1;
  static const int major[7] = {0, 2, 4, 5, 7, 9, 11};
  static const int minor[7] = {0, 2, 3, 5, 7, 8, 10};
  // Explicit alternative for MPC drum note assignments. Plugin tracks normally
  // use their Pad Perform layout instead; this map is never inferred from notes.
  static const int drum[16] = {37, 36, 42, 82, 40, 38, 46, 44,
                              48, 47, 45, 43, 49, 55, 51, 53};
  const int degree = (pad / 4) * stride + pad % 4;
  int note = -1;
  switch (layout) {
    case LegacyPads: note = 36 + pad; break;
    case ChromaticPads: note = root + degree; break;
    case MajorPads: note = root + (degree / 7) * 12 + major[degree % 7]; break;
    case MinorPads: note = root + (degree / 7) * 12 + minor[degree % 7]; break;
    case DrumPads: note = drum[pad]; break;
    default: return -1;
  }
  return note >= 0 && note < 128 ? note : -1;
}

inline int note_to_pad(int layout, int root, int note, int stride = 4) {
  if (note < 0 || note >= 128) return -1;
  for (int pad = 0; pad < 16; ++pad)
    if (pad_note(layout, root, pad, stride) == note) return pad;
  return -1;
}
}
