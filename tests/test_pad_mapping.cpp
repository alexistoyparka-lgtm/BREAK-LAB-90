extern "C" {
#include "engine.h"
}
#include "../src/pad_mapping.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#define CHECK(condition) do { if(!(condition)){std::fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#condition);return false;} }while(0)
static const mpc_engine_t* api=nullptr;
static std::string directory;
static void put16(FILE*f,unsigned value){std::fputc(value&255,f);std::fputc((value>>8)&255,f);}
static void put32(FILE*f,unsigned value){put16(f,value);put16(f,value>>16);}
static bool make_source(){
  FILE*f=std::fopen((directory+"/breaks/break01.wav").c_str(),"wb");
  if(!f)return false;
  const unsigned frames=8192;
  std::fwrite("RIFF",1,4,f);put32(f,36+frames*2);std::fwrite("WAVEfmt ",1,8,f);
  put32(f,16);put16(f,1);put16(f,1);put32(f,44100);put32(f,88200);put16(f,2);put16(f,16);
  std::fwrite("data",1,4,f);put32(f,frames*2);
  // Different material in each chronological region; reference output is the
  // actual legacy engine, including its transient detector and envelopes.
  for(unsigned i=0;i<frames;++i)put16(f,700+(i/512)*1000+(i%31)*17);
  return std::fclose(f)==0;
}
static void set(void*e,const char*name,int value){char text[32];std::snprintf(text,sizeof(text),"%d",value);api->set_param(e,name,text);}
static int value(void*e,const char*name){char text[64]={};api->get_param(e,name,text,sizeof(text));return std::atoi(text);}
static std::string text(void*e,const char*name){char result[64]={};api->get_param(e,name,result,sizeof(result));return result;}
static void hit(void*e,int note,int velocity=100){const uint8_t midi[3]={0x90,(uint8_t)note,(uint8_t)velocity};api->midi(e,midi,3);}
using Audio=std::array<int16_t,256>;
static Audio render(void*e){Audio out{};api->render(e,out.data(),128);return out;}
static Audio reference(int slice){void*e=api->create(directory.c_str());hit(e,36+slice);Audio result=render(e);api->destroy(e);return result;}
static bool same_slice(int layout,int root,int stride,int pad,int note){
  void*e=api->create(directory.c_str());
  set(e,"pad_layout",layout);set(e,"pad_root",root);set(e,"pad_stride",stride);
  set(e,"slice",3); // note 60 must use its mapped pad before audition selection
  hit(e,note);Audio actual=render(e);api->destroy(e);
  CHECK(actual==reference(pad));
  CHECK(std::any_of(actual.begin(),actual.end(),[](int16_t n){return n!=0;}));
  return true;
}
static bool test_maps(){
  const int major[16]={72,74,76,77,79,81,83,84,86,88,89,91,93,95,96,98};
  const int minor[16]={48,50,51,53,55,56,58,60,62,63,65,67,68,70,72,74};
  const int drum[16]={37,36,42,82,40,38,46,44,48,47,45,43,49,55,51,53};
  for(int pad=0;pad<16;++pad){
    CHECK(same_slice(bl90::LegacyPads,72,5,pad,36+pad));
    CHECK(same_slice(bl90::ChromaticPads,48,4,pad,48+pad));
    CHECK(same_slice(bl90::ChromaticPads,60,5,pad,60+(pad/4)*5+pad%4));
    CHECK(same_slice(bl90::MajorPads,72,4,pad,major[pad]));
    CHECK(same_slice(bl90::MinorPads,48,4,pad,minor[pad]));
    CHECK(same_slice(bl90::DrumPads,36,4,pad,drum[pad]));
  }
  // Note 60 is slice 15 in this major layout, not the selected audition slice.
  CHECK(same_slice(bl90::MajorPads,36,4,14,60));
  void*e=api->create(directory.c_str());set(e,"slice",7);hit(e,60);
  CHECK(render(e)==reference(7));api->destroy(e);
  CHECK(bl90::note_to_pad(bl90::ChromaticPads,60,64,5)==-1);
  CHECK(bl90::pad_note(bl90::ChromaticPads,127,1)==-1);
  return true;
}
static bool test_calibration(){
  void*e=api->create(directory.c_str());
  CHECK(value(e,"pad_layout")==bl90::LegacyPads);
  CHECK(text(e,"midi_note")=="--");
  set(e,"pad_learn",1);set(e,"pad_learn",0);
  CHECK(value(e,"pad_learn")==0);
  CHECK(text(e,"midi_note")=="HIT PAD 1");
  char changed[32]={};CHECK(mpc_engine_changed(e,changed,sizeof(changed))==0);
  hit(e,60,0);CHECK(text(e,"midi_note")=="HIT PAD 1");
  hit(e,60);CHECK(text(e,"midi_note")=="HIT PAD 5");
  CHECK(render(e)==Audio{}); // calibration hit never auditions note 60
  CHECK(value(e,"pad_layout")==bl90::LegacyPads); // no partial commit
  CHECK(mpc_engine_changed(e,changed,sizeof(changed))==0);
  hit(e,65);CHECK(render(e)==Audio{});
  CHECK(value(e,"pad_layout")==bl90::ChromaticPads);
  CHECK(value(e,"pad_root")==60);CHECK(value(e,"pad_stride")==5);
  CHECK(text(e,"midi_note")=="OK ROOT60 ROW5");
  CHECK(mpc_engine_changed(e,changed,2)==0); // small buffer cannot consume a bit
  const char*expectedKeys[]={"pad_layout","pad_root","pad_stride"};
  const int expectedValues[]={bl90::ChromaticPads,60,5};
  for(int i=0;i<3;++i){
    CHECK(mpc_engine_changed(e,changed,sizeof(changed))==1);
    CHECK(std::strcmp(changed,expectedKeys[i])==0);
    CHECK(value(e,changed)==expectedValues[i]);
  }
  CHECK(mpc_engine_changed(e,changed,sizeof(changed))==0);
  hit(e,60);CHECK(render(e)==reference(0));
  CHECK(text(e,"midi_note")=="60");
  api->destroy(e);

  // Each invalid two-hit sequence retains the entire pre-existing map.
  const int invalid[][2]={{48,48},{48,51},{48,61},{120,124},{70,60}};
  for(const auto&pair:invalid){
    e=api->create(directory.c_str());
    set(e,"pad_layout",bl90::MinorPads);set(e,"pad_root",48);set(e,"pad_stride",6);
    set(e,"pad_learn",1);hit(e,pair[0]);hit(e,pair[1]);
    CHECK(render(e)==Audio{});CHECK(text(e,"midi_note")=="BAD ROW; RETRY");
    CHECK(value(e,"pad_layout")==bl90::MinorPads);
    CHECK(value(e,"pad_root")==48);CHECK(value(e,"pad_stride")==6);
    CHECK(mpc_engine_changed(e,changed,sizeof(changed))==0);
    api->destroy(e);
  }
  return true;
}
int main(){
  char path[]="/tmp/bl90-pads-XXXXXX";char*dir=mkdtemp(path);
  if(!dir)return 1;directory=dir;
  if(mkdir((directory+"/breaks").c_str(),0700)!=0||!make_source())return 1;
  api=mpc_engine();const bool passed=test_maps()&&test_calibration();
  unlink((directory+"/breaks/break01.wav").c_str());rmdir((directory+"/breaks").c_str());rmdir(directory.c_str());
  if(!passed)return 1;
  std::puts("PASS: all 16 chronological slices across legacy, chromatic, row-stride, major, minor and MPC drum maps; note-60 collision; silent two-hit calibration and invalid-map rollback");
  return 0;
}
