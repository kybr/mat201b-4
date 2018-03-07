#include "allocore/io/al_App.hpp"
using namespace al;
using namespace std;

struct MyApp : App {
  Image image;
  Mesh plane, cube, cylinder, fun, current, target, last;

  MyApp() {
    const char* filename = "mat201b/color_spaces/photoshop.png";
    // const char* filename = "mat201b/color_spaces/Tv-Snow.jpg";
    // const char* filename = "mat201b/color_spaces/double-rainbow.jpg";
    if (!image.load(filename)) {
      cerr << "ERROR" << endl;
      exit(1);
    }
    Array& array(image.array());

    cout << "array has " << (int)array.components() << " components" << endl;
    cout << "Array's type (as enum) is " << array.type() << endl;
    printf("Array's type (human readable) is %s\n",
           allo_type_name(array.type()));
    cout << "Array.print: " << endl << "   ";
    array.print();
    assert(array.type() == AlloUInt8Ty);

    //

    Image::RGBAPix<uint8_t> pixel;

    int W = array.width();
    int H = array.height();
    float aspectRatio = W / (float)H;
    cout << "aspectRatio: " << aspectRatio << endl;

    for (size_t row = 0; row < H; ++row) {
      for (size_t col = 0; col < W; ++col) {
        array.read(&pixel.r, col, row);  // XXX &pixel.r versus &pixel
        Color color(pixel.r / 256.0f, pixel.g / 256.0f, pixel.b / 256.0f, 0.6);

        current.color(color);

        //
        current.vertex(1.0f * col / W, aspectRatio * row / H, 0);
        plane.vertex(1.0f * col / W, aspectRatio * row / H, 0);
        last.vertex(1.0f * col / W, aspectRatio * row / H, 0);
        target.vertex(1.0f * col / W, aspectRatio * row / H, 0);

        //
        cube.vertex(color.r, color.g, color.b);

        {
          HSV c(color);
          cylinder.vertex(c.s * sin(2 * M_PI * c.h), c.v,
                          c.s * cos(2 * M_PI * c.h));
        }

        {
          Lab c(color);
          fun.vertex(c.l / 100.0f, c.a / 100.0f, c.b / 100.0f);
          // printf("%f %f %f\n", c.l, c.a, c.b);
        }
      }
    }

    nav().pos(0, 0, 7);
  }

  double t = 1000;  // an animation parameter and mode flag
  double angle = 0;
  void onAnimate(double dt) {
    if (t < 1) {
      for (int v = 0; v < current.vertices().size(); v++)
        current.vertices()[v] =
            target.vertices()[v] * t + last.vertices()[v] * (1 - t);
      // current.vertices()[v].lerp(target.vertices()[v], 0.5);
    } else if (t > 100)
      ;  // do nothing
    else {
      for (int v = 0; v < current.vertices().size(); v++)
        current.vertices()[v] = target.vertices()[v];
      t += 1000;
    }

    t += dt;
    angle += dt;
  }

  void onDraw(Graphics& g) {
    //
    g.rotate(angle * 33, Vec3f(0, 1, 0));
    g.scale(1.7);
    g.draw(current);
  }

  void onKeyDown(const ViewpointWindow&, const Keyboard& k) {
    switch (k.key()) {
      default:
      case '1':
        last = target;
        target = plane;
        t = 0;
        break;
      case '2':
        last = target;
        target = cube;
        t = 0;
        break;
      case '3':
        last = target;
        target = cylinder;
        t = 0;
        break;
      case '4':
        last = target;
        target = fun;
        t = 0;
        break;
    }
  }
};

int main() {
  MyApp app;
  app.initWindow();
  app.start();
}
