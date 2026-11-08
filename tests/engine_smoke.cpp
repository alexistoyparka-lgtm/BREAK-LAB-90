#include "engine.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>

static void put16(FILE *f, uint16_t v) { fputc(v & 255, f); fputc((v >> 8) & 255, f); }
static void put32(FILE *f, uint32_t v) { put16(f, v & 65535); put16(f, v >> 16); }

static bool make_wav(const std::string &path, uint32_t sample_rate = 48000, int channels = 2) {
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return false;
    const uint32_t frames = 1600, data_size = frames * channels * 2;
    fwrite("RIFF", 1, 4, f); put32(f, 36 + data_size); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put32(f, 16); put16(f, 1); put16(f, channels);
    put32(f, sample_rate); put32(f, sample_rate * channels * 2); put16(f, channels * 2); put16(f, 16);
    fwrite("data", 1, 4, f); put32(f, data_size);
    for (uint32_t i = 0; i < frames; ++i) {
        int16_t amp = (int16_t)(1000 + (i / (frames / 16)) * 400);
        int16_t v = (int16_t)(amp + (int)(i % (frames / 16)) * 8);
        put16(f, (uint16_t)v); if (channels == 2) put16(f, (uint16_t)v);
    }
    fclose(f);
    return true;
}

static bool make_malformed_wav(const std::string &path) {
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return false;
    fwrite("RIFF", 1, 4, f); put32(f, 40); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put32(f, 0xffffffffu);
    fclose(f);
    return true;
}

static bool make_bad_byte_rate_wav(const std::string &path) {
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return false;
    fwrite("RIFF", 1, 4, f); put32(f, 36 + 64); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put32(f, 16); put16(f, 1); put16(f, 2);
    put32(f, 44100); put32(f, 123); put16(f, 4); put16(f, 16);
    fwrite("data", 1, 4, f); put32(f, 64);
    for (int i = 0; i < 16; ++i) { put16(f, 1000); put16(f, 1000); }
    fclose(f);
    return true;
}

