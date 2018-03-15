

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
#include "Gamma/Effects.h"
#include "allocore/math/al_Ray.hpp"
#include "allocore/math/al_Vec.hpp"
#include "common.hpp"

using namespace gam;

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
  // Sound Functions
  // Returns frequency ratio of a mode of a bar clamped at one end
float barClamp(float mode){
  float res = mode - 0.5;
  return 2.81f*res*res;
}

// Returns frequency ratio of a mode of a freely vibrating bar
float barFree(float mode){
  float res = mode + 0.5;
  return 0.441f*res*res;
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
  	ton = HSV( al::rnd::uniform() * M_PI , 0.1, 1);
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
  	ton = HSV( al::rnd::uniform() * M_PI , 0.1, 1);
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
	static const int Nc = 9; // # of chimes
	static const int Nm = 5; // # of modes
  SineDs<> src;
	Accum<> timer;
	Chorus<> chr1, chr2; // chorusing for more natural beating

   // OSC variables
  Vec3f cell_acc,  cell_vel, cell_pos;

  AlloApp() 
  :	chr1(0.10), chr2(0.11)
  {
    //Gamma
  	src.resize(Nc * Nm);
		timer.finish();
    // OSC
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
    initAudio();

    light.pos(0, 0, 100);
    nav().pos(0, 0, 100);

    planetVect.resize(planetCount);
    dustVect.resize(dustCount);  // make all the dusts
    constellVect.resize(stellCount);

    for (auto& s : constellVect) {
      s.pos(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS());
      s.pos() *= al::rnd::uniform(10.0, 300.0)* (int(rand() % 2) * 2 - 1);
    }

    for (auto& d : dustVect) {
      d.pos(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS());
      d.pos() *= al::rnd::uniform(10.0, 300.0)* (int(rand() % 2) * 2 - 1);
    }
    int i = 0;
    char str[128];
    for (auto& p : planetVect) {
      p.pos(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS());
      p.pos() *= al::rnd::uniform(400.0, 1000.0) * (int(rand() % 2) * 2 - 1);
      p.quat() = Quatd(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS(),
                       al::rnd::uniformS());
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
  // Audio 
  void onSound(AudioIOData& io) {
    gam::Sync::master().spu(audioIO().fps());
    while (io()) {
      if (timer()) {
          static double freqs[Nc] = {
					  scl::freq("c4"),
					  scl::freq("f4"), scl::freq("g4"), scl::freq("a4"), scl::freq("d5"),
					  scl::freq("f5"), scl::freq("g5"), scl::freq("a5"), scl::freq("d6"),
				  };

				int i = gam::rnd::uni(Nc);
				float f0 = freqs[i];
				float A = gam::rnd::uni(0.1,1.);

				//      osc #   frequency      amplitude               length
				src.set(i*Nm+0, f0*1.000,      A*1.0*gam::rnd::uni(0.8,1.), 1600.0/f0);
				src.set(i*Nm+1, f0*barFree(2), A*0.5*gam::rnd::uni(0.5,1.), 1200.0/f0);
				src.set(i*Nm+2, f0*barFree(3), A*0.4*gam::rnd::uni(0.5,1.),  800.0/f0);
				src.set(i*Nm+3, f0*barFree(4), A*0.3*gam::rnd::uni(0.5,1.),  400.0/f0);
				src.set(i*Nm+4, f0*barFree(5), A*0.2*gam::rnd::uni(0.5,1.),  200.0/f0);

				timer.period(gam::rnd::uni(0.01,1.5));
      }
			
      float2 s = src() * 0.1;

			s.x += chr1(s.x)*0.2;
			s.y += chr2(s.y)*0.2;

			io.out(0) = s.x;
			io.out(1) = s.y;
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