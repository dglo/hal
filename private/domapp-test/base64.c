#include <stdio.h>
#include <string.h>

/* use base64 incoding of output data...
 */

/* encode 6 bits...
 */
static char encode6Bits(unsigned char bits) {
  bits &= 0x3f;
  if (bits<26)  return 'A' + bits;
  if (bits<52)  return 'a' + (bits-26);
  if (bits<62)  return '0' + (bits-52);
  if (bits==62) return '+';
  return '/';
}

/* length of output vector in base64...
 */
int base64EncodeLength(const int n) {
   const int nn = 4 * ((n+3)/3);
   return nn + (nn+74)/75 + 1;
}

static int flushOne(char *output, char v, int n) {
   int ret = 1;

   if ( (n+1)%76 == 0 ) {
     *output = '\n'; output++;
     ret = 2;
   }

   *output = v;
   return ret;
}

/* encode output as base64...
 */
void base64EncodeBuffer(char *output, const unsigned char *input, int n) {
   int i;
   const int nn = n/3; /* unpadded... */
   int nw = 0;

   for (i=0; i<nn; i++, input+=3) {
      const unsigned char bits0 = input[0];
      const unsigned char bits1 = input[1];
      const unsigned char bits2 = input[2];
      const char e0 = encode6Bits( bits0>>2 );
      const char e1 = encode6Bits( ((bits0&3)<<4) | (bits1>>4) );
      const char e2 = encode6Bits( ((bits1&0xf)<<2) | (bits2>>6) );
      const char e3 = encode6Bits( bits2&0x3f );
      int nout;

      nout = flushOne(output, e0, nw);  nw += nout; output += nout;
      nout = flushOne(output, e1, nw);  nw += nout; output += nout;
      nout = flushOne(output, e2, nw);  nw += nout; output += nout;
      nout = flushOne(output, e3, nw);  nw += nout; output += nout;
   }

   /* now do extras... */
   if ( (n%3) != 0 ) {
      const unsigned char bits0 = input[0];
      const char e0 = encode6Bits(bits0>>2);
      int nout;

      nout = flushOne(output, e0, nw);  nw += nout; output += nout;
      
      if (n%3 == 1) {
	 const char e1 = encode6Bits( (bits0&3)<<4 );
	 nout = flushOne(output, e1, nw);   nw += nout; output += nout;
	 nout = flushOne(output, '=', nw);  nw += nout; output += nout;
      }
      else {
	 const unsigned char bits1 = input[1];
	 const char e1 = encode6Bits( ((bits0&3)<<4) | (bits1>>4) );
	 const char e2 = encode6Bits( ((bits1&0xf)<<2) );

	 nout = flushOne(output, e1, nw);   nw += nout; output += nout;
	 nout = flushOne(output, e2, nw);   nw += nout; output += nout;
	 nout = flushOne(output, '=', nw);  nw += nout; output += nout;
      }
   }

   if (nw>0) {
     *output = '\n'; output++; nw++;
   }
   *output = '\n'; output++; nw++;
}

/* returns: 0xfe if we received an '=' character, 
 *          0xff if invalid character decoded (empty char)...
 */
static unsigned char decode6Bits(char c) {
   if (c >= 'A' && c <= 'Z') return(c - 'A');
   if (c >= 'a' && c <= 'z') return(c - 'a' + 26);
   if (c >= '0' && c <= '9') return(c - '0' + 52);
   if (c == '+')             return 62;
   if (c == '/')             return 63;
   if (c == '=')             return 0xfe;
   return 0xff;
}

int base64DecodeBuffer(unsigned char *output,
			const char *input, int n) {
   int i, shift = 0;
   unsigned char sr = 0;
   int ret = 0;
   for (i=0; i<n; i++, input++) {
      const unsigned char v = decode6Bits(*input);

      if (v==0xfe) {
         shift = 0;
      }
      else if (v!=0xff) {
	 if (shift==0) {
	   sr = v<<2;
         }
	 else if (shift==1) {
	   sr |= (v>>4)&3;
	   output[ret] = sr; ret++;
	   sr = (v&0xf)<<4;
	 }
	 else if (shift==2) {
	   sr |= (v>>2)&0xf;
	   output[ret] = sr; ret++;
	   sr = (v&0x3)<<6;
	 }
	 else if (shift==3) {
	   sr |= v;
	   output[ret] = sr; ret++;
         }
	   
	 shift++;
	 shift&=3;
      }
   }
   return ret;
}

#if defined(TESTING)

int main(int argc, char *argv[]) {
   unsigned char buffer[4096];
   char encode[4096*2];
   int nr = fread(buffer, 1, sizeof(buffer), stdin);
  
  if (nr>=0) {
     const int len = base64EncodeLength(nr);
     base64EncodeBuffer(encode, buffer, nr);
     fwrite(encode, 1, len, stdout);
     memset(buffer, 0, sizeof(buffer));
     {  int nw = base64DecodeBuffer(buffer, encode, len);
        fwrite(buffer, 1, nw, stdout);
     }
  }

  return 0;
}
#endif
