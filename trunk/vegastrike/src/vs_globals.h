#ifndef __VS_GLOBALS_H_
#define __VS_GLOBALS_H_

#include <vector>

/* All global data is stored in a variable of this type */
typedef struct {
  int audio_frequency_mode; //0==11025/8 1==22050/8 2==44100/8  3==11025/16 4==22050/16 5==44100/16  
  int sound_enabled;
  int music_enabled;
  int sound_volume;
  int music_volume;
  int warning_level;
  int capture_mouse;
  float MouseSensitivityX;
  float MouseSensitivityY;   
  float detaillevel; /*how much the LOD calculation gets multiplied by...higher="bigger" */
  int y_resolution;
  int x_resolution;  
  float znear;
  float zfar ;
  float fov ;
  float aspect ;
  float difficulty;
} game_data_t;

extern game_data_t g_game;

class VegaConfig;
extern VegaConfig *vs_config;

class Mission;
extern Mission *mission;
template <class MyType> class LeakVector {
  std::vector <MyType> *active_missions;
 public:
  bool empty()const {return active_missions->empty();}
  void push_back (MyType mis) {active_missions->push_back (mis);}
  MyType back() {return active_missions->back();}
  LeakVector () {active_missions = new std::vector <MyType>();}

  unsigned int size ()const  {return (active_missions)->size();}
  MyType  operator [] (unsigned int i) {return (*active_missions)[i];}
  ~LeakVector () {/* DO NOTHING OR DIE INTENTIONAL LEAK We need this data after Exit*/}
  std::vector <MyType> * Get() {return active_missions;}
};

extern LeakVector<Mission *> active_missions;
class ForceFeedback;
extern ForceFeedback *forcefeedback;

extern double benchmark;

#endif

