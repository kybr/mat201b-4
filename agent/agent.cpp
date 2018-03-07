// Ben . 2018.02.07. Agent ver.0.01
// Press t,f,g,h to control the center of the agent(core)
// Press '5' to re-center the camera
// Still working with the orientation of the each particle


#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

unsigned particleCount = 200;       
float maximumAcceleration = 5;  // prevents explosion, loss of particles
float initialRadius = 50;         // initial condition
float initialSpeed = 10;           // initial condition
float dragFactor = 0.1;           //
float timeStep = 0.1;        // keys change this value for effect
float scaleFactor = 0.1;          // resizes the entire scene
float coneRadius = 10;  // increase this to make collisions more frequent
float coreRadius = 20;
float steerFactor = 100;

Mesh sphere; 
Mesh cone;  
// helper function: makes a random vector
Vec3d r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

struct Core {
  Vec3f position,velocity,acceleration;
  Color c;
  Core() {
    position = Vec3d(0,0,0);
    c = HSV(0.5, 0.7, 1);
  }
  void draw(Graphics& g) {
    g.pushMatrix();
    g.translate(position);
    g.color(c);
    g.draw(sphere);
    g.popMatrix();
  }
};


struct Particle {
  Vec3f position, velocity, acceleration, head;
  Color c;
  Particle() {
    head = r() * 30;
    position = r() * initialRadius;
    velocity =
        r().cross(position).normalize(initialSpeed);
  	c = HSV(rnd::uniform(), 0.7, 1);
   }
 void draw(Graphics& g) {
	g.pushMatrix();
	g.translate(position);
	g.color(c);
	g.draw(cone);
	g.popMatrix();
  }
};

struct MyApp : App {
  Material material;
  Light light;

  bool simulate = true;
  unsigned frameCount = 0;
  Particle p;
  vector<Particle> particle;
  vector<Core> core;

  MyApp() {
    addSphere(sphere, coreRadius);
    addCone(cone, coneRadius, Vec3f(10,10,10), 16,1);
    sphere.generateNormals();
    cone.generateNormals();
    light.pos(0, 0, 10);              // place the light
    nav().pos(0, 0, 100);             // place the viewer
    lens().far(400);                 // set the far clipping plane
    particle.resize(particleCount);  // make all the particles
    core.resize(1); 		     // make the flock's core
    background(Color(0.07));  

    initWindow();
    initAudio();
  }

  void onAnimate(double dt) {	
    Pose pose;
    Graphics g;
    if (!simulate)
      // skip the rest of this function
      return;

    for (unsigned i = 0; i < particle.size(); ++i){
	Core& c = core[0];
        Particle& a = particle[i];	

        Vec3f difference = (c.position - a.position);
        float d = difference.mag();
        Vec3f acceleration = difference / d * steerFactor;
        a.acceleration += acceleration;
    	g.pushMatrix();
   	pose.faceToward(a.velocity,1);
  	g.popMatrix();


    }
    for (auto& p : particle) p.acceleration += p.velocity * -dragFactor;

    // Euler's Method; Keep the time step small
    for (auto& c : core) c.acceleration += c.velocity * -dragFactor;
    for (auto& c : core) c.position += c.velocity * timeStep;
    for (auto& p : particle) p.position += p.velocity * timeStep;
    for (auto& p : particle) p.velocity += p.acceleration * timeStep;
    for (auto& p : particle) p.acceleration.zero();  // XXX zero accelerations
    frameCount++;
  }

  void onDraw(Graphics& g) {
    material();
    light();
    g.scale(scaleFactor);
    for (auto c : core) c.draw(g);
    for (auto p : particle) p.draw(g);
  }

  void onSound(AudioIO& io) {
    while (io()) {
      io.out(0) = 0;
      io.out(1) = 0;
    }
  }

  void onKeyDown(const ViewpointWindow&, const Keyboard& k) {
    Core& c = core[0];
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
      case '5':
	// Follow the core
        nav().nudgeToward(c.position, 1); 	
        Pose().faceToward(c.position, 1); 	
	break;
      case 't':
	c.velocity[1] += 30;
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
