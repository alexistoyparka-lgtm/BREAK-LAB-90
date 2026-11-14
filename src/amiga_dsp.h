#pragma once
// BREAK LAB 90 — compact ARMv7-friendly DSP primitives.
// No allocation, locks, I/O or dynamic dispatch in the playback functions.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace bl90 {

struct Onset { int frame; float strength; };

// Multi-resolution transient detector. Caller supplies mono float samples.
// Returns number of onsets written, bounded by capacity. Analysis is offline.
inline int detect_onsets(const float* x, int n, int sampleRate,
                         float sensitivity, int minGapMs,
                         Onset* out, int capacity) {
    if (!x || !out || n < 128 || sampleRate < 8000 || capacity <= 0) return 0;
    const int hop = 64;
    const int win = 256;
    const int minGap = std::max(hop, sampleRate * std::max(10, minGapMs) / 1000);
    const float sens = std::max(0.0f, std::min(1.0f, sensitivity));
    const int count = (n - win) / hop;
    if (count < 3) return 0;

    // Positive spectral-flux proxy: high-passed energy rise + broadband RMS rise.
    // It is deliberately light enough for a background analysis pass on ARMv7.
    float prevLow = 0.0f, prevHigh = 0.0f;
    float mean = 1.0e-7f, variance = 1.0e-8f;
    float strengths[4096];
    int frames[4096];
    const int usable = std::min(count, 4096);
    for (int k = 0; k < usable; ++k) {
        const int base = k * hop;
        double low = 0.0, high = 0.0;
        float last = x[base];
        for (int j = 0; j < win; j += 4) {
            const float v = x[base + j];
            const float d = v - last;
            low += (double)v * v;
            high += (double)d * d;
            last = v;
        }
        const float lo = (float)(low / (win / 4));
        const float hi = (float)(high / (win / 4));
        const float flux = std::max(0.0f, hi - prevHigh) * 0.7f +
                           std::max(0.0f, lo - prevLow) * 0.3f;
        const float threshold = mean + (0.8f - 0.62f * sens) * std::sqrt(std::max(variance, 1.0e-10f));
        strengths[k] = flux > threshold ? flux - threshold : 0.0f;
        frames[k] = base + win / 2;
        const float delta = flux - mean;
        mean += 0.035f * delta;
        variance = 0.965f * variance + 0.035f * delta * delta;
        prevLow = lo;
        prevHigh = hi;
    }

    int found = 0, lastFrame = -minGap;
    for (int k = 1; k + 1 < usable; ++k) {
        const float s = strengths[k];
        if (s <= 0.0f || s < strengths[k - 1] || s <= strengths[k + 1]) continue;
        const int frame = frames[k];
        if (frame - lastFrame < minGap) continue;
        // Keep strongest candidates if caller capacity is reached.
        if (found < capacity) {
            out[found++] = {frame, s};
            lastFrame = frame;
        } else {
            int weakest = 0;
            for (int q = 1; q < found; ++q) if (out[q].strength < out[weakest].strength) weakest = q;
            if (s > out[weakest].strength) out[weakest] = {frame, s};
        }
    }
    // Sort retained onsets by sample position.
    for (int i = 1; i < found; ++i) {
        Onset v = out[i]; int j = i;
        while (j > 0 && out[j - 1].frame > v.frame) { out[j] = out[j - 1]; --j; }
        out[j] = v;
    }
    return found;
}

inline float cubic4(float a, float b, float c, float d, float t) {
    const float p = (d - c) - (a - b);
    const float q = (a - b) - p;
    const float r = c - a;
    return ((p * t + q) * t + r) * t + b;
}

inline float paula_quantize(float x, int bits) {
    bits = std::max(4, std::min(16, bits));
    const float levels = (float)((1u << (bits - 1)) - 1u);
    return std::round(std::max(-1.0f, std::min(1.0f, x)) * levels) / levels;
}

} // namespace bl90
