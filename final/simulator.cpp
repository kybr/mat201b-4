

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


//#include omnistreo stuff

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
float scaleFactor = 0.02;          // resizes the entire scene
// Planet const.
int planetCount = 23;
float planetRadius = 20;  // 
float planetRange = 500;

// Constellation const.
int stellCount = 10;
// Comet const.
float cometRadius = 20;
float steerFactor = 100;
// Dust const.
unsigned dustCount = 300;       
float dustRange = 1500;
float dustRadius = 0.5;
bool keys[4];

Mesh planet;
Mesh constell;    
Mesh dust;    

Image image;

Mesh planetMesh, backMesh;
Texture cometTexture, backTexture;
// Frequent function: makes a random 3d vector 
Vec3d r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

string fullPathOrDie(string fileName, string whereToLook = ".") {
  SearchPaths searchPaths;
//  whereToLook = "/home/ben/Desktop/work/AlloSystem/mat201b/media";
  whereToLook = "/home/ben/Desktop/work/AlloSystem/mat201b/ben/final/media";
 // whereToLook = "../media/";
  searchPaths.addSearchPath(whereToLook);
  string filePath = searchPaths.find(fileName).filepath();
  if (filePath == "") {
    fprintf(stderr, "FAIL2\n");
    exit(1);
  }
  return filePath;
}

struct Comet : Pose {
  Mesh comet;
  Comet (){
        // Comet texture
    if (!image.load(fullPathOrDie("comet.jpg"))) {
      fprintf(stderr, "FAIL\n");
      exit(1);
    }
    addSphereWithTexcoords(comet, 10);
//    addSphereWithTexcoords(comet, 999);
    cometTexture.allocate(image.array());
  }
  void onDraw(Graphics& g) {
    g.pushMatrix();
    g.translate(pos());
    g.rotate(quat());
    cometTexture.bind();
    g.scale(scaleFactor);
    g.draw(comet);
    g.scale(1/scaleFactor);
    cometTexture.unbind();
    g.popMatrix();
  }
};
struct Planet : Pose {
  Mesh planet;
  Vec3f vector_to_comet;
  float distance_to_comet;
  Planet (){
        // Planet texture
    if (!image.load(fullPathOrDie("comet.jpg"))) {
      fprintf(stderr, "FAIL\n");
      exit(1);
    }
    addSphereWithTexcoords(planet, 10);
    cometTexture.allocate(image.array());
  }
  void onDraw(Graphics& g) {
    g.pushMatrix();
    g.scale(scaleFactor);
    g.translate(pos());
    g.rotate(quat());
    g.draw(planetMesh);
    g.scale(1 / scaleFactor);
    g.popMatrix();
  }
};

struct Constellation {
  Vec3f position;
  Color ton;
  Constellation() {
    position = r() * dustRange;
  	ton = HSV( rnd::uniform() * M_PI , 0.1, 1);
    int stellCount = 10;
  }
 void draw(Graphics& g) {
	g.pushMatrix();
	g.translate(position);
	g.color(ton);
	g.draw(constell);
	g.popMatrix();
 }
};
 
 struct Dust : Pose {
  Vec3f position;
  Color ton;
  Mesh dust;

 void draw(Graphics& g) {
 	g.pushMatrix();
//  g.scale(scaleFactor);
	g.translate(pos());
//	g.color(ton);
	g.draw(dust);
//  g.scale(1/scaleFactor);
	g.popMatrix();
 }
};

struct AlloApp : App, osc::PacketHandler {
  bool simulate = true;
  Material material;
  Light light;
  Comet c;
  Constellation stell;
  vector<Planet> planet;
  vector<Constellation> constellation;
  vector<Dust> dusts;

  // Gamma
  gam::SineD<> sined;
  gam::Accum<> timer;
   // OSC variables
  Vec3f cell_acc,  cell_vel, cell_pos;

