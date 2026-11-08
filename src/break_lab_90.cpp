// BREAK LAB 90 - first engine prototype for OpenMPC's generic MPC VST2 wrapper.
// Supports PCM WAV (16-bit mono/stereo, 44.1 kHz recommended), 16 MIDI-triggered slices,
// pitch, reverse, bit-depth reduction, sample-rate reduction and a one-pole low-pass.
// Place a user-owned WAV at: <plugin data folder>/breaks/break01.wav
extern "C" {
#include "engine.h"
}
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

namespace {
constexpr double PI = 3.14159265358979323846;
struct Sample { std::vector<float> l, r; int channels=0; int sampleRate=44100; };
struct Voice { bool on=false; double pos=0; int end=0; float gain=1; int slice=0; };
struct Engine {
  Sample sample;
  Voice voices[8];
  float slice=0, pitch=0, reverse=0, mode=0, bits=16, rate=0, cutoff=1, volume=.85f;
  float lpL=0, lpR=0;
  int heldSlice=0;
  int rateCounter=0;
  float heldL=0, heldR=0;
  std::string status="NO WAV: add breaks/break01.wav";
};
static uint16_t u16(const unsigned char*p){return (uint16_t)(p[0]|(p[1]<<8));}
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool load_wav(const char* path, Sample& s) {
  FILE*f=std::fopen(path,"rb"); if(!f)return false;
  if(std::fseek(f,0,SEEK_END)!=0){std::fclose(f);return false;}
  long fileSize=std::ftell(f);
  if(fileSize<12||std::fseek(f,0,SEEK_SET)!=0){std::fclose(f);return false;}
  unsigned char h[12]; if(std::fread(h,1,12,f)!=12||std::memcmp(h,"RIFF",4)||std::memcmp(h+8,"WAVE",4)){std::fclose(f);return false;}
  int fmt=0,ch=0,bits=0,sampleRate=44100,blockAlign=0; uint32_t dataBytes=0; long dataAt=0; unsigned char c[8];
  while(std::fread(c,1,8,f)==8){
    uint32_t n=u32(c+4); long at=std::ftell(f);
    if(at<0||(long long)n>(long long)fileSize-at)break;
    if(!std::memcmp(c,"fmt ",4)){
      if(n<16||n>4096)break;
      unsigned char b[16]; if(std::fread(b,1,16,f)!=16)break;
      fmt=u16(b);ch=u16(b+2);sampleRate=(int)u32(b+4);blockAlign=u16(b+12);bits=u16(b+14);
    } else if(!std::memcmp(c,"data",4)){dataBytes=n;dataAt=at;break;}
    if(std::fseek(f,at+(long)n+(n&1),SEEK_SET)!=0)break;
  }
  if(fmt!=1||(ch!=1&&ch!=2)||bits!=16||blockAlign!=ch*2||
     sampleRate<8000||sampleRate>192000||!dataAt||dataBytes<4||
     dataBytes%(2u*ch)!=0){std::fclose(f);return false;}
  const size_t frames=dataBytes/(2*ch);
  // Two decoded float channels: cap resident sample memory at 64 MiB on MPC Gen1.
  if(frames<16||frames>8u*1024u*1024u){std::fclose(f);return false;}
  if(std::fseek(f,dataAt,SEEK_SET)!=0){std::fclose(f);return false;}
  s.l.resize(frames);s.r.resize(frames);s.channels=ch;s.sampleRate=sampleRate;
  unsigned char b[65536]; size_t frame=0;
  while(frame<frames){
    size_t count=std::min((sizeof(b)/(2u*ch)),frames-frame);
    size_t bytes=count*2u*ch;
    if(std::fread(b,1,bytes,f)!=bytes){std::fclose(f);s.l.clear();s.r.clear();return false;}
    for(size_t i=0;i<count;i++){
      float a=(int16_t)u16(&b[i*ch*2])/32768.0f;
      float d=ch==2?(int16_t)u16(&b[i*4+2])/32768.0f:a;
      s.l[frame+i]=a;s.r[frame+i]=d;
    }
    frame+=count;
  }
  std::fclose(f);return true;
}
static void start_voice(Engine*e,int sl,float vel){
  if(e->sample.l.empty()) return;
  sl=std::max(0,std::min(15,sl));
  int total=(int)e->sample.l.size(), a=(int)((long long)total*sl/16), b=(int)((long long)total*(sl+1)/16);
  if(b-a<2)return;
  Voice*v=nullptr;for(auto&x:e->voices)if(!x.on){v=&x;break;}if(!v)v=&e->voices[0];
  v->on=true;v->slice=sl;v->gain=std::max(0.f,std::min(1.f,vel));v->end=b;
  v->pos=e->reverse>.5f?(b-1):a;
}
static float quantize(float x,int bits){bits=std::max(4,std::min(16,bits));float levels=(float)((1u<<(bits-1))-1u);return std::round(x*levels)/levels;}
}

