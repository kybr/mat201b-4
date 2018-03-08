#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

struct MyApp : App, osc::PacketHandler {
  MyApp() {
    initWindow();
    initAudio();
    // I listen on 60777
    oscRecv().open(60777, "", 0.016, Socket::UDP);
    oscRecv().handler(*this);
    oscRecv().start();
    // you better be listening on 60777
    oscSend().open(60777, "169.231.8.171", 0.016, Socket::UDP);
    addSphere(them);
  }
  Mesh them;
  Pose other;
  void onAnimate(double dt) {
    Vec3f accel;
//    oscSend().send("/xyz", nav().pos().x, nav().pos().y, nav().pos().z);
    oscSend().send("/xyz", accel.x, accel.y, accel.z);
    oscSend().send("/wxyz", nav().quat().w, nav().quat().x, nav().quat().y, nav().quat().z);
  }
  void onMessage(osc::Message& m) {
    if (m.addressPattern() == "/xyz") {
      Vec3f o;
      m >> o.x;
      m >> o.y;
      m >> o.z;
      other.pos(o);
    } else if (m.addressPattern() == "/wxyz") {
      Quatf q;
      m >> q.x;
      m >> q.y;
      m >> q.z;
      m >> q.w;
      other.quat(q);
    } else
      m.print();
  }

  void onDraw(Graphics& g) {
    g.translate(other.pos());
    g.rotate(other.quat());
    g.draw(them);
  }
};

int main() { MyApp().start(); }
