

// Ben . 2018.03.05. CeleSphere ver.0.01
// Press t,f,g,h to control the Comet

/*  Copyright 2018 [Myungin Lee]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
   
	This program code contains creative work of game "CeleSphere."
  */



#include "allocore/io/al_App.hpp"
#include "Gamma/Oscillator.h"
#include "allocore/math/al_Ray.hpp"
#include "allocore/math/al_Vec.hpp"
#include "common.hpp"

using namespace al;
using namespace std;

// Physics const.
float maximumAcceleration = 5;  // prevents explosion, loss of planets
float dragFactor = 0.1;           //
float timeStep = 0.1;        // keys change this value for effect
float scaleFactor = 0.1;          // resizes the entire scene
// Planet const.
int planetCount = 23;
float planetRadius = 20;  // 
float planetRange = 500;
// Comet const.
float cometRadius = 20;
float steerFactor = 100;
// Dust const.
unsigned dustCount = 300;       
float dustRange = 1500;
float dustRadius1 = 0.5;
float dustRadius2 = 1;
bool keys[4];
Mesh comet; 
Mesh planet;
Mesh dust1;  
Mesh dust2;  

// helper function: makes a random vector
Vec3d r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

struct Comet {
  Vec3f position,velocity,acceleration;
  float abs_speed, unit_rotate, force, head, head_angv, vel_angle;
  Color ton;
  Mesh comet;
  Texture texture;
  Image image; 
  Comet() {
    position = Vec3f(0,0,0);
    velocity = Vec3f(0,0,0);
    head = 0;
    head_angv = 0;
    ton = HSV(0.5, 0.2, 1);
    unit_rotate = 0.02;
    force = 0.01;
    addSphereWithTexcoords(comet);
    SearchPaths searchPaths;
    searchPaths.addSearchPath("");
//    searchPaths.addSearchPath("/home/ben/Desktop/work/AlloSystem/mat201b/media");
//    searchPaths.addSearchPath("/home/ben/Desktop/work/AlloSystem/mat201b/ben/final");

    string filename = searchPaths.find("ben_comet.jpg").filepath();
    if (image.load(filename)) {
      cout << "Read image from " << filename << endl;
    } else {
      cout << "Failed to read image from " << filename << "!!!" << endl;
      exit(-1);
    }
    texture.allocate(image.array());
  }
  void draw(Graphics& g) {
    g.pushMatrix();
    g.scale(30);
    g.translate(position);
    g.rotate(head*180/M_PI,0,1,0);
    texture.bind();
    g.draw(comet);
    texture.unbind();
    g.popMatrix();
    head_angv *= 0.95;
    head += head_angv * timeStep;
    if (head > M_PI) {
      head -= 2*M_PI;
    }
    if (head < -M_PI) {
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

struct Stardust1 {
  Vec3f position;
  Color ton;
  Stardust1() {
    position = r() * dustRange;
  	ton = HSV( rnd::uniform() * M_PI , 0.1, 1);
  }
 void draw(Graphics& g) {
	g.pushMatrix();
	g.translate(position);
	g.color(ton);
	g.draw(dust1);
	g.popMatrix();
 }
};

struct Stardust2 {
  Vec3f position;
  Color ton;
  Stardust2() {
    position = r() * dustRange;
  	ton = HSV( rnd::uniform() * M_PI , 0.1, 1);
  }
 void draw(Graphics& g) {
	g.pushMatrix();
	g.translate(position);
	g.color(ton);
	g.draw(dust2);
	g.popMatrix();
 }
};

struct MyApp : App, osc::PacketHandler {
  Material material;
  Light light;
  Pose pose;
  Vec3f cell_vel = Vec3f(0,0,0);
  Vec3f camera_position = Vec3f(c.position.x - 10, c.position.y - 10,  c.position.z + 10);

  Vec3f diff_to_comet[23];
  float distance_to_comet[23];

  bool simulate = true;
  unsigned frameCount = 0;
  Planet p;
  Comet c;
  
  Stardust1 d1;
  Stardust2 d2;
  vector<Planet> planets;
  vector<Stardust1> dusts1;
  vector<Stardust2> dusts2;
  // Gamma
  gam::SineD<> sined;
  gam::Accum<> timer;

  MyApp() {
    Pose pose;
    Nav nav;
    Light light;
    Graphics g;
    addSphere(planet, planetRadius);
    addSphere(dust1, dustRadius1);
    addSphere(dust1, dustRadius2);

    planet.generateNormals();
    dust1.generateNormals();
    dust2.generateNormals();


//    light.pos(0, 0, 10);              // place the light
    nav.pos(0, 0, -100);             // place the init. viewer
    lens().far(400);                 // set the far clipping plane
    planets.resize(planetCount);  // make all the planets
    dusts1.resize(dustCount);  // make all the dusts1
    dusts2.resize(dustCount);  // make all the dusts2

    background(Color(0.07));

  //  nav.faceToward(c.position , 1);             // place the viewer
  // nav.nudgeToward(camera_position);  // Face toward the comet
  //  light.pos(c.position);


    initWindow();
    initAudio();
    oscRecv().open(60777,"",0.016, Socket::UDP);
    oscRecv().handler(*this);
    oscRecv().start();
  }

  void onAnimate(double dt) {	
//    Pose pose;
//    Nav nav;
    Light light;

    Graphics g;
    if (!simulate)      // skip the rest of this function
      return;
    
    // Euler's Method; Keep the time step small
    d1.position += c.velocity * timeStep * 0.7;
    d2.position += c.velocity * timeStep * 0.2;

    c.acceleration += c.velocity * -dragFactor;
    c.position += c.velocity * timeStep;
    c.abs_speed = sqrt( c.velocity[0] * c.velocity[0] + c.velocity[2] * c.velocity[2]);

    c.vel_angle = atan(c.velocity[0] / c.velocity[2]);
    camera_position[0] = c.position.x + ( cos(c.head) *100 ) ;
    camera_position[1] = c.position.y ;
    camera_position[2] = c.position.z + ( sin(c.head) *100 );
  //  cout << c.position << "," << camera_position <<"," << c.head << "," << c.vel_angle << endl;

  // Distance btw comet and planets
    for(int i=0 ; i < planetCount; i++){
      diff_to_comet[i] = c.position - p.position[i];
      distance_to_comet[i] = diff_to_comet[i].x * diff_to_comet[i].x + diff_to_comet[i].y*diff_to_comet[i].y + diff_to_comet[i].z*diff_to_comet[i].z; 
    }




    g.pushMatrix();
    nav().pos(camera_position);  // Face toward the comet
    nav().faceToward(c.position , 1);             // place the viewer

    light.pos(c.position);
    g.popMatrix();

    frameCount++;
  }

  void onDraw(Graphics& g) {    
    material();
    light();
    g.scale(scaleFactor);
    c.draw(g);
    for (auto p : planets) p.draw(g);
    for (auto d : dusts1) d.draw(g);
    for (auto d : dusts2) d.draw(g);


    keyCommand();
  }

  void onSound(AudioIO& io) {
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      if (timer()) {
        // sined.set(rnd::uniform(220.0f, 880.0f), 0.5f, 1.0f);
        for(int i=0 ; i < planetCount; i++){
          sined.set(500.0f - distance_to_comet[i] * 6, 0.5f, 1.0f);
        }
      }
      float s = sined();
      io.out(0) = s;
      io.out(1) = s;
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
 //   cout << c.head << endl; 
  }
  void onMessage(osc::Message& m) {
    Vec3f o;
    if (m.addressPattern() == "/gyrosc/accel") {
      m >> o.x;
      m >> o.y;
      m >> o.z;
//      pose.pos(o);
    } else
//    m.print();
    cell_vel.x += o.x;
    cell_vel.y += o.y;
    cell_vel.z += o.z;
    cout << "Velo" << cell_vel << "\n"<< "Accel" << o << endl;
//    cout << "Accel" << o << endl;

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
