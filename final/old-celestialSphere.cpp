// Ben . 2018.03.06. Agent ver.0.01
// Press t,f,g,h to control the center of the agent(core)

#include "allocore/io/al_App.hpp"
#include "Gamma/Oscillator.h"
#include "allocore/io/al_App.hpp"
#include "allocore/math/al_Ray.hpp"
#include "allocore/math/al_Vec.hpp"

using namespace al;
using namespace std;

// Physics const.
float maximumAcceleration = 5;  // prevents explosion, loss of planets
float dragFactor = 0.1;           //
float timeStep = 0.1;        // keys change this value for effect
float scaleFactor = 0.1;          // resizes the entire scene
// Planet const.
unsigned planetCount = 23;
float planetRadius = 10;  // increase this to make collisions more frequent
float planetRange = 500;
// Comet const.
float cometRadius = 20;
float steerFactor = 100;
// Dust const.
unsigned dustCount = 300;       
float dustRange = 1000;
float dustRadius = 1;

bool keys[4];
Mesh comet; 
Mesh planet;
Mesh dust;  
// helper function: makes a random vector
Vec3d r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

struct Comet {
  Vec3f position,velocity,acceleration;
  float abs_speed, unit_rotate, force, head, head_angv;
  Color ton;
  Comet() {
    position = Vec3d(0,0,0);
    head = 0;
    head_angv = 0;
    ton = HSV(0.5, 0.2, 1);
    unit_rotate = 0.01;
    force = 1;
  }
  void draw(Graphics& g) {
    g.pushMatrix();
    g.translate(position);
    g.color(ton);
    g.draw(comet);
    g.popMatrix();
    head_angv *= 0.95;
    head += head_angv * timeStep;
    if (head > 2*M_PI) {
      head -= 2*M_PI;
    }
    if (head < 0) {
      head += 2*M_PI;
    }
  }
};


struct Planet {
  Vec3f position;
  Color ton;
  Planet() {
    position = r() * planetRange;
  	ton = HSV(rnd::uniform(), 0.7, 1);
   }
 void draw(Graphics& g) {
	g.pushMatrix();
	g.translate(position);
	g.color(ton);
	g.draw(planet);
	g.popMatrix();
  }
};

struct Stardust {
  Vec3f position;
  Color ton;
  Stardust() {
    position = r() * dustRange;
  	ton = HSV( rnd::uniform() * M_PI , 0.1, 1);
  }
 void draw(Graphics& g) {
	g.pushMatrix();
	g.translate(position);
	g.color(ton);
	g.draw(dust);
	g.popMatrix();
 }
};


struct MyApp : App {
  Material material;
  Light light;
  Vec3f camera_position = Vec3f(c.position.x - 10, c.position.y - 10,  c.position.z + 10);

  bool simulate = true;
  unsigned frameCount = 0;
  Planet p;
  Comet c;
  Stardust d;
  vector<Planet> planets;
  vector<Comet> acomet;
  vector<Stardust> dusts;

  MyApp() {
    addSphere(comet, cometRadius);
    addSphere(planet, planetRadius);
    addSphere(dust, dustRadius);

    comet.generateNormals();
    planet.generateNormals();
    dust.generateNormals();


    light.pos(0, 0, 10);              // place the light
    nav().pos(0, 0, 100);             // place the viewer
    lens().far(400);                 // set the far clipping plane
    planets.resize(planetCount);  // make all the planets
    dusts.resize(dustCount);  // make all the planets
    acomet.resize(1); 		     // make a comet

    background(Color(0.07));  

    initWindow();
    initAudio();
  }

  void onAnimate(double dt) {	
    Pose pose;
    Graphics g;
    if (!simulate)      // skip the rest of this function
      return;
    
    // Euler's Method; Keep the time step small
    d.position += c.velocity * timeStep * 0.7;
    c.acceleration += c.velocity * -dragFactor;
    c.position += c.velocity * timeStep;
    c.abs_speed = sqrt( c.velocity[0] * c.velocity[0] + c.velocity[2] * c.velocity[2]);
    camera_position[0] = c.position.x - (c.velocity[0] / c.abs_speed) * 0.01 * timeStep;
    camera_position[1] = c.position.y - 10;
    camera_position[2] = c.position.z - (c.velocity[2] / c.abs_speed) *0.01* timeStep;

 //   nav().nudgeToward(camera_position , 1);             // place the viewer
//   	pose.faceToward(c.velocity,1);
    frameCount++;
  }

  void onDraw(Graphics& g) {
    material();
    light();
    g.scale(scaleFactor);
    c.draw(g);
    for (auto p : planets) p.draw(g);
    for (auto d : dusts) d.draw(g);
    keyCommand();
  }

  void onSound(AudioIO& io) {
    while (io()) {
      io.out(0) = 0;
      io.out(1) = 0;
    }
  }

  void keyCommand(){
    if (keys[0]){
      c.velocity.x -= c.force * cos(c.head);
      c.velocity.z -= c.force * sin(c.head);
    }
    if (keys[1]){
      c.velocity.x += c.force * cos(c.head);
      c.velocity.z += c.force * sin(c.head);
    }
    if (keys[2]) c.head_angv -= c.unit_rotate;
    if (keys[3]) c.head_angv += c.unit_rotate;
    cout << c.head << endl; 
  }


  void onKeyDown(const ViewpointWindow&, const Keyboard& k) {
    switch (k.key()) {
      default:
 /*     case '1':
        // reverse time
        timeStep *= -1;
        break;*/
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
      case '5':
	// Follow the core
        nav().nudgeToward(c.position, 1); 	
        Pose().faceToward(c.position, 1);
    	break;
      case 't':
        keys[0] = true;
        break;
      case 'g':
        keys[1] = true;
        break;
      case 'f':
        keys[2] = true;
        break;
      case 'h':
        keys[3] = true;
        break;
    }
  }

  void onKeyUp(const ViewpointWindow&, const Keyboard& k) {
    switch (k.key()) {
      case 't':
        keys[0] = false;
        break;
      case 'g':
        keys[1] = false;
        break;
      case 'f':
        keys[2] = false;
        break;
      case 'h':
        keys[3] = false;
        break;
    }
  }
};

int main() { MyApp().start(); }
