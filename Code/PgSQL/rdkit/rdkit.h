// $Id$
//
//  Copyright (c) 2010, Novartis Institutes for BioMedical Research Inc.
//  All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met: 
//
//     * Redistributions of source code must retain the above copyright 
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following 
//       disclaimer in the documentation and/or other materials provided 
//       with the distribution.
//     * Neither the name of Novartis Institutes for BioMedical Research Inc. 
//       nor the names of its contributors may be used to endorse or promote 
//       products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
#ifndef _RDKIT_H_
#define _RDKIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "postgres.h"

typedef bytea Mol;

#define DatumGetMolP(x)		((Mol*)PG_DETOAST_DATUM(x))
#define DatumGetMolPCopy(x)	((Mol*)PG_DETOAST_DATUM_COPY(x))
#define MolPGetDatum(x)		(PointerGetDatum(x))

#define PG_GETARG_MOL_P(x) 	DatumGetMolP(PG_GETARG_DATUM(x))
#define PG_GETARG_MOL_P_COPY(x) DatumGetMolPCopy(PG_GETARG_DATUM(x))
#define PG_RETURN_MOL_P(x)	PG_RETURN_DATUM(MolPGetDatum(x))

typedef bytea BitmapFingerPrint;

#define DatumGetBitmapFingerPrintP(x)		((BitmapFingerPrint*)PG_DETOAST_DATUM(x))
#define DatumGetBitmapFingerPrintPCopy(x)	((BitmapFingerPrint*)PG_DETOAST_DATUM_COPY(x))
#define BitmapFingerPrintPGetDatum(x)		(PointerGetDatum(x))

#define PG_GETARG_BITMAPFINGERPRINT_P(x) 	DatumGetBitmapFingerPrintP(PG_GETARG_DATUM(x))
#define PG_GETARG_BITMAPFINGERPRINT_P_COPY(x) DatumGetBitmapFingerPrintPCopy(PG_GETARG_DATUM(x))
#define PG_RETURN_BITMAPFINGERPRINT_P(x)	PG_RETURN_DATUM(BitmapFingerPrintPGetDatum(x))

typedef bytea SparseFingerPrint;

#define DatumGetSparseFingerPrintP(x)		((SparseFingerPrint*)PG_DETOAST_DATUM(x))
#define DatumGetSparseFingerPrintPCopy(x)	((SparseFingerPrint*)PG_DETOAST_DATUM_COPY(x))
#define SparseFingerPrintPGetDatum(x)		(PointerGetDatum(x))

#define PG_GETARG_SPARSEFINGERPRINT_P(x) 	DatumGetSparseFingerPrintP(PG_GETARG_DATUM(x))
#define PG_GETARG_SPARSEFINGERPRINT_P_COPY(x) DatumGetSparseFingerPrintPCopy(PG_GETARG_DATUM(x))
#define PG_RETURN_SPARSEFINGERPRINT_P(x)	PG_RETURN_DATUM(SparseFingerPrintPGetDatum(x))

/*
 * GUC
 */
extern double getTanimotoLimit(void);
extern double getDiceLimit(void);

/*
 * From/to C/C++
 */

/* RDKit::ROMol */
typedef void * CROMol; 
void	freeCROMol(CROMol data);

CROMol constructROMol(Mol* data); 
Mol * deconstructROMol(CROMol data); 

CROMol parseMolText(char *data,bool asSmarts);
char *makeMolText(CROMol data, int *len,bool asSmarts);
bool isValidSmiles(char *data);
bool isValidSmarts(char *data);

int molcmp(CROMol i, CROMol a);

int MolSubstruct(CROMol i, CROMol a);

bytea *makeMolSign(CROMol data);

double MolAMW(CROMol i);
double MolLogP(CROMol i);
int MolHBA(CROMol i);
int MolHBD(CROMol i);
int MolNumAtoms(CROMol i);
int MolNumHeavyAtoms(CROMol i);


/* ExplicitBitVect */
typedef void * MolBitmapFingerPrint;
void	freeMolBitmapFingerPrint(MolBitmapFingerPrint data);

