
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

/////////////////////////////////////////////////////
// Sound Structures
struct Grain
{
  // Which input samples are in this grain?
  //
  int start;
  int stop;

  // What is the RMS of those samples?
  //
  float RMS;
};

bool compareGrainByRMS(const Grain &a, const Grain &b)
{
  return a.RMS < b.RMS;
}

// A new subclass of gam::SamplePlayer<> that implements only a single
// additional feature.
//
struct SafeSamplePlayer : gam::SamplePlayer<>
{
  // "operator[]" is the method for the array indexing operator, like when you
  // say samplePlayer[i]. The argument is an integer ("unsigned integer of 32
  // bits type") and the return value is float
  //
  float operator[](uint32_t i)
  {
    // Normally i must be less than size()
    //
    if (i >= size())
    {
      // if i is too big, explicitly return zero
      //
      return 0.;
    }
    else
    {
      // otherwise look up sample number i in the usual gam::SamplePlayer<> way
      //
      return gam::SamplePlayer<>::operator[](i);
    }
  }
};

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
  SafeSamplePlayer samplePlayer;
  SafeSamplePlayer GrainsPlayer[8];
  std::vector<Grain> grainArray;
  // Gamma
  static const int Nc = 9; // # of chimes
  static const int Nm = 5; // # of modes
  SineDs<> src;
  Accum<> timer;
  Chorus<> chr1, chr2; // chorusing for more natural beating

  // OSC + Cuttlebone
  Vec3f cell_gravity;
  State state;
  cuttlebone::Maker<State> maker;
  void printGrains()
  {
    int i = 0;
    for (std::vector<Grain>::iterator it = grainArray.begin();
         it != grainArray.end(); ++it)
    {
      std::cout << "  grainArray[" << i << "] = [start:" << it->start
                << ", stop:" << it->stop << ", RMS:" << it->RMS << "]" << endl;
      ++i;
    }
  }

  AlloApp()
      : maker(Simulator::defaultBroadcastIP()),
        InterfaceServerClient(Simulator::defaultInterfaceServerIP()),
        chr1(0.10),
        chr2(0.11)
  {

    // Gamma
    src.resize(Nc * Nm);
    timer.finish();
    // OSC
    cell_gravity = Vec3f(0, 0, 0);

    for (int j = 0; j < 7; j++)
    {
      oss << "planet_" << j << ".wav";
      string var = oss.str();
      samplePlayer.load(fullPathOrDie(var).c_str());
      int grainSize = samplePlayer.size() / NUM_GRAINS;
      if ((grainSize * NUM_GRAINS) < samplePlayer.size())
      {
        ++grainSize;
      }
      grainArray.resize(NUM_GRAINS);

      // Loop over all grains computing the RMS of each and saving it
      //
      for (int g = 0; g < NUM_GRAINS; ++g)
      {
        float sumOfSquares = 0;
        for (int i = 0; i < grainSize; ++i)
        {
          // Get sample number i from grain number g again we rely on it being OK
          // to read past the end of the buffer
          //
          float s = samplePlayer[g * grainSize + i];
          // Add the square of s to our running total
          //
          sumOfSquares += s * s;
        }

        // Now we've finished looping over all the sample in this block, so the
        // following code happens once per block, not once per sample.
        // sumOfSquares / grainSize is the mean squared sample value; the sqrt of
        // that is the RMS.
        //
        float RMS = sqrt(sumOfSquares / grainSize);

        // Record everything about this particular grain:
        // 1) Starting position (in samples) is g*grainsize;
        // 2) Ending position (in samples) is the starting position plus
        // grainSize-1
        // 3) The RMS of this grain is what we just computed
        grainArray[g].start = g * grainSize;
        grainArray[g].stop = g * grainSize + (grainSize - 1);
        grainArray[g].RMS = RMS;
      }
      // Now sort the grains in increasing order of RMS
      //
      sort(grainArray.begin(), grainArray.end(), compareGrainByRMS);
    //  cout << j << endl;
    //  printGrains();
      gam::Array<float> outArray;
      outArray.resize(NUM_GRAINS * grainSize, 0);

      for (int grain = 0; grain < NUM_GRAINS; ++grain)
      {
        Grain &g = grainArray[grain];
        int whereToPutThisGrain = grain * grainSize;
        for (int i = 0; i < grainSize; ++i)
        {
          outArray[whereToPutThisGrain + i] = samplePlayer[g.start + i];
        }
      }
      GrainsPlayer[j].buffer(outArray, samplePlayer.frameRate(), 1);
      samplePlayer.phase(0.99999);
      GrainsPlayer[j].phase(0.99999);

      oss.str("");
      oss.clear();
    }

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
    float s;
    float tmp = 0;
    while (io())
    {
      for (unsigned i = 0; i < 7; i++)
      {
        float sampForPlayback = 0.001 * GrainsPlayer[i]();
        tmp += sampForPlayback;
      }
      io.out(0) = io.out(1) = tmp;
      cout << tmp << endl;
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