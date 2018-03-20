// Winter 2018
// Author: Ben Myungin Lee.
//
// Cuttlebone "Laptop Graphics Renderer"
//

#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include "alloutil/al_OmniStereoGraphicsRenderer.hpp"
#include "alloutil/al_Simulator.hpp"
#include "common.hpp"

using namespace std;
using namespace al;

ostringstream oss;
Image image;
State state;

Mesh comet, planetMesh, backMesh, dustMesh, constellMesh;
Texture cometTexture, backTexture;
Texture planetTexture[9];

struct Planet : Pose {
  Vec3f vector_to_comet;
};

struct Constellation : Pose {
  Vec3f position;
  Color ton;
  Constellation() {
    ton = HSV(al::rnd::uniform() * M_PI, 0.1, 1);
    int stellCount = 10;
  }
  void onDraw(Graphics& g) {
    g.pushMatrix();
    g.translate(pos());
    g.color(1, 1, 1);
    g.draw(constellMesh);
    g.popMatrix();
  }
};

struct Dust : Pose {
  Vec3f position;
  Color ton;
  Dust() { ton = HSV(al::rnd::uniform() * M_PI, 0.1, 1); }
};

struct MyApp : OmniStereoGraphicsRenderer {
  bool simulate = true;
  Material material;
  Light light;
  //Comet c;
  Constellation stell;
  Planet p;
  Dust d;

  State state;
  cuttlebone::Taker<State> taker;

  vector<Planet> planetVect;
  vector<Constellation> constellVect;
  vector<Dust> dustVect;

  MyApp() {
    // Comet texture load
        // Comet texture
//    if (!image.load(fullPathOrDie("comet.png"))) {
    if (!image.load(fullPathOrDie("comet.jpg"))) {

      fprintf(stderr, "FAIL\n");
      exit(1);
    }
    cometTexture.allocate(image.array());

    // Background space texture
    if (!image.load(fullPathOrDie("back.jpg"))) {
      fprintf(stderr, "FAIL\n");
      exit(1);
    }
    backTexture.allocate(image.array());

    addSphereWithTexcoords(comet, 10);
    addSphereWithTexcoords(backMesh, 999);
    addSphereWithTexcoords(planetMesh, 50);
    addTetrahedron(dustMesh,0.05);
    addTetrahedron(constellMesh,0.1);
//    dustMesh.primitive(Graphics::POINTS);
//    constellMesh.primitive(Graphics::POINTS);

    comet.generateNormals();
    backMesh.generateNormals();
    planetMesh.generateNormals();
    constellMesh.generateNormals();
    dustMesh.generateNormals();

    lens().near(0.1);
    lens().far(1500);

    // initWindow();

    light.pos(0, 0, 100);
    nav().pos(0, 0, 100);

    planetVect.resize(planetCount);
    dustVect.resize(dustCount);  // make all the dusts
    constellVect.resize(stellCount);

    int i = 0;
    char str[128];
    for (auto& p : planetVect) {
      oss << "planet_" << i << ".jpg";
      string var = oss.str();
      if (!image.load(fullPathOrDie(var))) {
        fprintf(stderr, "FAIL\n");
        exit(1);
      }
      planetTexture[i].allocate(image.array());
      i += 1;
      oss.str("");
      oss.clear();
    }
  }

  void onAnimate(double dt) {
    taker.get(state);
    // pose = nav();
    pose = Pose(state.navPosition, state.navOrientation);
  }

  void onDraw(Graphics& g) {
   
   // comet 
    shader().uniform("COLOR", Color(1));
    shader().uniform("texture", 0.9);
    shader().uniform("lighting", 0.1);
    /* */g.pushMatrix();
    g.translate(state.comet_pose);
    g.rotate(state.comet_quat);
    cometTexture.bind();
    g.scale(scaleFactor);
    g.draw(comet);
    g.scale(1 / scaleFactor);
    cometTexture.unbind();
    /* */g.popMatrix();
// Position check
// cout << state.navPosition  << state.comet_pose << state.planet_pose[1]<< endl; 

    
// Back
    shader().uniform("COLOR", Color(1));
    shader().uniform("texture", 1.0);
    shader().uniform("lighting", 0.0);
    g.lighting(false);
    g.depthMask(false);
    /* */ g.pushMatrix();
    g.translate(nav().pos());
    g.rotate(nav().quat());
    backTexture.bind();
    g.draw(backMesh);
    backTexture.unbind();
    /* */ g.popMatrix();
    g.depthMask(true);
    g.lighting(true);


// Planet texture
    shader().uniform("COLOR", Color(1));
    shader().uniform("texture", 1.0);
    shader().uniform("lighting", 0.1);
    for (int i = 0; i < planetCount; i++) {
      g.pushMatrix();
      g.translate(state.planet_pose[i]);
      g.rotate(state.planet_quat[i]);
      planetTexture[i].bind();
      g.draw(planetMesh);
      planetTexture[i].unbind();
      g.popMatrix();
    }
  // Dusts & COnstellations
    shader().uniform("texture", 0.0);
    shader().uniform("lighting", 0.1);
    shader().uniform("COLOR", Color(HSV(al::rnd::uniform() * M_PI, 0.1, 1)));
    for (int i = 0; i < dustCount; i++) {
      g.pushMatrix();
      g.translate(state.dust_pose[i]);
      g.draw(dustMesh);
      g.popMatrix();
    }
    for (int i = 0; i < stellCount; i++) {
      g.pushMatrix();
      g.pointSize(1);
      g.blendAdd();
      g.translate(state.stell_pose[i]);
      g.draw(constellMesh);
      g.popMatrix();
    }
  }

  string vertexCode() {
    // XXX use c++11 string literals
    return R"(
varying vec4 color;
varying vec3 normal, lightDir, eyeVec;
void main() {
  color = gl_Color;
  vec4 vertex = gl_ModelViewMatrix * gl_Vertex;
  normal = gl_NormalMatrix * gl_Normal;
  vec3 V = vertex.xyz;
  eyeVec = normalize(-V);
  lightDir = normalize(vec3(gl_LightSource[0].position.xyz - V));
  gl_TexCoord[0] = gl_MultiTexCoord0;
  gl_Position = omni_render(vertex);
}
)";
  }

  string fragmentCode() {
    return R"(
uniform float lighting;
uniform float texture;
uniform vec4 COLOR;
uniform sampler2D texture0;
varying vec4 color;
varying vec3 normal, lightDir, eyeVec;
void main() {
  vec4 colorMixed;
  if (texture > 0.0) {
    vec4 textureColor = texture2D(texture0, gl_TexCoord[0].st);
    colorMixed = mix(color, textureColor, texture);
  } else {
    //colorMixed = color;
    colorMixed = COLOR;
  }
  vec4 final_color = colorMixed * gl_LightSource[0].ambient;
  vec3 N = normalize(normal);
  vec3 L = lightDir;
  float lambertTerm = max(dot(N, L), 0.0);
  final_color += gl_LightSource[0].diffuse * colorMixed * lambertTerm;
  vec3 E = eyeVec;
  vec3 R = reflect(-L, N);
  float spec = pow(max(dot(R, E), 0.0), 0.9 + 1e-20);
  final_color += gl_LightSource[0].specular * spec;
  gl_FragColor = mix(colorMixed, final_color, lighting);
}
)";
  }
};

int main() {
  MyApp app;
  app.taker.start();
  app.start();
}