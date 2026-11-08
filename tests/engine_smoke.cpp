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

static bool make_wav(const std::string &path, uint32_t sample_rate = 48000) {
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return false;
    const uint32_t frames = 1600, data_size = frames * 4;
    fwrite("RIFF", 1, 4, f); put32(f, 36 + data_size); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put32(f, 16); put16(f, 1); put16(f, 2);
    put32(f, sample_rate); put32(f, sample_rate * 4); put16(f, 4); put16(f, 16);
    fwrite("data", 1, 4, f); put32(f, data_size);
    for (uint32_t i = 0; i < frames; ++i) {
        int16_t v = (i & 1) ? 7000 : -7000;
        put16(f, (uint16_t)v); put16(f, (uint16_t)v);
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

    api->set_param(engine, "reverse", "1");
    uint8_t reverse_note[3] = {0x90, 51, 100};
    int16_t reverse_out[128 * 2] = {};
    api->midi(engine, reverse_note, 3);
    api->render(engine, reverse_out, 128);
    long reverse_energy = 0;
    for (int i = 0; i < 256; ++i) reverse_energy += (long)reverse_out[i] * reverse_out[i];
    if (!reverse_energy) { fprintf(stderr, "reverse playback produced silence\n"); api->destroy(engine); return 1; }

    api->set_param(engine, "slice", "7");
    uint8_t selected_note[3] = {0x90, 60, 100};
    int16_t selected_out[128 * 2] = {};
    api->midi(engine, selected_note, 3);
    api->render(engine, selected_out, 128);
    long selected_energy = 0;
    for (int i = 0; i < 256; ++i) selected_energy += (long)selected_out[i] * selected_out[i];
    if (!selected_energy) { fprintf(stderr, "selected-slice audition produced silence\n"); api->destroy(engine); return 1; }

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
    puts("PASS: bundled break loading, 48 kHz WAV loading, click-safe fade-in, all 16 MIDI slices, selected-slice audition, reverse playback and malformed WAV rejection");
    return 0;
}
