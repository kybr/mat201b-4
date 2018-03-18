// MAT201B
// Winter 2018
// Author: Ben Myungin Lee.
//
// Cuttlebone "Laptop Graphics Renderer"
//

#include "common.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include "alloutil/al_Simulator.hpp"
#include "alloutil/al_OmniStereoGraphicsRenderer.hpp"

using namespace std;
using namespace al;

ostringstream oss;
Image image;
State state;

Mesh planetMesh, backMesh, dustMesh, constellMesh;
Texture cometTexture, backTexture;
Texture planetTexture[9];

string fullPathOrDie(string fileName, string whereToLook = ".") {
  SearchPaths searchPaths;
// XXX Path should be changed to work in different machines 
//  whereToLook = "/home/ben/Desktop/work/AlloSystem/mat201b/ben/final/media";
 // whereToLook = "../media/";

  searchPaths.addSearchPath(whereToLook);
  string filePath = searchPaths.find(fileName).filepath();
  //    cout << fileName << endl;
  if (filePath == "") {
    cout << fileName << endl;
    fprintf(stderr, "FAIL file import \n");
    exit(1);
  }
  return filePath;
}

struct Comet : Pose {
  Mesh comet;
  Comet (){
    // Background texture
    if (!image.load(fullPathOrDie("back.jpg"))) {
      fprintf(stderr, "FAIL\n");
      exit(1);    }
    backTexture.allocate(image.array());

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
    g.translate(state.comet_pose);
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
	g.color(1,1,1);
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
};

struct MyApp : OmniStereoGraphicsRenderer {
  bool simulate = true;
  Material material;
  Light light;
  Comet c;
  Constellation stell;
  Planet p;
  Dust d;

  State state;
  cuttlebone::Taker<State> taker;

  vector<Planet> planetVect;
  vector<Constellation> constellVect;
  vector<Dust> dustVect;

  MyApp() 
  {
    // Background space texture

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
  }

  void onAnimate(double dt) {
    
    taker.get(state); 
    static bool hasNeverHeardFromSim = true;
    if (taker.get(state) > 0) hasNeverHeardFromSim = false;
    if (hasNeverHeardFromSim) return;

    nav().pos(state.navPosition);
    nav().quat (state.navOrientation);
    
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
    // dust
    for (int i = 0 ; i < dustCount; i++){
      g.pushMatrix();
      g.translate(state.dust_pose[i]);
      g.color(1,1,1);
      g.draw(dustMesh);
      g.popMatrix();
    }

    // planet
    for (int i = 0 ; i < planetCount; i++){
      planetTexture[i].bind();
      g.pushMatrix();
      g.translate(state.planet_pose[i]);
      g.rotate(state.planet_quat[i]);
      g.draw(planetMesh);
      planetTexture[i].unbind();
      g.popMatrix();
    }

    for (int i = 0 ; i < stellCount; i++){
      g.pushMatrix();
      g.translate(state.stell_pose[i]);
      g.draw(planetMesh);
      g.popMatrix();
    }
  }
};

int main() { 
  MyApp app;
  app.taker.start();
  app.start();
}