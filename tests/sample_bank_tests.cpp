extern "C" {
#include "engine.h"
}
#include "sample_bank.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

static void put16(FILE* f,unsigned v){std::fputc(v&255,f);std::fputc((v>>8)&255,f);}
static void put32(FILE* f,unsigned v){put16(f,v&65535);put16(f,v>>16);}
static void wav(const std::string& path,int amp){
  FILE*f=std::fopen(path.c_str(),"wb");assert(f);
  const unsigned frames=16000,bytes=frames*4;
  std::fwrite("RIFF",1,4,f);put32(f,36+bytes);std::fwrite("WAVEfmt ",1,8,f);put32(f,16);put16(f,1);put16(f,2);
  put32(f,44100);put32(f,176400);put16(f,4);put16(f,16);std::fwrite("data",1,4,f);put32(f,bytes);
  for(unsigned i=0;i<frames;i++){put16(f,amp);put16(f,amp);}
  assert(!std::fclose(f));
}
static std::string get(const mpc_engine_t*api,void*e,const char*key){char b[4096]={};assert(api->get_param(e,key,b,sizeof(b)));return b;}
static void set(const mpc_engine_t*api,void*e,const char*key,int value){char b[32];std::snprintf(b,sizeof(b),"%d",value);api->set_param(e,key,b);}
static long energy(const mpc_engine_t*api,void*e){
  uint8_t note[3]={0x90,36,127};api->midi(e,note,3);int16_t out[512]={};api->render(e,out,256);
  long result=0;for(int16_t x:out)result+=std::abs((int)x);return result;
}
int main(){
  char temp[]="/tmp/bl90-bank-XXXXXX";const char*dir=mkdtemp(temp);assert(dir);
  const std::string root(dir),folder=root+"/breaks";assert(!mkdir(folder.c_str(),0700));
  // A different level in every slot makes bank selection observable in rendered sound.
  for(int i=0;i<bl90::BANK_COUNT;i++)wav(folder+"/"+bl90::BANK_FILES[i],400+i*900);
  const mpc_engine_t*api=mpc_engine();void*e=api->create(dir);assert(e);
  // Removing every source proves switching is RAM-only after initial creation.
  for(int i=0;i<bl90::BANK_COUNT;i++)assert(!unlink((folder+"/"+bl90::BANK_FILES[i]).c_str()));
  long previous=0;
  for(int i=0;i<bl90::BANK_COUNT;i++){
    set(api,e,"break_select",i);assert(std::atoi(get(api,e,"break_select").c_str())==i);
    long current=energy(api,e);assert(current>previous);previous=current;
  }
  // END-only edits must bind Undo to the edited slice, not an older/default slot.
  set(api,e,"slice",0);
  const std::string zeroStart=get(api,e,"slice_start"),zeroEnd=get(api,e,"slice_end");
  set(api,e,"slice",5);
  const std::string fifthStart=get(api,e,"slice_start"),fifthEnd=get(api,e,"slice_end");
  api->set_param(e,"slice_end","0.6");assert(get(api,e,"slice_end")!=fifthEnd);
  api->set_param(e,"undo","1");assert(get(api,e,"slice_start")==fifthStart);assert(get(api,e,"slice_end")==fifthEnd);
  set(api,e,"slice",0);assert(get(api,e,"slice_start")==zeroStart);assert(get(api,e,"slice_end")==zeroEnd);
  // Bank selection must not reset a calibrated physical pad layout.
  set(api,e,"pad_layout",1);set(api,e,"pad_root",48);set(api,e,"pad_stride",5);
  set(api,e,"break_select",3);
  assert(std::atoi(get(api,e,"pad_root").c_str())==48);assert(std::atoi(get(api,e,"pad_stride").c_str())==5);
  api->set_param(e,"pitch","-3");api->set_param(e,"slice","2");api->set_param(e,"slice_start","0.2");
  const std::string sliceStart=get(api,e,"slice_start"),saved=get(api,e,"state");assert(saved.find("BL90STATE1")==0);
  void*restored=api->create(dir);assert(restored); // no source files now; missing bank restore is rejected
  const std::string missingBefore=get(api,restored,"state");
  api->set_param(restored,"state",saved.c_str());assert(get(api,restored,"state")==missingBefore);
  api->destroy(restored);
  // The same populated instance can restore every sound/mapping/edit field without a reload.
  set(api,e,"break_select",7);set(api,e,"pad_root",36);api->set_param(e,"pitch","12");
  api->set_param(e,"state",saved.c_str());
  assert(std::atoi(get(api,e,"break_select").c_str())==3);
  assert(std::atoi(get(api,e,"pad_root").c_str())==48);
  assert(std::fabs(std::atof(get(api,e,"pitch").c_str())+3)<0.001);
  assert(get(api,e,"slice_start")==sliceStart);
  assert(get(api,e,"state")==saved);
  // Truncated/unknown state is rejected transactionally.
  api->set_param(e,"state","BL90STATE1 3 0");assert(get(api,e,"state")==saved);
  api->set_param(e,"state","BL90STATE99");assert(get(api,e,"state")==saved);
  api->destroy(e);
  // A missing selection must retain the current audible sample and selected index.
  wav(folder+"/"+bl90::BANK_FILES[0],1000);e=api->create(dir);assert(e);
  set(api,e,"break_select",1);assert(std::atoi(get(api,e,"break_select").c_str())==0);
  assert(get(api,e,"status").find("MISSING")!=std::string::npos);assert(energy(api,e)>0);
  api->destroy(e);assert(!unlink((folder+"/"+bl90::BANK_FILES[0]).c_str()));assert(!rmdir(folder.c_str()));assert(!rmdir(root.c_str()));
  std::puts("PASS: eleven preloaded banks, switching after source deletion, sample/pad/edit state, missing bank fallback and malformed state rejection");
}
