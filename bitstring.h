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

extern unsigned char BitNumber[256];
extern   signed char BitFirst[256];
/*
#define BASETYPE   int
#define PRINTBASETYPE(a) Mprintf( 0, "%08x",a);

#define BASETYPE   long long
#define PRINTBASETYPE(a) Mprintf( 0, "%016x",a);
*/

void InitBS();
int  Is0BS(BitString a);
int  Isnt0BS(BitString a);
int FindAnySet(BitString a);
int FindFirstSet( BitString bs );
int  NumberBitsInt(int a);
int  NumberBitsBS(BitString a);
void PrintBS(BitString a);
void PrintBitMaze(BitString a);
int  EqualBS(BitString a, BitString b);
int  AllBitsSetBS(BitString x, BitString bits);
void UnsetBS(BitString x, BitString bits);

void BitAndNotBS(BitString r, BitString a, BitString b);
void BitAndNotButOrBS(BitString r, BitString a, BitString b, BitString c);
void BitAndBS (BitString r, BitString a, BitString b);
void BitAndAndBS (BitString r, BitString a, BitString b, BitString c);
void BitAndAndNotBS (BitString r, BitString a, BitString b, BitString c);
void BitNandBS(BitString r, BitString a, BitString b);
void BitOrBS  (BitString r, BitString a, BitString b);
void BitOrAndEqBS(BitString a, BitString b, BitString c);
void BitNorBS (BitString r, BitString a, BitString b);
void BitNotBS (BitString r, BitString a);
void BitNotAndNotBS(BitString r, BitString a, BitString b);

void BitAndEqBS (BitString a, BitString b);
void BitNandEqBS(BitString a, BitString b);
void BitOrEqBS  (BitString a, BitString b);
void BitNorEqBS (BitString a, BitString b);
void BitAndNotButOrEqBS(BitString a, BitString b, BitString c);
void BitAndNotEqBS(BitString a, BitString b);
void BitAndNotAndNotBS(BitString r, BitString a, BitString b, BitString c);
void BitNotAndNotAndNotBS(BitString r, BitString a, BitString b, BitString c);


int  LogAndBS   (BitString a, BitString b);
int  LogAndNotBS(BitString a, BitString b);
int  LogAndNotAndNotBS(BitString a, BitString b, BitString c);
int  LogAndAndNotBS(BitString a, BitString b, BitString c);
int  LogOrBS    (BitString a, BitString b);
int  LogOrNotBS (BitString a, BitString b);
int  LogNorAndNotBS (BitString r, BitString a, BitString b);

#define Set0BS(a) memset(a,0,sizeof(BitString))
#define Set1BS(a) memset(a,0xff,sizeof(BitString))
#define CopyBS(to,from) memcpy(to,from,sizeof(BitString))
#define UnsetBitBS(a,p) \
	   ((a)[(p)/(sizeof(BASETYPE)*8)] \
	&=~ (((BASETYPE)1)<<((p)%(sizeof(BASETYPE)*8))))
#define SetBitBS(a,p) \
	   ((a)[(p)/(sizeof(BASETYPE)*8)] \
	|= (((BASETYPE)1)<<((p)%(sizeof(BASETYPE)*8))))
#define IsBitSetBS(a,p) \
	   ( ((a)[(p)/(sizeof(BASETYPE)*8)] \
	    &  ((BASETYPE)1)<<((p)%(sizeof(BASETYPE)*8))))
