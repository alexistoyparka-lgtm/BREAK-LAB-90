#include "../src/amiga_dsp.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

int main() {
    // Synthetic break: isolated attacks separated by 80 ms.
    const int sr=48000, n=sr*2;
    std::vector<float> x(n,0.0f);
    for(int p=4000;p<n-256;p+=4000) {
        for(int j=0;j<48;j++) x[p+j]=(j&1)?0.8f:-0.8f;
    }
    bl90::Onset events[64];
    const int found=bl90::detect_onsets(x.data(),n,sr,0.55f,24,events,64);
    if(found<4){std::fprintf(stderr,"transient detector found only %d attacks\n",found);return 1;}
    for(int i=1;i<found;i++) if(events[i].frame<=events[i-1].frame) return 2;
    for(int i=0;i<found;i++) if(events[i].frame<0||events[i].frame>=n) return 3;

    // Paula quantisation is bounded and has fewer output levels than clean audio.
    const float q=bl90::paula_quantize(0.12345f,8);
    if(!std::isfinite(q)||q < -1.0f||q > 1.0f) return 4;
    if(bl90::paula_quantize(0.5f,16)==bl90::paula_quantize(0.5f,4)) return 5;

    // Cubic interpolation must pass exactly through the centre samples at t=0 and t=1.
    if(std::fabs(bl90::cubic4(-1.0f,0.25f,0.75f,1.0f,0.0f)-0.25f)>1e-6f) return 6;
    if(std::fabs(bl90::cubic4(-1.0f,0.25f,0.75f,1.0f,1.0f)-0.75f)>1e-6f) return 7;
    std::puts("Amiga DSP tests passed");
    return 0;
}
