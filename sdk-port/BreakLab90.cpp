// BREAK LAB 90 — native MPC SDK integration shim.
// This preserves the existing ARMv7 break engine and stable Brk9 VST identity.
// The legacy core is compiled into this translation unit during CI packaging.
// TODO for the next DSP revision: replace the int16 bridge with native float output.
// Copyright OpenBench contributors; official SDK framework is MIT.
#include "mpc_plugin.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <algorithm>
#include <limits>
#include "engine_core.inc"

class BreakLab90 final : public mpc::Plugin {
public:
  enum P { Slice, Pitch, Direction, Character, Bits, Rate,
           Filter, Volume, Status, Resampling, Start, End, Undo, Count };

  static mpc::PluginInfo info() {
    mpc::PluginInfo i;
    i.name="BREAK LAB 90";
    i.vendor="OpenBench";
    i.uniqueId=MPC_FOURCC('B','r','k','9');
    i.version=300;
    i.isInstrument=true; i.numInputs=0; i.numOutputs=2;
    return i;
  }

  BreakLab90(): mpc::Plugin(info()) {
    // Never reorder these IDs: existing MPC project compatibility.
    addParam(mpc::intParam("slice","Slice",0,15,0));
    addParam(mpc::floatParam("pitch","Pitch",-24.f,24.f,0.f,"st",2));
    addParam(mpc::choiceParam("reverse","Direction",{"Forward","Reverse"},0));
    addParam(mpc::choiceParam("mode","Character",{"Clean","Vintage","PAULA"},0));
    addParam(mpc::intParam("bits","Bits",4,16,12));
    addParam(mpc::floatParam("rate","Rate Crush",0.f,1.f,0.f,"",3));
    addParam(mpc::floatParam("cutoff","Filter",0.f,1.f,1.f,"",3));
    addParam(mpc::floatParam("volume","Volume",0.f,1.f,.85f,"",3));
    // Old index 8 was a status readout; keep a harmless placeholder to retain indices.
    addParam(mpc::floatParam("status","Status",0.f,1.f,0.f,"",0));
    addParam(mpc::choiceParam("interpolation","Resampling",{"Paula Hold","Linear","Cubic","HQ Sinc"},3));
    addParam(mpc::floatParam("slice_start","Slice Start",0.f,1.f,0.f,"",3));
    addParam(mpc::floatParam("slice_end","Slice End",0.f,1.f,1.f,"",3));
    addParam(mpc::boolParam("undo","Undo",false));
    // The SDK exposes the plugin's bundled Content/ directory even when CWD differs.
    std::string wav=mpc::contentPath("breaks/break01.wav");
    const std::string suffix="/breaks/break01.wav";
    if(wav.size()>=suffix.size() &&
       wav.compare(wav.size()-suffix.size(),suffix.size(),suffix)==0)
      wav.resize(wav.size()-suffix.size());
    core_=mpc_engine();
    inst_=core_ ? core_->create(wav.c_str()) : nullptr;
    for(int k=0;k<Count;k++) last_[k]=std::numeric_limits<float>::quiet_NaN();
    std::vector<float> defaults((size_t)numParams());
    for(int i=0;i<numParams();i++) defaults[(size_t)i]=param(i).def;
    addProgram("Clean HQ",defaults);
  }

  ~BreakLab90() override {if(inst_ && core_)core_->destroy(inst_);}

  void process(mpc::ProcessContext& ctx) override {
    if(ctx.numOutputs<1 || !ctx.outputs || !ctx.outputs[0]) return;
    float *L=ctx.outputs[0];
    float *R=ctx.numOutputs>1 ? ctx.outputs[1] : nullptr;
    // Silence all output, including when the sample is missing.
    for(int n=0;n<ctx.numFrames;n++){L[n]=0.f;if(R)R[n]=0.f;}
    if(!inst_ || !core_)return;
    applyParams();
    int cursor=0;
    for(int e=0;e<ctx.numMidiEvents;e++) {
      const mpc::MidiEvent& ev=ctx.midi[e];
      const int pos=std::max(cursor,std::min(ctx.numFrames,ev.sampleOffset));
      render(L,R,cursor,pos);
      cursor=pos;
      if(ev.size==3) core_->midi(inst_,ev.data,3);
    }
    render(L,R,cursor,ctx.numFrames);
  }

private:
  const mpc_engine_t* core_=nullptr;
  void* inst_=nullptr;
  float last_[Count]={};
  int16_t scratch_[256*2]={};

  void applyParams(){
    static const char* keys[Count]={
      "slice","pitch","reverse","mode","bits","rate","cutoff",
      "volume","status","interpolation","slice_start","slice_end","undo"};
    // The core's status is read-only. Its undo is a one-shot, edge-triggered command.
    for(int k=0;k<Count;k++){
      if(k==Status)continue;
      const float v=get(k);
      if(v==last_[k])continue;
      char buf[48];
      std::snprintf(buf,sizeof(buf),"%.7g",(double)v);
      core_->set_param(inst_,keys[k],buf);
      last_[k]=v;
    }
  }

  void render(float* L,float* R,int first,int last){
    while(first<last) {
      const int frames=std::min(256,last-first);
      core_->render(inst_,scratch_,frames);
      for(int i=0;i<frames;i++){
        L[first+i]=scratch_[2*i]*(1.f/32768.f);
        if(R)R[first+i]=scratch_[2*i+1]*(1.f/32768.f);
      }
      first+=frames;
    }
  }
};
MPC_PLUGIN_ENTRY(BreakLab90)
