#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base64.h"

int main(int argc, char *argv[]) {
   char *in = NULL;
   int n = 0, nalloc = 0;

   while (!feof(stdin) && !ferror(stdin)) {
      char b[65536];
      int nr = fread(b, 1, sizeof(b), stdin);
      if (nr>0) {
         if (nr + n > nalloc) {
            const int inc = sizeof(b)*2;
            if ((in = realloc(in, nalloc+inc))==NULL) {
               fprintf(stderr, "decode64: unable to realloc %d bytes\n", 
                       nalloc+inc);
               return 1;
            }
            nalloc += inc;
         }
         memcpy(in + n, b, nr);
         n+=nr;
      }
   }

   if (n>0) {
      unsigned char *out = (unsigned char *) malloc(n);
      if (out==NULL) {
         fprintf(stderr, "decode64: unable to malloc %d bytes\n", n);
         return 1;
      }
      
      {  int nd = base64DecodeBuffer(out, in, n);
         if (nd>0) fwrite(out, 1, nd, stdout);
      }
   }

   return 0;
}
