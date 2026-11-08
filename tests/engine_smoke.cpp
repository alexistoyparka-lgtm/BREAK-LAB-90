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

static bool make_wav(const std::string &path) {
    FILE *f = fopen(path.c_str(), "wb");
    if (!f) return false;
    const uint32_t frames = 1600, data_size = frames * 4;
    fwrite("RIFF", 1, 4, f); put32(f, 36 + data_size); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put32(f, 16); put16(f, 1); put16(f, 2);
    put32(f, 44100); put32(f, 44100 * 4); put16(f, 4); put16(f, 16);
    fwrite("data", 1, 4, f); put32(f, data_size);
    for (uint32_t i = 0; i < frames; ++i) {
        int16_t v = (i & 1) ? 7000 : -7000;
        put16(f, (uint16_t)v); put16(f, (uint16_t)v);
    }
    fclose(f);
    return true;
}

int main() {
    char root[] = "/tmp/break-lab-90-test-XXXXXX";
    char *dir = mkdtemp(root);
    if (!dir) { perror("mkdtemp"); return 1; }
    std::string breaks = std::string(dir) + "/breaks";
    if (mkdir(breaks.c_str(), 0700) != 0) { perror("mkdir"); return 1; }
    if (!make_wav(breaks + "/break01.wav")) { fprintf(stderr, "could not create WAV\n"); return 1; }

    const mpc_engine_t *api = mpc_engine();
    void *engine = api->create(dir);
    if (!engine) { fprintf(stderr, "engine creation failed\n"); return 1; }
    char status[128] = {};
    if (!api->get_param(engine, "status", status, sizeof(status)) || strcmp(status, "break01.wav READY")) {
        fprintf(stderr, "WAV load failed: %s\n", status); return 1;
    }

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

    api->set_param(engine, "reverse", "1");
    uint8_t reverse_note[3] = {0x90, 51, 100};
    int16_t reverse_out[128 * 2] = {};
    api->midi(engine, reverse_note, 3);
    api->render(engine, reverse_out, 128);
    long reverse_energy = 0;
    for (int i = 0; i < 256; ++i) reverse_energy += (long)reverse_out[i] * reverse_out[i];
    if (!reverse_energy) { fprintf(stderr, "reverse playback produced silence\n"); api->destroy(engine); return 1; }

    api->destroy(engine);
    unlink((breaks + "/break01.wav").c_str());
    rmdir(breaks.c_str());
    rmdir(dir);
    puts("PASS: WAV loading, all 16 MIDI slices and reverse playback");
    return 0;
}
