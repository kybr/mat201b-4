// MAT201B
// Winter 2018
// Author: Ben Myungin Lee.
//
// Cuttlebone "Laptop Graphics Renderer"
//

#include "Cuttlebone/Cuttlebone.hpp"
#include "allocore/io/al_App.hpp"

#include "common.hpp"

#include "alloutil/al_OmniStereoGraphicsRenderer.hpp"

using namespace al;

struct MyApp : App {
  Mesh ball_mesh;

  State state;
  cuttlebone::Taker<State> taker;

  MyApp() {
    addSphere(ball_mesh);
    navControl().useMouse(false);
    initWindow();
  }

  virtual void onAnimate(double dt) { 
    

    taker.get(state); 
    static bool hasNeverHeardFromSim = true;
    if (taker.get(state) > 0) hasNeverHeardFromSim = false;
    if (hasNeverHeardFromSim) return;


      myCursor.position = state.cursorPosition;
      nav().pos(state.navPosition);
      nav().quat (state.navOrienation);
    }
    
    }

  virtual void onDraw(Graphics& g, const Viewpoint& v) {
    g.translate(state.ball_position);
    g.draw(ball_mesh);
  }
};

int main() {
  MyApp app;
  app.taker.start();
  app.start();
}
