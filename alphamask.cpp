/*************************
  Rebecca Edson
  CPSC 4040 Assignment 3
  "Green Screening"
  October 6, 2019
*************************/


#include <OpenImageIO/imageio.h>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;
OIIO_NAMESPACE_USING


#define maximum(x, y, z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z)))
#define minimum(x, y, z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z)))

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


// Global variables
int width, height, channels;
rgba_pixel *pixmap;
double hMin, hMax, sMin, sMax, vMin, vMax;


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
  Input RGB colo r primary values : r , g, and b on scale 0 − 255
  Output HSV colors : h on scale 0−360, s and v on scale 0−1
*/
void RGBtoHSV(int r, int g, int b, double &h, double &s, double &v){

  double red, green, blue;
  double max, min, delta;

  // r, g, b to 0 − 1 scale
  red = r / 255.0;
  green = g / 255.0;
  blue = b / 255.0;

  max = maximum(red, green, blue);
  min = minimum(red, green, blue);

  v = max;  // value is maximum of r, g, b

  if (max == 0) {  //saturation and hue 0 if value is 0
    s = 0;
    h = 0;
  }
  else {
    s = (max - min) / max;  // saturation is color purity on scale 0 − 1
    delta = max - min;
    if (delta == 0)  // hue doesn’t matter if saturation is 0
      h = 0;
    else {
      if (red == max)  // otherwise, determine hue on scale 0 − 360
        h = (green - blue) / delta;
      else if (green == max)
        h = 2.0 + (blue - red) / delta;
      else  // ( blue == max)
        h = 4.0 + (red - green) / delta;
      h = h * 60.0;
      if(h < 0)
        h = h + 360.0;
    }
  }
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

  // get width and height for image
  const ImageSpec &spec = in->spec();
  width = spec.width;
  height = spec.height;
  channels = 4;  // final image will be RGBA

  // temporary pixmap to read RGB image
  rgb_pixel *pixels = new rgb_pixel[width*height];

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

  // store pixels in RGBA pixmap
  pixmap = new rgba_pixel[width*height];
  for(int i = 0; i < height; i++) {
    for(int j = 0; j < width; j++) {
      int k = i * width + j;
      pixmap[k].r = pixels[k].r;
      pixmap[k].g = pixels[k].g;
      pixmap[k].b = pixels[k].b;
    }
  }

}


/*
  Routine to set alpha channel to 0 for green in pixmap
*/
void removeAlpha() {

  for(int i = 0; i < height; i++) {
    for(int j = 0; j < width; j++) {
      int k = i * width + j;
      double h, s, v;
      // get HSV values for each pixel
      RGBtoHSV(pixmap[k].r, pixmap[k].g, pixmap[k].b, h, s, v);
      // hue: 60-150, saturation: >.40, value: >.30
      if(h >= hMin && h <= hMax && s >= sMax && v >= vMax) {
        pixmap[k].a = 0;  // transparent mask
      }
      else {
        pixmap[k].a = 255;  // opaque mask
      }
    }
  }

}


/*
  Routine to filter and balance green boundary pixels
*/
void reduceGreen() {

  for(int i = 0; i < height; i++) {
    for(int j = 0; j < width; j++) {
      int k = i * width + j;
      if(pixmap[k].a > 0) {
        int r = pixmap[k].r;
        int g = pixmap[k].g;
        int b = pixmap[k].b;
        double h, s, v;
        RGBtoHSV(r, g, b, h, s, v);
        if(h >= hMin && h <= hMax && s >= sMin && v >= vMin) {
          if((r * b) != 0 && (g * g) / (r * b) >= 1.5) {
            pixmap[k].r = r * 1.4;
            pixmap[k].g = g;
            pixmap[k].b = b * 1.4;
          }
          else {
            pixmap[k].r = r * 1.2;
            pixmap[k].g = g;
            pixmap[k].b = b * 1.2;
          }
        }
      }
    }
  }

}


/*
  Helper function to set radius for gradience
*/
void alphaRadius(int x, int y) {

  int k = x * width + y;
  float oldA = pixmap[k].a;
  float newA;
  int totalA = 0;
  int total_pix = 0;
  int r = 3;

  for(int j = y-r; j < y+r; j++) {
    for(int i = x-r; i < x+r; i++) {
      if(i >= 0 && i < width && j >= 0 && j < height){
        totalA += pixmap[k].a;
        total_pix++;
      }
    }
  }

  if(total_pix > 0)
    newA = totalA / total_pix;

  if(newA < oldA)
    pixmap[k].a = newA;

}


/*
  Routine to apply gradient transparency to boundary pixels
*/
void gradient() {

  for(int i = 0; i < height; i++) {
    for(int j = 0; j < width; j++) {
      alphaRadius(i, j);
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
  // file header will indicate image with dimensions of original image
  ImageSpec spec(width, height, channels, TypeDesc::UINT8);
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


int main(int argc, char *argv[]) {

  string infilename, outfilename, other;

  if(argc != 3) {
    cout << "ERROR: Command line must contain <executable> <input_filename.img> ";
    cout << "<output_filename>.png" << endl;
    exit(1);
  }
  else if(getFileExt(argv[2]) != "png") {
    cout << "ERROR: Image output file name must have extension '.png'" << endl;
    exit(1);
  }
  else {
    infilename = argv[1];
    outfilename = argv[2];
  }

  ifstream paramfile("param.txt");
  paramfile >> hMin >> hMax >> sMin >> sMax >> vMin >> vMax;

  readImage(infilename);
  removeAlpha();
  reduceGreen();
  gradient();
  writeImage(outfilename);

  return 0;

}