int main(int argc, char **argv) {
    char root[] = "/tmp/break-lab-90-test-XXXXXX";
    char *dir = mkdtemp(root);
    if (!dir) { perror("mkdtemp"); return 1; }
    std::string breaks = std::string(dir) + "/breaks";
    if (mkdir(breaks.c_str(), 0700) != 0) { perror("mkdir"); return 1; }
    if (!make_wav(breaks + "/break01.wav")) { fprintf(stderr, "could not create WAV\n"); return 1; }

    const mpc_engine_t *api = mpc_engine();
    if (argc > 1) {
        void *bundled = api->create(argv[1]);
        char bundled_status[128] = {};
        if (!bundled || !api->get_param(bundled, "status", bundled_status, sizeof(bundled_status)) ||
            strcmp(bundled_status, "break01.wav READY")) {
            fprintf(stderr, "bundled break failed to load: %s\n", bundled_status);
            if (bundled) api->destroy(bundled);
            return 1;
        }
        api->destroy(bundled);
    }
    void *engine = api->create(dir);
    if (!engine) { fprintf(stderr, "engine creation failed\n"); return 1; }
    char status[128] = {};
    if (!api->get_param(engine, "status", status, sizeof(status)) || strcmp(status, "break01.wav READY")) {
        fprintf(stderr, "WAV load failed: %s\n", status); return 1;
    }

    // Capture the same source material at 48 kHz for a later 44.1 kHz comparison.
    uint8_t sample_rate_note[3] = {0x90, 36, 100};
    int16_t sample_rate_48k_out[128 * 2] = {};
    api->midi(engine, sample_rate_note, 3);
    api->render(engine, sample_rate_48k_out, 128);

    // A fresh voice must fade in from silence to avoid a hard slice-edge click.
    void *fade_engine = api->create(dir);
    if (!fade_engine) return 1;
    uint8_t fade_note[3] = {0x90, 36, 100};
    int16_t fade_first[2] = {1, 1};
    api->midi(fade_engine, fade_note, 3);
    api->render(fade_engine, fade_first, 1);
    if (fade_first[0] != 0 || fade_first[1] != 0) {
        fprintf(stderr, "slice fade-in did not start at silence\n");
        api->destroy(fade_engine); return 1;
    }
    int16_t fade_tail[128 * 2] = {};
    api->render(fade_engine, fade_tail, 128);
    long fade_energy = 0;
    for (int i = 0; i < 256; ++i) fade_energy += (long)fade_tail[i] * fade_tail[i];
    api->destroy(fade_engine);
    if (!fade_energy) { fprintf(stderr, "slice fade-in muted the sample\n"); return 1; }

    for (int note = 36; note <= 51; ++note) {
        uint8_t midi[3] = {0x90, (uint8_t)note, 100};
        int16_t out[128 * 2] = {};
        api->midi(engine, midi, 3);
        api->render(engine, out, 128);
        long energy = 0;
        for (int i = 0; i < 256; ++i) energy += (long)out[i] * out[i];
        if (energy == 0) {
            fprintf(stderr, "slice note %d produced silence\n", note);
            api->destroy(engine); return 1;
        }
    }

    // Parameter round-trip and clamping are part of the saved-project contract.
    api->set_param(engine, "pitch", "12");
    char pitch_value[32] = {};
    if (!api->get_param(engine, "pitch", pitch_value, sizeof(pitch_value)) || strcmp(pitch_value, "12.000")) {
        fprintf(stderr, "pitch parameter round-trip failed: %s\n", pitch_value);
        api->destroy(engine); return 1;
    }
    api->set_param(engine, "pitch", "99");
    memset(pitch_value, 0, sizeof(pitch_value));
    api->get_param(engine, "pitch", pitch_value, sizeof(pitch_value));
    if (strcmp(pitch_value, "24.000")) {
        fprintf(stderr, "pitch parameter was not clamped: %s\n", pitch_value);
        api->destroy(engine); return 1;
    }
    api->set_param(engine, "pitch", "0");

    // Pitch must change the actual playback rate, not only the displayed parameter.
    void *pitch_normal = api->create(dir);
    void *pitch_up = api->create(dir);
    if (!pitch_normal || !pitch_up) return 1;
    api->set_param(pitch_up, "pitch", "12");
    uint8_t pitch_note[3] = {0x90, 36, 100};
    int16_t normal_out[128 * 2] = {}, pitch_out[128 * 2] = {};
    api->midi(pitch_normal, pitch_note, 3); api->render(pitch_normal, normal_out, 128);
    api->midi(pitch_up, pitch_note, 3); api->render(pitch_up, pitch_out, 128);
    long pitch_difference = 0;
    for (int i = 0; i < 256; ++i) pitch_difference += abs((int)normal_out[i] - (int)pitch_out[i]);
    api->destroy(pitch_normal); api->destroy(pitch_up);
    if (pitch_difference < 1000) { fprintf(stderr, "pitch control did not alter playback rate\n"); return 1; }

    // MIDI velocity must scale the playback amplitude rather than being ignored.
    void *vel_full = api->create(dir);
    void *vel_half = api->create(dir);
    uint8_t full_note[3] = {0x90, 36, 127};
    uint8_t half_note[3] = {0x90, 36, 64};
    int16_t full_out[128 * 2] = {}, half_out[128 * 2] = {};
    api->midi(vel_full, full_note, 3); api->render(vel_full, full_out, 128);
    api->midi(vel_half, half_note, 3); api->render(vel_half, half_out, 128);
    long full_energy = 0, half_energy = 0;
    for (int i = 0; i < 256; ++i) {
        full_energy += (long)full_out[i] * full_out[i];
        half_energy += (long)half_out[i] * half_out[i];
    }
    api->destroy(vel_full); api->destroy(vel_half);
    double velocity_ratio = full_energy ? (double)half_energy / full_energy : 0.0;
    if (velocity_ratio < 0.20 || velocity_ratio > 0.32) {
        fprintf(stderr, "MIDI velocity scaling is incorrect: ratio %.3f\n", velocity_ratio);
        api->destroy(engine); return 1;
    }

    // Vintage mode must apply bit-depth reduction to the rendered signal.
    void *clean_engine = api->create(dir);
    void *vintage_engine = api->create(dir);
    if (!clean_engine || !vintage_engine) return 1;
    api->set_param(vintage_engine, "mode", "1");
    api->set_param(vintage_engine, "bits", "4");
    uint8_t character_note[3] = {0x90, 36, 100};
    int16_t clean_out[128 * 2] = {}, vintage_out[128 * 2] = {};
    api->midi(clean_engine, character_note, 3);
    api->midi(vintage_engine, character_note, 3);
    api->render(clean_engine, clean_out, 128);
    api->render(vintage_engine, vintage_out, 128);
    long character_difference = 0;
    for (int i = 0; i < 256; ++i) character_difference += abs((int)clean_out[i] - (int)vintage_out[i]);
    api->destroy(clean_engine);
    api->destroy(vintage_engine);
    if (character_difference < 1000) {
        fprintf(stderr, "Vintage bit-depth reduction did not change the signal\n");
        api->destroy(engine); return 1;
    }

    // Rate Crush must hold samples for multiple output frames in Vintage mode.
    clean_engine = api->create(dir);
    vintage_engine = api->create(dir);
    if (!clean_engine || !vintage_engine) return 1;
    api->set_param(vintage_engine, "mode", "1");
    api->set_param(vintage_engine, "bits", "16");
    api->set_param(vintage_engine, "rate", "1");
    memset(clean_out, 0, sizeof(clean_out));
    memset(vintage_out, 0, sizeof(vintage_out));
    api->midi(clean_engine, character_note, 3);
    api->midi(vintage_engine, character_note, 3);
    api->render(clean_engine, clean_out, 128);
    api->render(vintage_engine, vintage_out, 128);
    long rate_difference = 0;
    for (int i = 0; i < 256; ++i) rate_difference += abs((int)clean_out[i] - (int)vintage_out[i]);
    api->destroy(clean_engine);
    api->destroy(vintage_engine);
    if (rate_difference < 1000) {
        fprintf(stderr, "Rate Crush did not change the sample-hold output\n");
        api->destroy(engine); return 1;
    }

    api->set_param(engine, "reverse", "1");
    uint8_t reverse_note[3] = {0x90, 51, 100};
    int16_t reverse_out[128 * 2] = {};
    api->midi(engine, reverse_note, 3);
    api->render(engine, reverse_out, 128);
    long reverse_energy = 0;
    for (int i = 0; i < 256; ++i) reverse_energy += (long)reverse_out[i] * reverse_out[i];
    if (!reverse_energy) { fprintf(stderr, "reverse playback produced silence\n"); api->destroy(engine); return 1; }

    // Reverse must change the actual sample order, not merely keep the voice audible.
    void *forward_engine = api->create(dir);
    void *backward_engine = api->create(dir);
    if (!forward_engine || !backward_engine) return 1;
    api->set_param(backward_engine, "reverse", "1");
    uint8_t direction_note[3] = {0x90, 51, 100};
    int16_t forward_out[32 * 2] = {}, backward_out[32 * 2] = {};
    api->midi(forward_engine, direction_note, 3);
    api->midi(backward_engine, direction_note, 3);
    api->render(forward_engine, forward_out, 32);
    api->render(backward_engine, backward_out, 32);
    long direction_difference = 0;
    for (int i = 0; i < 64; ++i) direction_difference += abs((int)forward_out[i] - (int)backward_out[i]);
    api->destroy(forward_engine);
    api->destroy(backward_engine);
    if (direction_difference < 1000) {
        fprintf(stderr, "reverse control did not change playback direction\n");
        api->destroy(engine); return 1;
    }

    api->destroy(engine);

    // C4 must audition the selected slice, not always slice 1.
    void *selected_engine = api->create(dir);
    void *first_engine = api->create(dir);
    if (!selected_engine || !first_engine) return 1;
    api->set_param(selected_engine, "slice", "7");
    uint8_t selected_note[3] = {0x90, 60, 100};
    uint8_t first_note[3] = {0x90, 36, 100};
    int16_t selected_out[128 * 2] = {}, first_out[128 * 2] = {};
    api->midi(selected_engine, selected_note, 3); api->render(selected_engine, selected_out, 128);
    api->midi(first_engine, first_note, 3); api->render(first_engine, first_out, 128);
    long selected_energy = 0, first_energy = 0;
    for (int i = 0; i < 256; ++i) {
        selected_energy += (long)selected_out[i] * selected_out[i];
        first_energy += (long)first_out[i] * first_out[i];
    }
    api->destroy(selected_engine); api->destroy(first_engine);
    if (!first_energy || selected_energy <= first_energy * 4) {
        fprintf(stderr, "C4 did not audition the selected slice: selected=%ld first=%ld\n", selected_energy, first_energy);
        return 1;
    }

    // Mono PCM at 44.1 kHz is also a supported source format.
    if (!make_wav(breaks + "/break01.wav", 44100, 1)) return 1;
    engine = api->create(dir);
    if (!engine) return 1;
    memset(status, 0, sizeof(status));
    if (!api->get_param(engine, "status", status, sizeof(status)) || strcmp(status, "break01.wav READY")) {
        fprintf(stderr, "mono WAV load failed: %s\n", status);
        api->destroy(engine); return 1;
    }
    uint8_t mono_note[3] = {0x90, 36, 100};
    int16_t mono_out[128 * 2] = {};
    api->midi(engine, mono_note, 3); api->render(engine, mono_out, 128);
    long mono_energy = 0;
    for (int i = 0; i < 256; ++i) mono_energy += (long)mono_out[i] * mono_out[i];
    api->destroy(engine);
    if (!mono_energy) { fprintf(stderr, "mono WAV playback produced silence\n"); return 1; }

    // The loader must honor the source sample rate, not play every WAV at 44.1 kHz.
    long sample_rate_difference = 0;
    for (int i = 0; i < 128; ++i)
        sample_rate_difference += abs((int)sample_rate_48k_out[i * 2] - (int)mono_out[i * 2]);
    if (sample_rate_difference < 5000) {
        fprintf(stderr, "48 kHz source rate was not reflected in playback step: %ld\n", sample_rate_difference);
        return 1;
    }

    // A PCM file with an inconsistent byte rate must be rejected.
    if (!make_bad_byte_rate_wav(breaks + "/break01.wav")) return 1;
    engine = api->create(dir);
    if (!engine) return 1;
    memset(status, 0, sizeof(status));
    if (!api->get_param(engine, "status", status, sizeof(status)) ||
        strcmp(status, "NO WAV: add breaks/break01.wav")) {
        fprintf(stderr, "invalid byte-rate WAV was not rejected: %s\n", status);
        api->destroy(engine); return 1;
    }
    api->destroy(engine);

    // Oversized/truncated chunk lengths must be rejected safely, not allocated.
    if (!make_malformed_wav(breaks + "/break01.wav")) return 1;
    engine = api->create(dir);
    if (!engine) return 1;
    memset(status, 0, sizeof(status));
    if (!api->get_param(engine, "status", status, sizeof(status)) ||
        strcmp(status, "NO WAV: add breaks/break01.wav")) {
        fprintf(stderr, "malformed WAV was not rejected: %s\n", status);
        api->destroy(engine); return 1;
    }
    api->destroy(engine);

    unlink((breaks + "/break01.wav").c_str());
    rmdir(breaks.c_str());
    rmdir(dir);
    puts("PASS: bundled break loading, 48 kHz stereo and 44.1 kHz mono WAV loading and sample-rate conversion, click-safe fade-in, all 16 MIDI slices, velocity scaling, Vintage bit-depth/rate crushing, pitch playback/clamping, selected-slice audition, reverse playback direction and malformed RIFF/PCM metadata rejection");
    return 0;
}
