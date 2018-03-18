#ifndef __COMMON__
#define __COMMON__

#include "Gamma/Oscillator.h"
#include "Gamma/Effects.h"
#include "allocore/math/al_Ray.hpp"
#include "allocore/math/al_Vec.hpp"
#include "Cuttlebone/Cuttlebone.hpp"
#include "allocore/io/al_App.hpp"

using namespace al;

// Common definition of application state
//
// Physics const.
float maximumAcceleration = 5;  // prevents explosion, loss of planets
float dragFactor = 0.1;           //
float timeStep = 0.1;        // keys change this value for effect
float scaleFactor = 0.05;          // resizes the entire scene
// Planet const.
int planetCount = 9;
float planetRadius = 20;  // 
float planetRange = 500;

// Constellation const.
int stellCount = 2000;
// Comet const.
float cometRadius = 20;
float steerFactor = 100;
// Dust const.
unsigned dustCount = 500;       
float dustRange = 1500;
float dustRadius = 0.5;
bool keys[4];

struct State {
  Vec3f navPosition;
  Quatd navOrientation;

 // Comet
    Vec3f comet_pose;
 // Planets
    Vec3f planet_pose[planetCount];
    Vec3f planet_quat[planetCount];
 // dust
    Vec3f dust_pose[dustCount];
// Constell
    Vec3f stell_pose[stellCount] 
};

#endif
