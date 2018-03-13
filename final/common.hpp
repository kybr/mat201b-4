#ifndef __COMMON__
#define __COMMON__

#include "allocore/io/al_App.hpp"
using namespace al;

// Common definition of application state
//
#define NUM_GRAINS (20)  // How many grains we want

struct State {
  Vec3f ball_position;
};

#endif
