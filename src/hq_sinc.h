#pragma once
// BREAK LAB 90 — bounded polyphase windowed-sinc sample interpolation.
// Table coefficients are built once outside the render callback. No heap
// allocations, transcendental functions, or locks in sinc16_sample().
#include <cmath>
#include <algorithm>

namespace bl90 {

// 16 taps, 512 fractional phases, ten conservative low-pass bandwidths.
// Cutoffs are normalised to source Nyquist.  The chosen band NEVER exceeds
// 1 / source_frames_per_output_frame when downsampling/pitching up.
struct Sinc16Bank {
    static constexpr int taps = 16;
    static constexpr int phases = 512;
    static constexpr int bands = 10;
    float kernels[bands][phases][taps];
    float cutoff[bands];

    Sinc16Bank() : cutoff{1.0f, .875f, .75f, .625f, .5f, .375f,
                          .25f, .125f, .0625f, .03125f} {
        constexpr double pi = 3.1415926535897932384626433832795;
        for (int b = 0; b < bands; ++b) {
            const double fc = cutoff[b];
            for (int p = 0; p < phases; ++p) {
                const double phase = (double)p / phases;
                double sum = 0;
                for (int t = 0; t < taps; ++t) {
                    const double x = (t - 7) - phase;
                    const double z = pi * fc * x;
                    const double sinc = std::fabs(z) < 1.e-12 ? 1.0 : std::sin(z) / z;
                    const double window = .5 + .5 * std::cos(pi * x / 8.);
                    const double value = fc * sinc * window;
                    kernels[b][p][t] = (float)value;
                    sum += value;
                }
                if (std::fabs(sum) > 1.e-12)
                    for (int t = 0; t < taps; ++t)
                        kernels[b][p][t] = (float)(kernels[b][p][t] / sum);
            }
        }
    }

    int band_for_step(double step) const {
        if (!(step > 1.0)) return 0;
        const double maxCutoff = 1.0 / step;
        for (int b = 0; b < bands; ++b)
            if (cutoff[b] <= maxCutoff) return b;
        return bands - 1; // extreme transpositions: best-effort attenuation
    }

    // ix=floor(pos); phase in [0,1). Clamped to this slice only.
    float sample(const float *source, int ix, float phase,
                 int sliceBegin, int sliceEnd, int band) const {
        if (!source || sliceBegin >= sliceEnd) return 0.f;
        const int b = std::max(0, std::min(bands - 1, band));
        const float f = std::max(0.f, std::min(.999999f, phase));
        const int p = std::min(phases - 1, (int)(f * phases + .5f));
        const float *kernel = kernels[b][p];
        float result = 0.f;
        for (int t = 0; t < taps; ++t) {
            const int index = std::max(sliceBegin, std::min(sliceEnd - 1, ix + t - 7));
            result += source[index] * kernel[t];
        }
        return result;
    }
};

inline const Sinc16Bank &sinc16_bank() {
    static const Sinc16Bank bank;
    return bank;
}
} // namespace bl90
