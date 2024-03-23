/*
** Copyright (C) 1999 by Andreas Junghanns.
**
** Permission to use, copy, modify, and distribute this software and its
** documentation for any purpose and without fee is hereby granted, provided
** that the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  This software is provided "as is" without express or
** implied warranty.
*/

#include "board.h"
unsigned char BitNumber[256];
  signed char BitFirst[256];

void InitBS() {
   int  i;
   char j;

   for ( i=0 ; i<256 ; i++ ) {
      BitNumber[i] = 0;
      BitFirst[i] = -1;
      for ( j=0 ; j<8 ; j++ )
          if ( i & (1<<j) ) BitNumber[i]++;
      for ( j=0 ; j<8 ; j++ )
          if ( i & (1<<j) ) {
	      BitFirst[i] = j;
	      break;
      }
   }
}

int  Is0BS(BitString a) {
	int i; for (i=0; i<NUMBERINTS; i++) if (a[i]!=0) return(0);
	return(1);
}

int Isnt0BS( BitString a ) {
  int i;

  for( i = 0; i < NUMBERINTS; i++ )
    if( a[ i ] )
      return 1;
  return 0;
}

int NumberBitsInt(int a) {
	int i,r=0;
	unsigned char *c = (unsigned char *) &a;
	for (i=0; i<sizeof(int); i++) r+=BitNumber[c[i]];
	return(r);
}

int NumberBitsBS(BitString a) {
	int i,r=0;
	unsigned char *c = (unsigned char *) a;
	for (i=0; i<sizeof(BitString); i++) r+=BitNumber[c[i]];
	return(r);
}

void  PrintBS(BitString a) {
	int i;
	Mprintf( 0, "-%i-\n",NUMBERINTS);fflush(stdout);
	PRINTBASETYPE(a[NUMBERINTS-1]);
	for (i=NUMBERINTS-2; i>=0; i--) {
		Mprintf( 0, "."); PRINTBASETYPE(a[i]);
	}
	Mprintf( 0, "\n");
}

void PrintBitMaze(BitString a) {
	int x,y;

	for (y = YSIZE-1; y>=0; y--) {
		for (x = 0; x<XSIZE; x++) {
			if (IsBitSetBS(a,XY2ID(x,y))) Mprintf( 0, "*");
			else Mprintf( 0, ".");
		}
		Mprintf( 0, "\n");
	}
	fflush(stdout);
}

int FindAnySet(BitString a)
{
	int x,y;

	for (y = YSIZE-1; y>=0; y--) {
		for (x = 0; x<XSIZE; x++) {
			if (IsBitSetBS(a,XY2ID(x,y))) return(XY2ID(x,y));
		}
	}
	return(0);
}

static char _foobits[ 256 ] = {
0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
7, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
6, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
5, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0,
4, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0
};

int FindFirstSet( BitString bs )
{
  int i, t;

  for( i = 0; i < NUMBERINTS; i++ )
    if( bs[ i ] ) {
      t = bs[ i ];
      i = i * sizeof( BASETYPE ) * 8;
      while( !( t & 255 ) ) {
	i += 8;
	t = t >> 8;
      }
      i += _foobits[ t & 255 ];
      return i;
    }
  return -1;
}


int  EqualBS(BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) if (a[i] != b[i]) return(0);
	return(1);
}

int  AllBitsSetBS(BitString x, BitString bits) {
  int i;

  for( i = 0; i < NUMBERINTS; i++ )
    if( ~x[ i ] & bits[ i ] )
      return 0;
  return 1;
}

void UnsetBS(BitString x, BitString bits) {
	int i; for (i=0; i<NUMBERINTS; i++) x[i] &= ~bits[i];
}

void BitAndNotBS(BitString r, BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) r[i] = a[i] & ~b[i];
}

void BitAndNotButOrBS(BitString r, BitString a, BitString b, BitString c) {
	int i; for (i=0; i<NUMBERINTS; i++) r[i] = a[i] & (~b[i] | c[i]);
}

void BitAndAndBS(BitString r, BitString a, BitString b, BitString c) {
	int i; for (i=0; i<NUMBERINTS; i++) r[i] = a[i] & b[i] & c[i];
}

void BitAndAndNotBS(BitString r, BitString a, BitString b, BitString c) {
	int i; for (i=0; i<NUMBERINTS; i++) r[i] = a[i] & b[i] & ~c[i];
}

void BitAndBS(BitString r, BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) r[i] = a[i] & b[i];
}

void BitNandBS(BitString r, BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) r[i] = ~(a[i] & b[i]);
}

void BitOrBS(BitString r, BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) r[i] = a[i] | b[i];
}

void BitOrAndEqBS(BitString a, BitString b, BitString c) {
	int i; for (i=0; i<NUMBERINTS; i++) a[i] |= b[i] & c[i];
}

void BitNorBS(BitString r, BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) r[i] = ~(a[i] | b[i]);
}

void BitNotBS(BitString r, BitString a) {
        int i; for (i=0; i<NUMBERINTS; i++) r[i] = ~a[i];
}

/******/

void BitAndEqBS(BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) a[i] &= b[i];
}

void BitNandEqBS(BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) a[i] = ~(a[i] & b[i]);
}

void BitOrEqBS(BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) a[i] |= b[i];
}

void BitNorEqBS(BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) a[i] = ~(a[i] | b[i]);
}

void BitAndNotEqBS(BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) a[i] &= ~b[i];
}

void BitAndNotButOrEqBS(BitString a, BitString b, BitString c) {
	int i; for (i=0; i<NUMBERINTS; i++) a[i] &= (~b[i] | c[i]);
}


void BitNotAndNotBS(BitString r, BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) r[i] = ~(a[i] | b[i]);
}

void BitNotAndNotAndNotBS(BitString r, BitString a, BitString b,
			  BitString c) {
	int i;

	for (i = 0; i < NUMBERINTS; i++) r[i] = ~( a[i] | b[i] | c[i] );
}

void BitAndNotAndNotBS(BitString r, BitString a, BitString b, BitString c) {
	int i;

	for (i = 0; i < NUMBERINTS; i++) r[i] = a[i] & ~b[i] & ~c[i];
}

/******/


int  LogAndBS (BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) if ((a[i] & b[i]) != 0) return(1);
	return(0);
}
int  LogAndNotBS(BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) if ((a[i] & ~b[i]) != 0) return(1);
	return(0);
}
int  LogAndAndNotBS(BitString a, BitString b, BitString c) {
	int i; for (i=0; i<NUMBERINTS; i++) if (((a[i] & b[i]) & ~c[i]) != 0)
		return(1);
	return(0);
}
int  LogAndNotAndNotBS(BitString a, BitString b, BitString c) {
	int i; for (i=0; i<NUMBERINTS; i++) if ((a[i] & ~b[i] & ~c[i]) != 0)
		return(1);
	return(0);
}
int  LogOrBS  (BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) if ((a[i] | b[i]) != 0) return(1);
	return(0);
}
int  LogOrNotBS (BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) if ((a[i] | ~b[i]) != 0) return(1);
	return(0);
}
int  LogNorAndNotBS (BitString r, BitString a, BitString b) {
	int i; for (i=0; i<NUMBERINTS; i++) if (((r[i]^a[i])&~b[i])!=0) return(1);
	return(0);
}

