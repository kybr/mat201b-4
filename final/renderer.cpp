
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

Mesh planetMesh, backMesh, dustMesh, constellMesh;
Texture cometTexture, backTexture;
Texture planetTexture[9];

struct Comet : Pose {
  Mesh comet;
  Comet() {
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
    g.scale(1 / scaleFactor);
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
  Comet c;
  Constellation stell;
  Planet p;
  Dust d;

  State state;
  cuttlebone::Taker<State> taker;

  vector<Planet> planetVect;
  vector<Constellation> constellVect;
  vector<Dust> dustVect;

  MyApp() {
    // Background space texture
    // Background texture
    if (!image.load(fullPathOrDie("back.jpg"))) {
      fprintf(stderr, "FAIL\n");
      exit(1);
    }
    backTexture.allocate(image.array());

    addSphereWithTexcoords(backMesh, 999);
    addSphereWithTexcoords(planetMesh, 50);
    // addSphere(planetMesh, 50);
    addSphere(constellMesh, 0.05);
    addSphere(dustMesh, 0.1);

    //  dust.primitive(Graphics::POINTS);

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

    for (auto& s : constellVect) {
      s.pos(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS());
      s.pos() *= al::rnd::uniform(10.0, 300.0) * (int(rand() % 2) * 2 - 1);
    }

    for (auto& d : dustVect) {
      d.pos(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS());
      d.pos() *= al::rnd::uniform(10.0, 300.0) * (int(rand() % 2) * 2 - 1);
    }
    int i = 0;
    char str[128];
    for (auto& p : planetVect) {
      p.pos(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS());
      p.pos() *= al::rnd::uniform(400.0, 1000.0) * (int(rand() % 2) * 2 - 1);
      p.quat() = Quatd(al::rnd::uniformS(), al::rnd::uniformS(),
                       al::rnd::uniformS(), al::rnd::uniformS());
      p.quat().normalize();
      p.vector_to_comet = c.pos() - p.pos();
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

    cout << "got here" << endl;
  }

  void ___onAnimate(double dt) {
    if (taker.get(state) > 0) {
      cout << "got here" << endl;
    }
    nav().pos(state.navPosition);
    nav().quat(state.navOrientation);

    nav().faceToward(c);
    c.quat() = nav();
    Vec3f v = (c.pos() - nav());
    float d = v.mag();
    c.pos() += (v / d) * (10 - d);
  }

  void onDraw(Graphics& g) {
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

    // shader().uniform("COLOR", Color(1));
    // shader().uniform("texture", 1.0);
    // shader().uniform("lighting", 0.5);
    // planetTexture[2].bind();
    // g.draw(planetMesh);
    // planetTexture[2].unbind();

    shader().uniform("COLOR", Color(1));
    shader().uniform("texture", 1.0);
    shader().uniform("lighting", 0.5);
    for (int i = 0; i < planetCount; i++) {
      g.pushMatrix();
      g.translate(state.planet_pose[i]);
      g.rotate(state.planet_quat[i]);
      planetTexture[i].bind();
      g.draw(planetMesh);
      planetTexture[i].unbind();
      g.popMatrix();
    }

    shader().uniform("texture", 0.0);
    shader().uniform("lighting", 0.0);
    shader().uniform("COLOR", Color(1));
    for (int i = 0; i < dustCount; i++) {
      g.pushMatrix();
      g.translate(state.dust_pose[i]);
      //      g.color(1, 1, 1);
      g.draw(dustMesh);
      g.popMatrix();
    }

    for (int i = 0; i < stellCount; i++) {
      g.pushMatrix();
      g.translate(state.stell_pose[i]);
      g.draw(planetMesh);
      g.popMatrix();
    }
  }

  void __onDraw(Graphics& g) {
    shader().uniform("texture", 1.0);
    shader().uniform("lighting", 0.0);
    shader().uniform("COLOR", Color(1));
    g.lighting(false);
    g.depthMask(false);
    /* */ g.pushMatrix();
    g.translate(nav().pos());
    backTexture.bind();
    g.draw(backMesh);
    backTexture.unbind();
    /* */ g.popMatrix();
    g.depthMask(true);
    g.lighting(true);

    material();
    light();
    // light.pos(nav().pos() - Vec3f(0, 0, 100));  // turns lighting back on

    // Object Draw
    // dust
    shader().uniform("texture", 0.0);
    shader().uniform("lighting", 0.0);
    shader().uniform("COLOR", Color(1));
    for (int i = 0; i < dustCount; i++) {
      g.pushMatrix();
      g.translate(state.dust_pose[i]);
      //      g.color(1, 1, 1);
      g.draw(dustMesh);
      g.popMatrix();
    }

    for (int i = 0; i < stellCount; i++) {
      g.pushMatrix();
      g.translate(state.stell_pose[i]);
      g.draw(planetMesh);
      g.popMatrix();
    }

    // planet
    shader().uniform("texture", 1.0);
    shader().uniform("lighting", 1.0);
    for (int i = 0; i < planetCount; i++) {
      planetTexture[i].bind();
      g.pushMatrix();
      g.translate(state.planet_pose[i]);
      g.rotate(state.planet_quat[i]);
      g.draw(planetMesh);
      planetTexture[i].unbind();
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