MolBitmapFingerPrint constructMolBitmapFingerPrint(BitmapFingerPrint *data);
BitmapFingerPrint * deconstructMolBitmapFingerPrint(MolBitmapFingerPrint data);
bytea * makeSignatureBitmapFingerPrint(MolBitmapFingerPrint data);

int MolBitmapFingerPrintSize(MolBitmapFingerPrint a);

double calcBitmapTanimotoSml(MolBitmapFingerPrint a, MolBitmapFingerPrint b);
double calcBitmapDiceSml(MolBitmapFingerPrint a, MolBitmapFingerPrint b);

/* SparseIntVect<boost::int32_t> */
typedef void * MolSparseFingerPrint;
void	freeMolSparseFingerPrint(MolSparseFingerPrint data);

MolSparseFingerPrint constructMolSparseFingerPrint(SparseFingerPrint *data);
SparseFingerPrint * deconstructMolSparseFingerPrint(MolSparseFingerPrint data);
bytea * makeSignatureSparseFingerPrint(MolSparseFingerPrint data, int numBits);
bytea * makeLowSparseFingerPrint(MolSparseFingerPrint data, int numInts);

double calcSparseTanimotoSml(MolSparseFingerPrint a, MolSparseFingerPrint b);
double calcSparseDiceSml(MolSparseFingerPrint a, MolSparseFingerPrint b);
double calcSparseStringDiceSml(const char *a, unsigned int sza, const char *b, unsigned int szb);
MolSparseFingerPrint  addSFP(MolSparseFingerPrint a, MolSparseFingerPrint b);
MolSparseFingerPrint  subtractSFP(MolSparseFingerPrint a, MolSparseFingerPrint b);


void countOverlapValues(bytea * sign, MolSparseFingerPrint data, int numBits,
        int * sum, int * overlapSum, int * overlapN);
void countLowOverlapValues(bytea * sign, MolSparseFingerPrint data, int numInts,
        int * querySum, int *keySum, int * overlapUp, int * overlapDown);
/*
 * Various mol -> fp transformation
 */

MolBitmapFingerPrint makeLayeredBFP(CROMol data);
MolBitmapFingerPrint makeRDKitBFP(CROMol data);
MolBitmapFingerPrint makeMorganBFP(CROMol data, int radius);
MolSparseFingerPrint makeMorganSFP(CROMol data, int radius);
MolSparseFingerPrint makeAtomPairSFP(CROMol data);
MolSparseFingerPrint makeTopologicalTorsionSFP(CROMol data);
MolBitmapFingerPrint makeAtomPairBFP(CROMol data);
MolBitmapFingerPrint makeTopologicalTorsionBFP(CROMol data);

/*
 * Indexes
 */

#define NUMBITS					(2048)
#define NUMRANGE				(120)

#define INTRANGEMAX				(0xff)
typedef struct IntRange {
	uint8		low;
	uint8		high;
} IntRange;

#define RDKitTanimotoStrategy   (1)
#define RDKitDiceStrategy       (2)
#define	RDKitContains			(3)
#define	RDKitContained			(4)
#define	RDKitEquals			(6)

bool calcConsistency(bool isLeaf, uint16 strategy, 
    double nCommonUp, double nCommonDown, double nKey, double nQuery);


/*
 *  Cache subsystem. Moleculas and fingerprints I/O is extremely expensive.
 */
struct MemoryContextData; /* forward declaration to prevent conflicts with C++ */
void* SearchMolCache( void *cache, struct MemoryContextData * ctx, Datum a, 
						Mol **m, CROMol *mol, bytea **sign);
void* SearchBitmapFPCache( void *cache, struct MemoryContextData * ctx, Datum a, 
							BitmapFingerPrint **f, MolBitmapFingerPrint *fp, bytea **val);
void* SearchSparseFPCache( void *cache, struct MemoryContextData * ctx, Datum a, 
							SparseFingerPrint **f, MolSparseFingerPrint *fp, bytea **val);

/*
void* SearchMolCache( void *cache, struct MemoryContextData * ctx, Datum a, Mol **m, CROMol *mol, MolFingerPrint *fp, bytea **val);
void* SearchFPCache( void *cache, struct MemoryContextData * ctx, Datum a, FingerPrint **f, MolFingerPrint *fp, bytea **val);
*/
#ifdef __cplusplus
}
#endif
#endif
