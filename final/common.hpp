#ifndef __COMMON__
#define __COMMON__

#include "Cuttlebone/Cuttlebone.hpp"
#include "allocore/io/al_App.hpp"

using namespace al;

// Common definition of application state
//

struct State {
  Vec3f navPosition;
  Quatd navOrientation;
};

#endif
