#include <macosx_math.h>

int float_to_int (float a) {
  int maxint= 0x7ffffff;
  int minint=-0x8000000;
  if (a<maxint&&a>minint) return a;
  if (a>0) return maxint;
  if (a<0) return minint;
  return 0;
}
int double_to_int (double a) {
  int maxint= 0x7ffffff;
  int minint=-0x8000000;
  if (a<maxint&&a>minint) return a;
  if (a>0) return maxint;
  if (a<0) return minint;
  return 0;
}
#if 0
#if defined(__APPLE__) || defined(MACOSX)
extern "C" {
  char * ctermid_r(char *buf) {
    if (buf) {
      buf[0]='/';
      buf[1]='d';
      buf[2]='e';
      buf[3]='v';
      buf[4]='/';
      buf[5]='t';
      buf[6]='t';
      buf[7]='y';
      buf[8]='\0';
    }else {
      static char ret[]="/dev/tty";
      return ret;
    }
  }
}

float sqrtf(float v)
{
    return (float) sqrt((double)v);
}

float cosf (float v)
{
    return (float) cos((double)v);
}

float sinf (float v)
{
    return (float) sin((double)v);
}

float tanf (float v)
{
    return (float) tan((double)v);
}

float powf (float v, float p)
{
    return (float) pow((double)v, (double) p);
}

#endif
#endif
