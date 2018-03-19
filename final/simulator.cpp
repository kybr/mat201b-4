
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

#include "common.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include "alloutil/al_AlloSphereSpeakerLayout.hpp"
#include "alloutil/al_Simulator.hpp"
#include "allocore/io/al_ControlNav.hpp"
#include "Gamma/Gamma.h"
#include "Gamma/SamplePlayer.h"
#include "Gamma/Noise.h"
#include "Gamma/Oscillator.h"

using namespace gam;
using namespace al;
using namespace std;

ostringstream oss;
Image image;

Mesh planetMesh, backMesh, dustMesh, constellMesh;
Texture cometTexture, backTexture;
Texture planetTexture[9];

/////////////////////////////
// Visual Structures
struct Comet : Pose
{
  SoundSource *soundSource;
  Mesh comet;
  Comet()
  {
    // Comet texture
    if (!image.load(fullPathOrDie("comet.png")))
    {
      fprintf(stderr, "FAIL\n");
      exit(1);
    }
    addSphereWithTexcoords(comet, 10);
    cometTexture.allocate(image.array());
    soundSource = new SoundSource;
  }
  void onDraw(Graphics &g)
  {
    g.pushMatrix();
    g.translate(pos());
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
struct Planet : Pose
{
  SoundSource *soundSource;
  Vec3f vector_to_comet;
  Planet() { soundSource = new SoundSource; }
  void onDraw(Graphics &g)
  {
    g.pushMatrix();
    g.translate(pos());
    g.rotate(quat());
    g.draw(planetMesh);
    g.popMatrix();
  }
};

struct Constellation : Pose
{
  Vec3f position;
  Color ton;
  Constellation()
  {
    ton = HSV(al::rnd::uniform() * M_PI, 0.1, 1);
    int stellCount = 10;
  }
  void onDraw(Graphics &g)
  {
    g.pushMatrix();
    g.translate(pos());
    g.color(ton);
    g.draw(constellMesh);
    g.popMatrix();
  }
};

struct Dust : Pose
{
  SoundSource *soundSource;
  Vec3f position;
  Color ton;
  Dust()
  {
    soundSource = new SoundSource;
    ton = HSV(al::rnd::uniform() * M_PI, 0.1, 1);
  }

  void onDraw(Graphics &g)
  {
    g.pushMatrix();
    g.translate(pos());
    g.color(ton);
    g.draw(dustMesh);
    g.popMatrix();
  }
};

struct AlloApp : App, AlloSphereAudioSpatializer, InterfaceServerClient
{
  bool simulate = true;
  Material material;
  Light light;
  Comet c;
  Constellation stell;
  Planet p;
  Dust dust;

  // Visual
  vector<Planet> planetVect;
  vector<Constellation> constellVect;
  vector<Dust> dustVect;
  float distance_to_comet[8];
  float control, azi_control;

  // Audio
  SamplePlayer<float, gam::ipl::Linear, phsInc::Loop> grainArray_0, grainArray_1, grainArray_2, grainArray_3, grainArray_4, grainArray_5, grainArray_6;

  gam::OnePole<> smoothRate_0,smoothRate_1,smoothRate_2,smoothRate_3,smoothRate_4,smoothRate_5,smoothRate_6;

//  Mesh waveform, frame, region, cursor;
  double timer;

  // Gamma


  // OSC + Cuttlebone
  Vec3f cell_gravity;
  State state;
  cuttlebone::Maker<State> maker;

  AlloApp()
      : maker(Simulator::defaultBroadcastIP()),
        InterfaceServerClient(Simulator::defaultInterfaceServerIP())
  {
    //Sound
    timer = 0;
    /*
    smoothRate_0.freq(13.14159);
    smoothRate_1.freq(13.14159);
    smoothRate_2.freq(13.14159);
    smoothRate_3.freq(13.14159);
    smoothRate_4.freq(13.14159);
    smoothRate_5.freq(13.14159);
    smoothRate_6.freq(13.14159);
    */
    smoothRate_0.freq(100.14159);
    smoothRate_1.freq(100.14159);
    smoothRate_2.freq(100.14159);
    smoothRate_3.freq(100.14159);
    smoothRate_4.freq(100.14159);
    smoothRate_5.freq(100.14159);
    smoothRate_6.freq(100.14159);

    smoothRate_0 = 1.0;
    smoothRate_1 = 1.0;
    smoothRate_2 = 1.0;
    smoothRate_3 = 1.0; 
    smoothRate_4 = 1.0;
    smoothRate_5 = 1.0;
    smoothRate_6 = 1.0;

    // Gamma
    
    // OSC
    cell_gravity = Vec3f(0, 0, 0);
    
  // Import wav files for each planet
  // planet_0
  grainArray_0.load(fullPathOrDie("planet_0.wav").c_str());
  // planet_1
  grainArray_1.load(fullPathOrDie("planet_1.wav").c_str());
  // planet_2
  grainArray_2.load(fullPathOrDie("planet_2.wav").c_str());
  // planet_3
  grainArray_3.load(fullPathOrDie("planet_3.wav").c_str());
  // planet_4
  grainArray_4.load(fullPathOrDie("planet_4.wav").c_str());
  // planet_5
  grainArray_5.load(fullPathOrDie("planet_5.wav").c_str());
  // planet_6
  grainArray_6.load(fullPathOrDie("planet_6.wav").c_str());

  ///////////////////////////////////////////////////////
  // Visual
  // Background space texture
  if (!image.load(fullPathOrDie("back.jpg")))
  {
    fprintf(stderr, "FAIL\n");
    exit(1);
    }
    backTexture.allocate(image.array());

    addSphereWithTexcoords(backMesh, 999);
    addSphere(planetMesh, 50);
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

    // audio
    AlloSphereAudioSpatializer::initAudio();
    AlloSphereAudioSpatializer::initSpatialization();
    // if gamma
    gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
    // scene()->addSource(aSoundSource);
    // aSoundSource.dopplerType(DOPPLER_NONE);
    // scene()->usePerSampleProcessing(true);
    // scene()->usePerSampleProcessing(false);

    light.pos(0, 0, 100);
    nav().pos(0, 0, 100);

    planetVect.resize(planetCount);
    dustVect.resize(dustCount); // make all the dusts
    constellVect.resize(stellCount);

    for (auto &s : constellVect)
    {
      s.pos(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS());
      s.pos() *= al::rnd::uniform(10.0, 300.0) * (int(rand() % 2) * 2 - 1);
    }

    for (auto &du : dustVect)
    {
      du.pos(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS());
      du.pos() *= al::rnd::uniform(10.0, 300.0) * (int(rand() % 2) * 2 - 1);
    }
    int i = 0;
    char str[128];
    for (auto &p : planetVect)
    {
      p.pos(al::rnd::uniformS(), al::rnd::uniformS(), al::rnd::uniformS());
      p.pos() *= al::rnd::uniform(400.0, 1000.0) * (int(rand() % 2) * 2 - 1);
      p.quat() = Quatd(al::rnd::uniformS(), al::rnd::uniformS(),
                       al::rnd::uniformS(), al::rnd::uniformS());
      p.quat().normalize();
      p.vector_to_comet = c.pos() - p.pos();
      distance_to_comet[i] = p.vector_to_comet.mag();
      oss << "planet_" << i << ".jpg";
      string var = oss.str();
      if (!image.load(fullPathOrDie(var)))
      {
        fprintf(stderr, "FAIL\n");
        exit(1);
      }
      planetTexture[i].allocate(image.array());
      i += 1;
      oss.str("");
      oss.clear();
    }

    // OSC Receiver
    al::InterfaceServerClient::oscRecv().open(60777, "", 0.016, Socket::UDP);
    al::InterfaceServerClient::oscRecv().handler(*this);
    al::InterfaceServerClient::oscRecv().start();
  }

  void onAnimate(double dt)
  {
    timer += dt;
    // Granular synth
  if (timer > 0.1) {
      timer -= 0.1;

      float begin, end;
      int begintoend = 200;
      // Planet0
      for (int t = 0; t < begintoend; t++) {
        begin = al::rnd::uniform(grainArray_0.frames());
        end = al::rnd::uniform(grainArray_0.frames());
        if (abs(begin - end) < begintoend*2) break;
        if (abs(grainArray_0[int(begin)] - grainArray_0[int(end)]) < 0.125) break;  }
      if (begin > end) {
        float t = begin;
        begin = end;
        end = t;  }
      grainArray_0.min(begin);
      grainArray_0.max(end);
      float r = pow(1, al::rnd::uniformS(2.0f));
      if (al::rnd::prob(0.0)) r *= -1;
      smoothRate_0 = r;
      grainArray_0.reset();
      // Planet1
      for (int t = 0; t < begintoend; t++) {
        begin = al::rnd::uniform(grainArray_1.frames());
        end = al::rnd::uniform(grainArray_1.frames());
        if (abs(begin - end) < begintoend*2) break;
        if (abs(grainArray_1[int(begin)] - grainArray_1[int(end)]) < 0.125) break;  }
      if (begin > end) {
        float t = begin;
        begin = end;
        end = t;     }
      grainArray_1.min(begin);
      grainArray_1.max(end);
      r = pow(1, al::rnd::uniformS(2.0f));
      if (al::rnd::prob(0.0)) r *= -1;
      smoothRate_1 = r;
      grainArray_1.reset();
      // Planet2
      for (int t = 0; t < begintoend; t++) {
        begin = al::rnd::uniform(grainArray_2.frames());
        end = al::rnd::uniform(grainArray_2.frames());
        if (abs(begin - end) < begintoend*2) break;
        if (abs(grainArray_2[int(begin)] - grainArray_2[int(end)]) < 0.125) break;    }
      if (begin > end) {
        float t = begin;
        begin = end;
        end = t;      }
      grainArray_2.min(begin);
      grainArray_2.max(end);
      r = pow(1, al::rnd::uniformS(2.0f));
      if (al::rnd::prob(0.0)) r *= -1;
      smoothRate_2 = r;
      grainArray_2.reset();
      // Planet3
      for (int t = 0; t < begintoend; t++) {
        begin = al::rnd::uniform(grainArray_3.frames());
        end = al::rnd::uniform(grainArray_3.frames());
        if (abs(begin - end) < begintoend*2) break;
        if (abs(grainArray_3[int(begin)] - grainArray_3[int(end)]) < 0.125) break;    }
      if (begin > end) {
        float t = begin;
        begin = end;
        end = t;      }
      grainArray_3.min(begin);
      grainArray_3.max(end);
      r = pow(1, al::rnd::uniformS(2.0f));
      if (al::rnd::prob(0.0)) r *= -1;
      smoothRate_3 = r;
      grainArray_3.reset();
      // Planet4
      for (int t = 0; t < begintoend; t++) {
        begin = al::rnd::uniform(grainArray_4.frames());
        end = al::rnd::uniform(grainArray_4.frames());
        if (abs(begin - end) < begintoend*2) break;
        if (abs(grainArray_4[int(begin)] - grainArray_4[int(end)]) < 0.125) break;    }
      if (begin > end) {
        float t = begin;
        begin = end;
        end = t;      }
      grainArray_4.min(begin);
      grainArray_4.max(end);
      r = pow(1, al::rnd::uniformS(2.0f));
      if (al::rnd::prob(0.0)) r *= -1;
      smoothRate_4 = r;
      grainArray_4.reset();
      // Planet5
      for (int t = 0; t < begintoend; t++) {
        begin = al::rnd::uniform(grainArray_5.frames());
        end = al::rnd::uniform(grainArray_5.frames());
        if (abs(begin - end) < begintoend*2) break;
        if (abs(grainArray_5[int(begin)] - grainArray_5[int(end)]) < 0.125) break;    }
      if (begin > end) {
        float t = begin;
        begin = end;
        end = t;      }
      grainArray_5.min(begin);
      grainArray_5.max(end);
      r = pow(1, al::rnd::uniformS(2.0f));
      if (al::rnd::prob(0.0)) r *= -1;
      smoothRate_5 = r;
      grainArray_5.reset();
      // Planet6
      for (int t = 0; t < begintoend; t++) {
        begin = al::rnd::uniform(grainArray_6.frames());
        end = al::rnd::uniform(grainArray_6.frames());
        if (abs(begin - end) < begintoend*2) break;
        if (abs(grainArray_6[int(begin)] - grainArray_6[int(end)]) < 0.125) break;      }
      if (begin > end) {
        float t = begin;
        begin = end;
        end = t;      }
      grainArray_6.min(begin);
      grainArray_6.max(end);
      r = pow(1, al::rnd::uniformS(2.0f));
      if (al::rnd::prob(0.0)) r *= -1;
      smoothRate_6 = r;
      grainArray_6.reset();
    }

    while (InterfaceServerClient::oscRecv().recv())
      ;
    nav().faceToward(c);
    c.quat() = nav();
    Vec3f v = (c.pos() - nav());
    float d = v.mag();
    c.pos() += (v / d) * (10 - d);

    // OSC control
    if (control < -0.5)
      nav().moveR(-0.03);
    else if (control > 0.5)
      nav().moveR(+0.03);
    else
      nav().moveR(0);
    // in any cases, comet goes forward
    if (control)
      nav().moveF(0.05);

    if (azi_control < -0.5)
      nav().moveU(-0.02);
    else if (azi_control > 0.5)
      nav().moveU(+0.02);
    else
      nav().moveU(0);

    // for cuttlebone
    // Observer
    state.navPosition = nav().pos();
    state.navOrientation = nav().quat();
    // Comet
    state.comet_pose = c.pos();
    // Planets
    unsigned i = 0;
    for (auto &p : planetVect)
    {
      state.planet_pose[i] = p.pos();
      state.planet_quat[i] = p.quat();
      i++;
    }
    // dust
    i = 0;
    for (auto &dust : dustVect)
    {
      state.dust_pose[i] = dust.pos();
      i++;
    }
    // Constell
    i = 0;
    for (auto &stell : constellVect)
    {
      state.stell_pose[i] = stell.pos();
      i++;
    }
    maker.set(state);
  }
  void onDraw(Graphics &g)
  {
    // Background Mesh Draw
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
    light.pos(nav().pos() - Vec3f(0, 0, 100)); // turns lighting back on

    // Object Draw
    for (auto &d : dustVect)
      d.onDraw(g);
    int i = 0;
    for (auto &p : planetVect)
    {
      planetTexture[i].bind();
      p.onDraw(g);
      planetTexture[i].unbind();
      i += 1;
    }
    for (auto &s : constellVect)
      s.onDraw(g);
    c.onDraw(g);

    // OSC dynami                  cs
    control = cell_gravity.z;
    azi_control = cell_gravity.x;
    //  cout << azi_control << endl;
    cout << fixed;
    cout.precision(6);
  }

  ////////////////////////////
  //  Audio
  virtual void onSound(al::AudioIOData &io)
  {
    gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
    while (io())
    {
      float tmp = 0;
      grainArray_0.rate(smoothRate_0());
      grainArray_1.rate(smoothRate_1());
      grainArray_2.rate(smoothRate_2());
      grainArray_3.rate(smoothRate_3());
      grainArray_4.rate(smoothRate_4());
      grainArray_5.rate(smoothRate_5());
      grainArray_6.rate(smoothRate_6());
      float sl = ( grainArray_0() + grainArray_1() + grainArray_2() )  / 3;
      float sr = ( grainArray_3() + grainArray_4() + grainArray_5() + grainArray_6() ) / 4;

      io.out(0) = sl * 0.5;
      io.out(1) = sr * 0.5;
      listener()->pose(nav());
  //    scene()->render(io);
    }
  }
  void onMessage(osc::Message &m)
  {
    Vec3f o, r;
    if (m.addressPattern() == "/gyrosc/grav")
    {
      m >> o.x;
      m >> o.y;
      m >> o.z;
    }
    cell_gravity = o;
  }
};

int main()
{
  AlloApp app;
  app.AlloSphereAudioSpatializer::audioIO().start();
  app.InterfaceServerClient::connect();
  app.maker.start();
  app.start();
}