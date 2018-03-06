// Ben . 2018.03.05. AlloComet.0.01
// Press t,f,g,h to control the center of the agent(core)
// Press '5' to re-center the camera

#include "allocore/io/al_App.hpp"
#include "Gamma/Oscillator.h"
#include "allocore/io/al_App.hpp"
#include "allocore/math/al_Ray.hpp"
#include "allocore/math/al_Vec.hpp"

using namespace al;
using namespace std;

unsigned planetCount = 20;       
float maximumAcceleration = 5;  // prevents explosion, loss of particles
float initialRadius = 50;         // initial condition
float initialSpeed = 10;           // initial condition
float dragFactor = 0.1;           //
float timeStep = 0.1;        // keys change this value for effect
float scaleFactor = 0.1;          // resizes the entire scene
float cometRadius = 10;  // increase this to make collisions more frequent
float planetRadius = 100;
float steerFactor = 100;

Mesh sphere; 
Mesh cone;  

// helper function: makes a random vector
Vec3d r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

struct Comet {
  Vec3f position,velocity,acceleration;
  Color cl;
  Comet() {
    position = Vec3d(0,0,0);
    cl = HSV(0.5, 0.7, 1);
  }
  void draw(Graphics& g) {
    g.pushMatrix();
    g.translate(position);
    g.color(cl);
    g.draw(sphere);
    g.popMatrix();
  }
};


struct Planet {
  Vec3f position, head;
  Color cl;
  Planet() {
    head = r() * 30;
    position = r() * 900;
  	cl = HSV(rnd::uniform(), 0.7, 1);
   }
 void draw(Graphics& g) {
	g.pushMatrix();
	g.translate(position);
	g.color(cl);
	g.draw(sphere);
	g.popMatrix();
  }
};

struct MyApp : App {
  Material material;
  Light light;

  bool simulate = true;
  unsigned frameCount = 0;
  Planet p;
  vector<Planet> planet;
  vector<Comet> comet;

  MyApp() {
    addSphere(sphere, cometRadius);
    addSphere(sphere, planetRadius);
    sphere.generateNormals();
    light.pos(0, 0, 10);              // place the light
    nav().pos(10, 10, 100);             // place the viewer
    lens().far(400);                 // set the far clipping plane
    planet.resize( planetCount);  // make all the particles
    comet.resize(1); 		     // make the flock's core
    background(Color(0.07));  

    initWindow();
    initAudio();
  }

  void onAnimate(double dt) {	
    Pose pose;
    Graphics g;
    Comet c;
    if (!simulate)
      // skip the rest of this function
      return;

    for (unsigned i = 0; i < planet.size(); ++i){
      Planet& a = planet[i];	

      Vec3f difference = (c.position - a.position);
      float d = difference.mag();
     // Vec3f acceleration = difference / d * steerFactor;

    }

    // Euler's Method; Keep the time step small
    c.acceleration += c.velocity * -dragFactor;
    c.position += c.velocity * timeStep;  
    Vec3f camera_position = Vec3f(c.position.x - (c.velocity[0] - 10), c.position.y - (c.velocity[1] - 10), 100.0);
    nav().nudgeToward(camera_position , 1);             // place the viewer
   //pose.faceToward(c.position,1);
  cout << c.position << c.velocity << camera_position <<endl;

    frameCount++;
  }

  void onDraw(Graphics& g) {
    Comet c;
    material();
    light();
    g.scale(scaleFactor);
    c.draw(g);
    for (auto p : planet) p.draw(g);
  }

  void onSound(AudioIO& io) {
    while (io()) {
      io.out(0) = 0;
      io.out(1) = 0;
    }
  }

  void onKeyDown(const ViewpointWindow&, const Keyboard& k) {
    Comet c;
    switch (k.key()) {
      default:
      case '1':
        // reverse time
        timeStep *= -1;
        break;
      case '2':
        // speed up time
        if (timeStep < 1) timeStep *= 2;
        break;
      case '3':
        // slow down time
        if (timeStep > 0.0005) timeStep /= 2;
        break;
      case '4':
        // pause the simulation
        simulate = !simulate;
        break;

      case 't':
      	c.velocity[1] += 30;
        cout << c.velocity << endl;
        break;
      case 'g':
	      c.velocity[1] -= 30;
        break;
      case 'f':
      	c.velocity[0] -= 30;
        break;
      case 'h':
	      c.velocity[0] += 30;
        break;
    }
  }
};

int main() { MyApp().start(); }
