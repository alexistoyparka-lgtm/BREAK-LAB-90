// BREAK LAB 90 - first engine prototype for OpenMPC's generic MPC VST2 wrapper.
// Supports PCM WAV (16-bit mono/stereo, 44.1 kHz recommended), 16 MIDI-triggered slices,
// pitch, reverse, bit-depth reduction, sample-rate reduction and a one-pole low-pass.
// Place a user-owned WAV at: <plugin data folder>/breaks/break01.wav
extern "C" {
#include "engine.h"
}
#include "amiga_dsp.h"
#include "pad_mapping.h"
#include "sample_bank.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cctype>

namespace {
constexpr double PI = 3.14159265358979323846;
struct Sample { std::vector<float> l, r; int channels=0; int sampleRate=44100; };
struct BankSlot { Sample sample; int start[16]={}, end[16]={}; };
struct Voice { bool on=false; double pos=0; int begin=0, end=0; int direction=1; int fadeFrames=88; float gain=1; int slice=0; };
struct Engine {
  BankSlot bank[bl90::BANK_COUNT];
  Sample* sample=&bank[0].sample;
  int selectedBank=0;
  Voice voices[8];
  int voiceCursor=0;
  float slice=0, pitch=0, reverse=0, mode=0, bits=16, rate=0, cutoff=1, volume=.85f;
  float lpL=0, lpR=0;
  int heldSlice=0;
  // One atomic snapshot prevents partially applied pad calibration.
  std::atomic<unsigned> padMap{bl90::pack_pad_map(bl90::LegacyPads,36,4)};
  std::atomic<unsigned> padChanged{0};
  std::atomic<int> padLearnStage{0}, padLearnRoot{-1};
  std::atomic<int> lastMidiNote{-1};
  std::atomic<int> sliceStart[16]; std::atomic<int> sliceEnd[16];
  int baseStart[16]={}; int baseEnd[16]={};
  int undoStart[16]={}; int undoEnd[16]={}; int undoSlice=0; bool undoValid=false;
  float interpolation=1;
  int rateCounter=0;
  float heldL=0, heldR=0;
  char status[128]="NO WAV: add breaks/break01.wav";
};
static uint16_t u16(const unsigned char*p){return (uint16_t)(p[0]|(p[1]<<8));}
static uint32_t u32(const unsigned char*p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
static bool load_wav(const char* path, Sample& s, size_t remainingFrames) {
  FILE*f=std::fopen(path,"rb"); if(!f)return false;
  if(std::fseek(f,0,SEEK_END)!=0){std::fclose(f);return false;}
  long fileSize=std::ftell(f);
  if(fileSize<12||std::fseek(f,0,SEEK_SET)!=0){std::fclose(f);return false;}
  unsigned char h[12]; if(std::fread(h,1,12,f)!=12||std::memcmp(h,"RIFF",4)||std::memcmp(h+8,"WAVE",4)){std::fclose(f);return false;}
  uint32_t riffBytes=u32(h+4);
  long long riffEnd64=8LL+(long long)riffBytes;
  if(riffBytes<4||riffEnd64>fileSize||riffEnd64<12){std::fclose(f);return false;}
  long riffEnd=(long)riffEnd64;
  int fmt=0,ch=0,bits=0,sampleRate=44100,blockAlign=0,byteRate=0; uint32_t dataBytes=0; long dataAt=0; unsigned char c[8];
  while(std::ftell(f)>=0&&std::ftell(f)+8<=riffEnd&&std::fread(c,1,8,f)==8){
    uint32_t n=u32(c+4); long at=std::ftell(f);
    if(at<0||(long long)at+(long long)n+(n&1)>riffEnd)break;
    if(!std::memcmp(c,"fmt ",4)){
      if(n<16||n>4096)break;
      unsigned char b[16]; if(std::fread(b,1,16,f)!=16)break;
      fmt=u16(b);ch=u16(b+2);sampleRate=(int)u32(b+4);byteRate=(int)u32(b+8);blockAlign=u16(b+12);bits=u16(b+14);
    } else if(!std::memcmp(c,"data",4)){dataBytes=n;dataAt=at;break;}
    if(std::fseek(f,(long)((long long)at+(long long)n+(n&1)),SEEK_SET)!=0)break;
  }
  if(fmt!=1||(ch!=1&&ch!=2)||bits!=16||blockAlign!=ch*2||
     sampleRate<8000||sampleRate>192000||byteRate!=sampleRate*blockAlign||!dataAt||dataBytes<4||
     dataBytes%(2u*ch)!=0){std::fclose(f);return false;}
  const size_t frames=dataBytes/(2*ch);
  // Two decoded float channels: cap resident sample memory at 64 MiB on MPC Gen1.
  if(frames<16||frames>remainingFrames){std::fclose(f);return false;}
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
  if(e->sample->l.empty()) return;
  sl=std::max(0,std::min(15,sl));
  int total=(int)e->sample->l.size(), a=e->sliceStart[sl].load(), b=e->sliceEnd[sl].load();
  if(b-a<2)return;
  int slot=-1;for(int i=0;i<8;i++)if(!e->voices[i].on){slot=i;break;}if(slot<0)slot=e->voiceCursor;e->voiceCursor=(slot+1)%8;Voice*v=&e->voices[slot];
  v->on=true;v->slice=sl;v->gain=std::max(0.f,std::min(1.f,vel));v->begin=a;v->end=b;v->direction=e->reverse>.5f?-1:1;v->fadeFrames=std::max(8,e->sample->sampleRate/500);v->fadeFrames=std::min(v->fadeFrames,(b-a)/2);
  v->pos=v->direction<0?(b-1):a;
  // Keep the shared sample-hold clock running when a new voice starts.
  // Resetting it here makes every pad retrigger the Vintage/PAULA decimator,
  // producing timing-dependent clicks and changing the character of other voices.
}
static float quantize(float x,int bits){bits=std::max(4,std::min(16,bits));float levels=(float)((1u<<(bits-1))-1u);return std::round(x*levels)/levels;}
}

static void prepare_bank(BankSlot& slot){
  const int total=(int)slot.sample.l.size();
  for(int i=0;i<16;i++){slot.start[i]=(int)((long long)total*i/16);slot.end[i]=(int)((long long)total*(i+1)/16);}
  bl90::Onset onsets[64];
  const int found=bl90::detect_onsets(slot.sample.l.data(),total,slot.sample.sampleRate,0.55f,24,onsets,64);
  if(found>=15){
    int prev=0;bool valid=true;
    for(int i=1;i<16;i++){
      const int pick=((i-1)*(found-1))/14;
      int boundary=onsets[std::min(found-1,pick)].frame;
      boundary=std::max(prev+128,std::min(total-(16-i)*128,boundary));
      if(boundary<=prev){valid=false;break;}
      slot.end[i-1]=boundary;slot.start[i]=boundary;prev=boundary;
    }
    if(valid){slot.start[0]=0;slot.end[15]=total;}
    else for(int i=0;i<16;i++){slot.start[i]=(int)((long long)total*i/16);slot.end[i]=(int)((long long)total*(i+1)/16);}
  }
}
static void silence_voices(Engine*e){
  for(auto&v:e->voices)v.on=false;
  e->voiceCursor=0;e->rateCounter=0;e->heldL=e->heldR=e->lpL=e->lpR=0;
}
static bool select_bank(Engine*e,int index){
  if(index<0||index>=bl90::BANK_COUNT)return false;
  BankSlot& slot=e->bank[index];
  if(slot.sample.l.empty()){
    std::snprintf(e->status,sizeof(e->status),"MISSING: %s",bl90::BANK_NAMES[index]);
    return false;
  }
  if(e->selectedBank!=index||e->sample!=&slot.sample){
    silence_voices(e);
    e->selectedBank=index;e->sample=&slot.sample;e->undoValid=false;
    for(int i=0;i<16;i++){
      e->baseStart[i]=slot.start[i];e->baseEnd[i]=slot.end[i];
      e->sliceStart[i].store(slot.start[i]);e->sliceEnd[i].store(slot.end[i]);
    }
  }
  std::snprintf(e->status,sizeof(e->status),"%s READY",index?bl90::BANK_NAMES[index]:"break01.wav");
  return true;
}
static void* create_engine(const char* data_dir){
  Engine*e=new Engine();
  for(int i=0;i<16;i++){e->sliceStart[i].store(0);e->sliceEnd[i].store(0);}
  if(data_dir){
    std::string folder(data_dir);if(!folder.empty()&&folder.back()!='/')folder+='/';folder+="breaks/";
    size_t used=0;
    for(int bank=0;bank<bl90::BANK_COUNT;bank++){
      BankSlot& slot=e->bank[bank];
      const std::string path=folder+bl90::BANK_FILES[bank];
      // All file I/O, allocation and onset analysis happen only at instance creation.
      // Switching presets is then a pointer swap plus cached boundary stores.
      if(load_wav(path.c_str(),slot.sample,bl90::MAX_DECODED_FRAMES-used)){
        used+=slot.sample.l.size();prepare_bank(slot);
      }else slot.sample=Sample(); // release partial/failed allocations before loading another slot
    }
    // Force applying slot zero's precomputed bounds on the initial selection.
    e->selectedBank=-1;
    if(!select_bank(e,0)){e->selectedBank=0;std::snprintf(e->status,sizeof(e->status),"NO WAV: add breaks/break01.wav");}
  }
  return e;
}
static bool read_state_number(const char*&p,double&value){
  while(std::isspace((unsigned char)*p))++p;
  if(!*p)return false;
  errno=0;char*end=nullptr;value=std::strtod(p,&end);
  if(end==p||errno==ERANGE||!std::isfinite(value))return false;
  p=end;return true;
}
static bool state_int(const char*&p,int&value,int low,int high){
  double x;if(!read_state_number(p,x)||x<low||x>high||x!=std::floor(x))return false;
  value=(int)x;return true;
}
static bool restore_state(Engine*e,const char*text){
  const char marker[]="BL90STATE1 ";
  if(std::strncmp(text,marker,sizeof(marker)-1))return false;
  const char*p=text+sizeof(marker)-1;
  int bank,layout,root,stride;
  if(!state_int(p,bank,0,bl90::BANK_COUNT-1)||!state_int(p,layout,0,4)||
     !state_int(p,root,0,127)||!state_int(p,stride,4,12))return false;
  const int total=(int)e->bank[bank].sample.l.size();if(total<=0)return false;
  const double lows[9]={0,-24,0,0,4,0,0,0,0}, highs[9]={15,24,1,2,16,1,1,1,2};
  float value[9];
  for(int i=0;i<9;i++){double x;if(!read_state_number(p,x)||x<lows[i]||x>highs[i])return false;value[i]=(float)x;}
  int start[16],end[16];
  for(int i=0;i<16;i++){
    if(!state_int(p,start[i],0,total-1)||!state_int(p,end[i],1,total)||end[i]<=start[i]||
       (i&&start[i]<end[i-1]))return false;
  }
  while(std::isspace((unsigned char)*p))++p;if(*p)return false;
  // Parse and validate every field before changing the live engine.
  select_bank(e,bank);silence_voices(e);
  e->padMap.store(bl90::pack_pad_map(layout,root,stride));e->padLearnStage.store(0);e->padLearnRoot.store(-1);
  e->slice=value[0];e->pitch=value[1];e->reverse=value[2];e->mode=value[3];e->bits=value[4];
  e->rate=value[5];e->cutoff=value[6];e->volume=value[7];e->interpolation=value[8];e->undoValid=false;
  for(int i=0;i<16;i++){e->sliceStart[i].store(start[i]);e->sliceEnd[i].store(end[i]);}
  return true;
}
static int save_state(Engine*e,char*b,int n){
  const unsigned map=e->padMap.load();
  int used=std::snprintf(b,n,"BL90STATE1 %d %d %d %d %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g %.9g",
    e->selectedBank,bl90::map_layout(map),bl90::map_root(map),bl90::map_stride(map),
    e->slice,e->pitch,e->reverse,e->mode,e->bits,e->rate,e->cutoff,e->volume,e->interpolation);
  if(used<0||used>=n){b[0]=0;return 0;}
  for(int i=0;i<16;i++){
    const int count=std::snprintf(b+used,n-used," %d %d",e->sliceStart[i].load(),e->sliceEnd[i].load());
    if(count<0||count>=n-used){b[0]=0;return 0;}used+=count;
  }
  return 1;
}
static void destroy_engine(void*p){delete (Engine*)p;}
static void midi_engine(void*p,const uint8_t*m,int n){
  if(!p||!m||n<3)return;
  Engine*e=(Engine*)p;
  const int st=m[0]&0xf0, note=m[1], vel=m[2];
  if(st!=0x90||vel<=0||note>127)return; // one-shot slices ignore note-off
  e->lastMidiNote.store(note);
  const int stage=e->padLearnStage.load();
  if(stage==1){e->padLearnRoot.store(note);e->padLearnStage.store(2);return;}
  if(stage==2){
    const int root=e->padLearnRoot.load(), stride=note-root;
    if(bl90::valid_chromatic_map(root,stride)){
      e->padMap.store(bl90::pack_pad_map(bl90::ChromaticPads,root,stride));
      e->padChanged.fetch_or(7u); // wrapper reports the three learned controls
      e->padLearnStage.store(3);
    }else e->padLearnStage.store(4); // retain the complete previous map
    return;
  }
  if(stage>=3)e->padLearnStage.store(0);
  const unsigned map=e->padMap.load();
  int sl=bl90::note_to_pad(bl90::map_layout(map),bl90::map_root(map),note,bl90::map_stride(map));
  // A mapped pad always wins: note 60 can be a physical pad in other layouts.
  // The historical audition note remains available when it is outside the map.
  if(sl<0&&note==60)sl=std::max(0,std::min(15,(int)std::round(e->slice)));
  if(sl>=0){e->heldSlice=sl;start_voice(e,sl,vel/127.f);}
}
static void set_param_engine(void*p,const char*k,const char*v){if(!p||!k||!v)return;Engine*e=(Engine*)p;float x=(float)std::atof(v);
 if(!std::strcmp(k,"state")){restore_state(e,v);return;}
 if(!std::strcmp(k,"break_select")){
   if(std::isfinite(x))select_bank(e,(int)std::round(std::max(0.f,std::min((float)(bl90::BANK_COUNT-1),x))));
   return;
 }
#define SET(name,field,lo,hi) if(!std::strcmp(k,name)){e->field=std::max((float)(lo),std::min((float)(hi),x));return;}
 if(!std::strcmp(k,"pad_learn")){
   if(x>.5f){e->padLearnRoot.store(-1);e->padLearnStage.store(1);}
   return; // ignore the host's momentary release; learning needs two pad hits
 }
 if(!std::strcmp(k,"pad_layout")||!std::strcmp(k,"pad_root")||!std::strcmp(k,"pad_stride")){
   if(!std::isfinite(x))return;
   const bool layout=!std::strcmp(k,"pad_layout"), root=!std::strcmp(k,"pad_root");
   const int value=(int)std::round(std::max(layout?0.f:root?0.f:4.f,std::min(layout?4.f:root?127.f:12.f,x)));
   unsigned old=e->padMap.load(), changed;
   do{
     changed=bl90::pack_pad_map(layout?value:bl90::map_layout(old),root?value:bl90::map_root(old),!layout&&!root?value:bl90::map_stride(old));
   }while(!e->padMap.compare_exchange_weak(old,changed));
   return;
 }
 SET("slice",slice,0,15)
 if(!std::strcmp(k,"slice_start")||!std::strcmp(k,"slice_end")){
   const int total=(int)e->sample->l.size();
   if(total>0){
     const int sl=std::max(0,std::min(15,(int)std::round(e->slice)));
     const int minGap=std::max(8,std::min(128,total/32));
     const int oldStart=e->sliceStart[sl].load(), oldEnd=e->sliceEnd[sl].load();
     const float norm=std::max(0.f,std::min(1.f,x));
     const int baseA=e->baseStart[sl], baseB=e->baseEnd[sl];
     int frame=baseA+(int)std::round(norm*std::max(0,baseB-baseA));
     if(!std::strcmp(k,"slice_start")){
       const int lower=sl?e->sliceEnd[sl-1].load()+minGap:0;
       const int upper=e->sliceEnd[sl].load()-minGap;
       frame=std::max(lower,std::min(upper,frame));
       if(frame!=oldStart){e->undoStart[sl]=oldStart;e->undoEnd[sl]=oldEnd;e->undoSlice=sl;e->undoValid=true;e->sliceStart[sl].store(frame);}
     } else {
       const int lower=e->sliceStart[sl].load()+minGap;
       const int upper=sl<15?e->sliceStart[sl+1].load()-minGap:total;
       frame=std::max(lower,std::min(upper,frame));
       if(frame!=oldEnd){e->undoStart[sl]=oldStart;e->undoEnd[sl]=oldEnd;e->undoSlice=sl;e->undoValid=true;e->sliceEnd[sl].store(frame);}
     }
   }
   return;
 }
 if(!std::strcmp(k,"undo")){if(x>.5f&&e->undoValid){const int sl=e->undoSlice;e->sliceStart[sl].store(e->undoStart[sl]);e->sliceEnd[sl].store(e->undoEnd[sl]);e->undoValid=false;}return;}
 SET("interpolation",interpolation,0,2) SET("pitch",pitch,-24,24) SET("reverse",reverse,0,1) SET("mode",mode,0,2) SET("bits",bits,4,16) SET("rate",rate,0,1) SET("cutoff",cutoff,0,1) SET("volume",volume,0,1)
#undef SET
}
static int get_param_engine(void*p,const char*k,char*b,int n){if(!p||!k||!b||n<2)return 0;Engine*e=(Engine*)p;float x=0;
 if(!std::strcmp(k,"state"))return save_state(e,b,n);
 if(!std::strcmp(k,"break_select")){std::snprintf(b,n,"%d",e->selectedBank);return 1;}
#define GET(name,field) if(!std::strcmp(k,name)){x=e->field;std::snprintf(b,n,"%.3f",x);return 1;}
 if(!std::strcmp(k,"midi_note")){
   const int stage=e->padLearnStage.load(), note=e->lastMidiNote.load();
   const unsigned map=e->padMap.load();
   if(stage==1)std::snprintf(b,n,"HIT PAD 1");
   else if(stage==2)std::snprintf(b,n,"HIT PAD 5");
   else if(stage==3)std::snprintf(b,n,"OK ROOT%d ROW%d",bl90::map_root(map),bl90::map_stride(map));
   else if(stage==4)std::snprintf(b,n,"BAD ROW; RETRY");
   else if(note<0)std::snprintf(b,n,"--");else std::snprintf(b,n,"%d",note);
   return 1;
 }
 if(!std::strcmp(k,"pad_learn")){std::snprintf(b,n,"0");return 1;}
 if(!std::strcmp(k,"pad_layout")||!std::strcmp(k,"pad_root")||!std::strcmp(k,"pad_stride")){
   const unsigned map=e->padMap.load();
   const int value=!std::strcmp(k,"pad_layout")?bl90::map_layout(map):!std::strcmp(k,"pad_root")?bl90::map_root(map):bl90::map_stride(map);
   std::snprintf(b,n,"%d",value);return 1;
 }
 GET("slice",slice)
 if(!std::strcmp(k,"slice_start")||!std::strcmp(k,"slice_end")){
   const int total=(int)e->sample->l.size();
   const int sl=std::max(0,std::min(15,(int)std::round(e->slice)));
   const int frame=!std::strcmp(k,"slice_start")?e->sliceStart[sl].load():e->sliceEnd[sl].load();
   const int baseA=e->baseStart[sl], baseB=e->baseEnd[sl];
   x=baseB>baseA?(float)(frame-baseA)/(float)(baseB-baseA):0.f;
   std::snprintf(b,n,"%.5f",x);return 1;
 }
 if(!std::strcmp(k,"undo")){std::snprintf(b,n,"0");return 1;}
 GET("interpolation",interpolation) GET("pitch",pitch) GET("reverse",reverse) GET("mode",mode) GET("bits",bits) GET("rate",rate) GET("cutoff",cutoff) GET("volume",volume)
#undef GET
 if(!std::strcmp(k,"status")){std::snprintf(b,n,"%s",e->status);return 1;}return 0;}
static void render_engine(void *p, int16_t *out, int frames) {
  Engine *e = (Engine *)p;
  if (!e || !out || frames <= 0) return;

  const float cutoffHz = 150.0f + e->cutoff * 18000.0f;
  const float alpha = 1.0f - std::exp(-2.0f * (float)PI * cutoffHz / 44100.0f);
  const int bits = (int)std::round(e->bits);
  // Vintage keeps the wider 1..24-step rate-crush range. PAULA uses a
  // narrower 1..6 divider range, approximating a controllable Paula-style
  // playback clock without pretending to be cycle-accurate hardware emulation.
  const int hold = e->mode > 1.5f ? 1 + (int)std::round(e->rate * 5.0f)
                 : e->mode > .5f ? 1 + (int)(e->rate * 23.0f) : 1;
  const double pitchStep = std::pow(2.0, e->pitch / 12.0) * (e->sample->sampleRate / 44100.0);

  for (int i = 0; i < frames; ++i) {
    float l = 0.0f, r = 0.0f;
    for (auto &v : e->voices) {
      if (!v.on) continue;

      const int lo = v.begin;
      const int hi = v.end;
      // Check the slice boundary before reading: reverse playback below the
      // slice start must never leak a frame from the preceding slice.
      if (v.pos < (double)lo || v.pos >= (double)hi ||
          v.pos >= (double)e->sample->l.size()) {
        v.on = false;
        continue;
      }

      const int ix = (int)std::floor(v.pos);
      const int ix1 = std::max(lo, std::min(ix + 1, hi - 1));
      const float frac = (float)(v.pos - ix);
      const int ixm=std::max(lo,ix-1), ix2=std::min(hi-1,ix+2);
      float sl, sr;
      if(e->mode>1.5f || e->interpolation<0.5f){
        sl=e->sample->l[ix]; sr=e->sample->r[ix];
      } else if(e->interpolation>1.5f){
        sl=bl90::cubic4(e->sample->l[ixm],e->sample->l[ix],e->sample->l[ix1],e->sample->l[ix2],frac);
        sr=bl90::cubic4(e->sample->r[ixm],e->sample->r[ix],e->sample->r[ix1],e->sample->r[ix2],frac);
      } else {
        sl=e->sample->l[ix]+(e->sample->l[ix1]-e->sample->l[ix])*frac;
        sr=e->sample->r[ix]+(e->sample->r[ix1]-e->sample->r[ix])*frac;
      }

      const double fromStart = v.direction > 0 ? v.pos - v.begin : (v.end - 1) - v.pos;
      const double toEnd = v.direction > 0 ? (v.end - 1) - v.pos : v.pos - v.begin;
      float env = 1.0f;
      if (v.fadeFrames > 0) {
        env = (float)std::min(1.0, std::max(0.0, std::min(fromStart, toEnd) / v.fadeFrames));
      }
      l += sl * v.gain * env;
      r += sr * v.gain * env;

      const double step = pitchStep * v.direction;
      v.pos += step;
      if ((step > 0.0 && v.pos >= v.end) || (step < 0.0 && v.pos < lo)) v.on = false;
    }

    if (e->rateCounter == 0) {
      e->heldL = l;
      e->heldR = r;
    }
    if (++e->rateCounter >= hold) e->rateCounter = 0;
    if (e->mode > .5f) {
      l = bl90::paula_quantize(e->heldL, bits);
      r = bl90::paula_quantize(e->heldR, bits);
    } else {
      l = e->heldL;
      r = e->heldR;
    }

    e->lpL += alpha * (l - e->lpL);
    e->lpR += alpha * (r - e->lpR);
    l = e->lpL * e->volume;
    r = e->lpR * e->volume;
    l = std::max(-1.0f, std::min(1.0f, l));
    r = std::max(-1.0f, std::min(1.0f, r));
    out[i * 2] = (int16_t)(l * 32767.0f);
    out[i * 2 + 1] = (int16_t)(r * 32767.0f);
  }
}
static const mpc_engine_t ENGINE={create_engine,destroy_engine,midi_engine,set_param_engine,get_param_engine,render_engine,nullptr,nullptr};
extern "C" const mpc_engine_t* mpc_engine(void){return &ENGINE;}

// The wrapper drains engine-originated changes and reports current values to the
// host. Fixed bits and static names keep calibration notifications allocation-free.
extern "C" int mpc_engine_changed(void*p,char*key,int len){
  if(!p||!key||len<11)return 0;
  Engine*e=(Engine*)p;
  unsigned pending=e->padChanged.load();
  while(pending){
    const unsigned bit=pending&(~pending+1u);
    if(e->padChanged.compare_exchange_weak(pending,pending&~bit)){
      const char*name=bit==1u?"pad_layout":bit==2u?"pad_root":"pad_stride";
      std::memcpy(key,name,std::strlen(name)+1);
      return 1;
    }
  }
  return 0;
}
