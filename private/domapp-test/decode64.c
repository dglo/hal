#include <stdio.h>

#include "base64.h"

int main(int argc, char *argv[]) {
   char in[8172];
   char out[8172];
   int nr = fread(in, 1, sizeof(in), stdin);

   if (nr>0) {
      int nd = base64DecodeBuffer(out, in, nr);
      if (nd>0) fwrite(out, 1, nd, stdout);
   }

   return 0;
}