  AlloApp() {
    // Background space texture
    if (!image.load(fullPathOrDie("back.jpg"))) {
      fprintf(stderr, "FAIL\n");
      exit(1);
      cell_vel = Vec3f(0,0,0);
      cell_pos = Vec3f(0,0,0);
    }
    backTexture.allocate(image.array());

    addSphereWithTexcoords(backMesh, 999);
    addSphere(constell);
    addSphere(planetMesh);
    dust.primitive(Graphics::POINTS);

    backMesh.generateNormals();
    planetMesh.generateNormals();
    constell.generateNormals();
    dust.generateNormals();

    lens().near(0.1);
    lens().far(1000);

    initWindow();
    light.pos(0, 0, 100);
    nav().pos(0, 0, 100);

    planet.resize(planetCount);
    dusts.resize(dustCount);  // make all the dusts
    constellation.resize(stellCount);



    for (auto& d : dusts) {
      d.pos(rnd::uniformS(), rnd::uniformS(), rnd::uniformS());
      d.pos() *= rnd::uniform(-1000.0, 1000.0);
      //d.vertex(pos());
    }
    for (auto& p : planet) {
      p.pos(rnd::uniformS(), rnd::uniformS(), rnd::uniformS());
      p.pos() *= rnd::uniform(800.0, 800.0);
      p.quat() = Quatd(rnd::uniformS(), rnd::uniformS(), rnd::uniformS(),
                       rnd::uniformS());
      p.quat().normalize();
      p.vector_to_comet = c.pos() - p.pos();
      p.distance_to_comet = p.vector_to_comet.mag();

    }

    // OSC Receiver
    oscRecv().open(60777,"",0.016, Socket::UDP);
    oscRecv().handler(*this);
    oscRecv().start();
  }

  void onAnimate(double dt) {
    // cuttlebone::Taker<State> taker;
  // State* stae = new State;
    // taker.get(*state);
    // nav. state->pose ????
    nav().faceToward(c);
    c.quat() = nav();
    Vec3f v = (c.pos() - nav());
    float d = v.mag();
    c.pos() += (v / d) * (10 - d);
  }
  void onDraw(Graphics& g) {
    g.lighting(false);
    g.depthMask(false);
    g.pushMatrix();
    g.translate(nav().pos());
    backTexture.bind();
    g.draw(backMesh);
    backTexture.unbind();
    g.popMatrix();

    g.depthMask(true);
    material();
    light();  // turns lighting back on

//    for (auto constell : constellation ) constell.draw(g);
//    g.scale(scaleFactor);
    for (auto d : dusts) d.draw(g);
//    g.scale(1 / scaleFactor);

//    for (auto& p : planet) p.onDraw(g);
    c.onDraw(g);
  // OSC dynamics
    cell_vel.x += cell_acc.x;
    cell_vel.y += cell_acc.y;
    cell_vel.z += cell_acc.z;
    cell_pos.x +=  cell_vel.x;
    cell_pos.y +=  cell_vel.y;
    cell_pos.z +=  cell_vel.z;
    
    cout << fixed;
    cout.precision(6); 
    cout << cell_pos.z << endl;//<< cell_vel << cell_pos << endl;
    //


  }

  void onSound(AudioIO& io) {
    Planet p;
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      if (timer()) {
        // sined.set(rnd::uniform(220.0f, 880.0f), 0.5f, 1.0f);
//        for(int i=0 ; i < planetCount; i++){
          sined.set(500.0f - p.distance_to_comet * 6, 0.5f, 1.0f);
//        }
      }
      float s = sined();
      io.out(0) = s;
      io.out(1) = s;
    }
  }

  void onMessage(osc::Message& m) {
    Vec3f o, r;
/*
    if (m.addressPattern() == "/gyrosc/gyro") {
      m >> r.x;
      m >> r.y;
      m >> r.z; 
    }*/
    if (m.addressPattern() == "/gyrosc/accel") {
      m >> o.x;
      m >> o.y;
      m >> o.z;
    } 
    cell_acc = o;
    
/*
    cell_acc.x = o.x;
    cell_acc.y = o.y;
    cell_acc.z = o.z;
    cell_gyro.x = r.x;
    cell_gyro.y = r.y;
    cell_gyro.z = r.z;
*/
  }



  void onKeyDown(const ViewpointWindow&, const Keyboard& k) {
    switch (k.key()) {
      default:
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

int main() { AlloApp().start(); }