static void* create_engine(const char* data_dir){
  Engine*e=new Engine();
  if(data_dir){std::string p(data_dir);if(!p.empty()&&p.back()!='/')p+='/';p+="breaks/break01.wav";if(load_wav(p.c_str(),e->sample))e->status="break01.wav READY";}
  return e;
}
static void destroy_engine(void*p){delete (Engine*)p;}
static void midi_engine(void*p,const uint8_t*m,int n){if(!p||!m||n<3)return;Engine*e=(Engine*)p;int st=m[0]&0xf0,note=m[1],vel=m[2];if(st==0x90&&vel>0&&note>=36&&note<=51){int sl=note-36;e->heldSlice=sl;start_voice(e,sl,vel/127.f);}else if(st==0x90&&vel>0&&note==60){int sl=std::max(0,std::min(15,(int)std::round(e->slice)));e->heldSlice=sl;start_voice(e,sl,vel/127.f);}else if((st==0x90&&vel==0)||st==0x80){/* one-shot slices: note-off deliberately does not choke */}}
static void set_param_engine(void*p,const char*k,const char*v){if(!p||!k||!v)return;Engine*e=(Engine*)p;float x=(float)std::atof(v);
#define SET(name,field,lo,hi) if(!std::strcmp(k,name)){e->field=std::max((float)(lo),std::min((float)(hi),x));return;}
 SET("slice",slice,0,15) SET("pitch",pitch,-24,24) SET("reverse",reverse,0,1) SET("mode",mode,0,1) SET("bits",bits,4,16) SET("rate",rate,0,1) SET("cutoff",cutoff,0,1) SET("volume",volume,0,1)
#undef SET
}
static int get_param_engine(void*p,const char*k,char*b,int n){if(!p||!k||!b||n<2)return 0;Engine*e=(Engine*)p;float x=0;
#define GET(name,field) if(!std::strcmp(k,name)){x=e->field;std::snprintf(b,n,"%.3f",x);return 1;}
 GET("slice",slice) GET("pitch",pitch) GET("reverse",reverse) GET("mode",mode) GET("bits",bits) GET("rate",rate) GET("cutoff",cutoff) GET("volume",volume)
#undef GET
 if(!std::strcmp(k,"status")){std::snprintf(b,n,"%s",e->status.c_str());return 1;}return 0;}
static void render_engine(void*p,int16_t*out,int frames){Engine*e=(Engine*)p;if(!e||!out)return;float cutoffHz=150.0f+e->cutoff*18000.0f;float alpha=1.0f-std::exp(-2.0f*(float)PI*cutoffHz/44100.0f);int bits=(int)std::round(e->bits);int hold=e->mode>.5f?1+(int)(e->rate*23.0f):1;double pitchStep=std::pow(2.0,(e->pitch/12.0))*(e->sample.sampleRate/44100.0);double direction=e->reverse>.5f?-1.0:1.0;
 for(int i=0;i<frames;i++){float l=0,r=0;for(auto&v:e->voices){if(!v.on)continue;int ix=(int)v.pos;if(ix<0||ix>=v.end||ix>=(int)e->sample.l.size()){v.on=false;continue;}int lo=(int)((long long)e->sample.l.size()*v.slice/16),hi=v.end;double step=pitchStep*direction;int ix1=step>0?std::min(ix+1,hi-1):std::max(ix-1,lo);float frac=(float)std::fabs(v.pos-ix);float sl=e->sample.l[ix]+(e->sample.l[ix1]-e->sample.l[ix])*frac;float sr=e->sample.r[ix]+(e->sample.r[ix1]-e->sample.r[ix])*frac;l+=sl*v.gain;r+=sr*v.gain;v.pos+=step;if((step>0&&v.pos>=v.end)||(step<0&&v.pos<lo))v.on=false;}
 if(e->rateCounter==0){e->heldL=l;e->heldR=r;}if(++e->rateCounter>=hold)e->rateCounter=0;if(e->mode>.5f){l=quantize(e->heldL,bits);r=quantize(e->heldR,bits);}else{l=e->heldL;r=e->heldR;}
 e->lpL+=alpha*(l-e->lpL);e->lpR+=alpha*(r-e->lpR);l=e->lpL*e->volume;r=e->lpR*e->volume;
 l=std::max(-1.f,std::min(1.f,l));r=std::max(-1.f,std::min(1.f,r));out[i*2]=(int16_t)(l*32767.f);out[i*2+1]=(int16_t)(r*32767.f);
 }
}
static const mpc_engine_t ENGINE={create_engine,destroy_engine,midi_engine,set_param_engine,get_param_engine,render_engine,nullptr,nullptr};
extern "C" const mpc_engine_t* mpc_engine(void){return &ENGINE;}
