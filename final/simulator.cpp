

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
float scaleFactor = 0.05;          // resizes the entire scene
// Planet const.
int planetCount = 8;
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

ostringstream oss;
Image image;

Mesh planetMesh, backMesh, dustMesh, constellMesh;
Texture cometTexture, backTexture;
Texture planetTexture[8];
// Frequent function: makes a random 3d vector 
Vec3d r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }

string fullPathOrDie(string fileName, string whereToLook = ".") {
  SearchPaths searchPaths;
//  whereToLook = "/home/ben/Desktop/work/AlloSystem/mat201b/media";
  whereToLook = "/home/ben/Desktop/work/AlloSystem/mat201b/ben/final/media";
 // whereToLook = "../media/";
  searchPaths.addSearchPath(whereToLook);
  string filePath = searchPaths.find(fileName).filepath();
  //    cout << fileName << endl;
  if (filePath == "") {
    fprintf(stderr, "FAIL file import \n");
    exit(1);
  }
  return filePath;
}

struct Comet : Pose {
  Mesh comet;
  Comet (){
        // Comet texture
    if (!image.load(fullPathOrDie("comet.png"))) {
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
//    cout << pos() << endl;
  }
};
struct Planet : Pose {
  Vec3f vector_to_comet;
  float distance_to_comet;

//    addSphereWithTexcoords(planet, 10);
//    cometTexture.allocate(image.array());
  void onDraw(Graphics& g) {
    g.pushMatrix();
    g.translate(pos());
    g.rotate(quat());
    g.draw(planetMesh);
    g.popMatrix();
  }
};

struct Constellation : Pose {
  Vec3f position;
  Color ton;
  Constellation() {
    position = r() * dustRange;
  	ton = HSV( rnd::uniform() * M_PI , 0.1, 1);
    int stellCount = 10;
  }
 void onDraw(Graphics& g) {
	g.pushMatrix();
	g.translate(pos());
	g.color(ton);
	g.draw(constellMesh);
	g.popMatrix();
 }
};
 
 struct Dust : Pose {
  Vec3f position;
  Color ton;
  Dust() {
  	ton = HSV( rnd::uniform() * M_PI , 0.1, 1);
  }

 void onDraw(Graphics& g) {
 	g.pushMatrix();
	g.translate(pos());
	g.color(ton);
	g.draw(dustMesh);
	g.popMatrix();
 }
};

struct AlloApp : App, osc::PacketHandler {
  bool simulate = true;
  Material material;
  Light light;
  Comet c;
  Constellation stell;
  Planet p;
  Dust d;

  vector<Planet> planetVect;
  vector<Constellation> constellVect;
  vector<Dust> dustVect;

  // Gamma
  gam::SineD<> sined;
  gam::Accum<> timer;
   // OSC variables
  Vec3f cell_acc,  cell_vel, cell_pos;

  AlloApp() {
    cell_vel = Vec3f(0,0,0);
    cell_pos = Vec3f(0,0,0);

    // Background space texture
    if (!image.load(fullPathOrDie("back.jpg"))) {
      fprintf(stderr, "FAIL\n");
      exit(1);    }
    backTexture.allocate(image.array());


    addSphereWithTexcoords(backMesh, 999);
    addSphere(planetMesh,50);
    addSphere(constellMesh, 0.05);
    addSphere(dustMesh, 0.1);
  
  //  dust.primitive(Graphics::POINTS);

    backMesh.generateNormals();
    planetMesh.generateNormals();
    constellMesh.generateNormals();
    dustMesh.generateNormals();

    lens().near(0.1);
    lens().far(1500);

    initWindow();
    light.pos(0, 0, 100);
    nav().pos(0, 0, 100);

    planetVect.resize(planetCount);
    dustVect.resize(dustCount);  // make all the dusts
    constellVect.resize(stellCount);

    for (auto& s : constellVect) {
      s.pos(rnd::uniformS(), rnd::uniformS(), rnd::uniformS());
      s.pos() *= rnd::uniform(10.0, 300.0)* (int(rand() % 2) * 2 - 1);
    }

    for (auto& d : dustVect) {
      d.pos(rnd::uniformS(), rnd::uniformS(), rnd::uniformS());
      d.pos() *= rnd::uniform(10.0, 300.0)* (int(rand() % 2) * 2 - 1);
    }
    int i = 0;
    char str[128];
    for (auto& p : planetVect) {
      p.pos(rnd::uniformS(), rnd::uniformS(), rnd::uniformS());
      p.pos() *= rnd::uniform(400.0, 1000.0) * (int(rand() % 2) * 2 - 1);
      p.quat() = Quatd(rnd::uniformS(), rnd::uniformS(), rnd::uniformS(),
                       rnd::uniformS());
      p.quat().normalize();
      p.vector_to_comet = c.pos() - p.pos();
      p.distance_to_comet = p.vector_to_comet.mag();
      oss << "planet_" << i << ".jpg";
      string var = oss.str();
      if (!image.load(fullPathOrDie(var))) {
        fprintf(stderr, "FAIL\n");
        exit(1);    }
      planetTexture[i].allocate(image.array());
      i += 1;
      oss.str("");
      oss.clear();
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
    light();
    light.pos(nav().pos() - (0, 0 , 100));  // turns lighting back on


    // Object Draw
    for (auto& d : dustVect) d.onDraw(g);
    int i = 0;
    for (auto& p : planetVect) {
      planetTexture[i].bind();
      p.onDraw(g);
      planetTexture[i].unbind();
      i += 1;
      }
    for (auto& s : constellVect) s.onDraw(g);
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
  //  cout << cell_pos.z << endl;//<< cell_vel << cell_pos << endl;
    //
  }

  void onSound(AudioIO& io) {
    Planet p;
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      if (timer()) {
          for (auto& p : planetVect) {
            


          }

          for (auto& s : constellVect) {



          }

          sined.set(500.0f - p.distance_to_comet * 6, 0.5f, 1.0f);
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