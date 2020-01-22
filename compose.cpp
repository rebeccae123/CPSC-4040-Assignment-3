/*************************
  Rebecca Edson
  CPSC 4040 Assignment 3
  "Green Screening"
  October 6, 2019
*************************/


#include <OpenImageIO/imageio.h>
#include <iostream>
#include <string>

#ifdef __APPLE__
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

using namespace std;
OIIO_NAMESPACE_USING


struct rgb_pixel {
  unsigned char r;
  unsigned char g;
  unsigned char b;
};

struct rgba_pixel {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
};


int aw, ah, bw, bh;
rgba_pixel *pixmap;
rgba_pixel *foreground;


/*
  Routine to get file extension from file name
*/
string getFileExt(const string &file) {

  size_t i = file.rfind('.', file.length());

  if(i != string::npos)
    return (file.substr(i+1, file.length()-1));

  return("");

}


/*
  Routine to read image file and store in pixmap
*/
void readImage(string infilename) {

  ImageInput *in = ImageInput::open(infilename);  // open file
  if(!in) {
    cerr << "Could not open: " << infilename << ", error = ";
    cerr << geterror() << endl;
    return;
  }

  // get width, height, and # of channels for image
  const ImageSpec &spec = in->spec();
  int w = spec.width;
  int h = spec.height;
  int channels = spec.nchannels;

  // temporary pixmap to read image
  unsigned char *pixels = new unsigned char[w*h*channels];

  // width and height for background image
  if(channels == 3) {  // RGB
    bw = w;
    bh = h;
  }
  // width and height for foreground image
  else {
    aw = w;
    ah = h;
  }

  // read image file, map pixels
  if(!in->read_image(TypeDesc::UINT8, &pixels[0])) {
    cerr << "Could not read pixels from: " << infilename << ", error = ";
    cerr << geterror() << endl;
    ImageInput::destroy(in);
    return;
  }

  // close file
  if(!in->close()) {
    cerr << "Error closing: " << infilename << ", error = ";
    cerr << geterror() << endl;
    ImageInput::destroy(in);
  }

  ImageInput::destroy(in);

  // store background image in pixmap
  if(channels == 3) {
    rgb_pixel *rgb_pixmap = (rgb_pixel *) pixels;
    pixmap = new rgba_pixel[bw*bh];
    for(int i = 0; i < bh; i++) {
      for(int j = 0; j < bw; j++) {
        int k = i * bw + j;
        pixmap[k].r = rgb_pixmap[k].r;
        pixmap[k].g = rgb_pixmap[k].g;
        pixmap[k].b = rgb_pixmap[k].b;
        pixmap[k].a = 255;
      }
    }
  }
  // store foreground image in pixmap
  else {
    foreground = (rgba_pixel *) pixels;
  }

}


/*
  Composite of pixmaps A over B
*/
void AoverB() {

  int w, h;

  if(aw > bw)  // foreground width larger than background
    w = bw;  // only iterate to background width
  else
    w = aw;  // vice versa

  if(ah > bh)  // height
    h = bh;
  else
    h = ah;

  for(int i = 0; i < h; i++) {
    for(int j = 0; j < w; j++) {
      int a = i * aw + j;  // pos in foreground pixmap
      int b = i * bw + j;  // pos in background pixmap
      if(foreground[a].a == 255) {  // foreground pixel is opaque
        pixmap[b] = foreground[a];  // pixel = foreground
      }
      else if(foreground[a].a > 0) {  // pixel neither opque nor transparent
        // convert channels to values 0-1
        double ar = foreground[a].r / 255.0;
        double ag = foreground[a].g / 255.0;
        double ab = foreground[a].b / 255.0;
        double aa = foreground[a].a / 255.0;
        double br = pixmap[b].g / 255.0;
        double bg = pixmap[b].g / 255.0;
        double bb = pixmap[b].b / 255.0;
        // calculate composite pixmap channel values
        pixmap[b].r = 255 * ((ar * aa) + ((1 - aa) * br));
        pixmap[b].g = 255 * ((ag * aa) + ((1 - aa) * bg));
        pixmap[b].b = 255 * ((ab * aa) + ((1 - aa) * bb));
      }
    }
  }

}


/*
  Routine to write pixmap to image file
*/
void writeImage(string outfilename) {

  // create oiio file handler for image
  ImageOutput *out = ImageOutput::create(outfilename);
  if(!out) {
    cerr << "Could not create: " << outfilename << ", error = ";
    cerr << OpenImageIO::geterror() << endl;
    return;
  }

  // open file for writing image
  // file header will indicate RGBA image with dimensions of background image
  ImageSpec spec(bw, bh, 4, TypeDesc::UINT8);
  if(!out->open(outfilename, spec)) {
    cerr << "Could not open " << outfilename << ", error = ";
    cerr << geterror() << endl;
    ImageOutput::destroy(out);
    return;
  }

  // write image to file
  if(!out->write_image(TypeDesc::UINT8, pixmap)) {  // pixmap
    cerr << "Could not close " << outfilename << ", error = ";
    cerr << geterror() << endl;
    ImageOutput::destroy(out);
    return;
  }

  ImageOutput::destroy(out);

}


/*
  Routine to display pixmap in current window
*/
void displayImage() {

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  glPixelZoom(1, -1);
  glRasterPos2i(-1, 1);

  glDrawPixels(bw, bh, GL_RGBA, GL_UNSIGNED_BYTE, pixmap);

  glFlush();

}


int main(int argc, char *argv[]) {

  string Afilename, Bfilename, outfilename;

  if(argc < 3) {
    cout << "ERROR: Command line must contain 2 images" << endl;
    exit(1);
  }
  else if(argc == 4) {
    if(getFileExt(argv[3]) != "png") {
      cout << "ERROR: Output image file name must have extension '.png'" << endl;
      exit(1);
    }
    outfilename = argv[3];
  }
  else if(argc > 4) {
    cout << "ERROR: Command line can only contain 2 images and output file" << endl;
    exit(1);
  }
  Afilename = argv[1];
  Bfilename = argv[2];

  readImage(Afilename);
  readImage(Bfilename);
  AoverB();
  if(argc == 4)
    writeImage(outfilename);

  glutInit(&argc, argv);

  glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
  glutInitWindowSize(bw, bh);
  glutCreateWindow("Composite Image");

  glutDisplayFunc(displayImage);

  glutMainLoop();

  return 0;

}
