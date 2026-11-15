#include "../src/amiga_dsp.h"
#include "../src/hq_sinc.h"
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
    // HQ Sinc: preserve DC gain across pitch-aware bands and fractional phases.
    const auto &bank = bl90::sinc16_bank();
    std::vector<float> dc(8192,0.35f);
    for(int band=0;band<bl90::Sinc16Bank::bands;band++)
      for(int p=0;p<bl90::Sinc16Bank::phases;p+=37) {
        const float y=bank.sample(dc.data(),2000,(float)p/512.f,0,(int)dc.size(),band);
        if(std::fabs(y-0.35f)>2e-5f) {
          std::fprintf(stderr,"HQ sinc DC error band=%d phase=%d value=%g\\n",band,p,y);
          return 8;
        }
      }

    // High frequencies above the output Nyquist must be attenuated when
    // playing a sample at twice the original rate; bass should be retained.
    std::vector<float> high(8192),low(8192);
    const double pi=3.14159265358979323846;
    for(int i=0;i<8192;i++) {
       high[i]=(float)(.7*std::sin(2*pi*.43*i));
       low[i]=(float)(.7*std::sin(2*pi*.04*i));
    }
    const int downsampleBand=bank.band_for_step(2.0);
    double highSq=0,lowSq=0,highNaiveSq=0;
    for(int n=0;n<1000;n++) {
      const double pos=100.31+2.0*n;
      const int ix=(int)std::floor(pos);
      const float frac=(float)(pos-ix);
      const float hf=bank.sample(high.data(),ix,frac,0,8192,downsampleBand);
      const float lf=bank.sample(low.data(),ix,frac,0,8192,downsampleBand);
      highSq+=hf*hf; lowSq+=lf*lf;
      const float naive=high[ix]+(high[ix+1]-high[ix])*frac;
      highNaiveSq+=naive*naive;
    }
    if(highSq>0.15*highNaiveSq || lowSq < 60.0) {
      std::fprintf(stderr,"HQ sinc alias attenuation / low-pass regression: high=%g naive=%g low=%g\\n",
                   highSq,highNaiveSq,lowSq);
      return 9;
    }

    // The sinc lookup must never read beyond a short slice boundary.
    std::vector<float> isolated(64,0.f);
    for(int i=0;i<64;i++) isolated[i]=i<32?0.0f:1.f;
    for(int p=0;p<512;p+=17) {
      float a=bank.sample(isolated.data(),30,(float)p/512.f,0,32,0);
      if(std::fabs(a)>1.e-5f) return 10;
    }

    std::puts("Amiga DSP tests passed");
    return 0;
}
