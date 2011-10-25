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
#include "postgres.h"
#include "fmgr.h"
#include "utils/guc.h"

#include "rdkit.h"

static double rdkit_tanimoto_smlar_limit = 0.5;
static double rdkit_dice_smlar_limit = 0.5;
static bool rdkit_guc_inited = false;

static bool
TanimotoLimitAssign(double nlimit, bool doit, GucSource source)
{
  if (nlimit < 0 || nlimit > 1.0)
    return false;

  if (doit)
    rdkit_tanimoto_smlar_limit = nlimit;

  return true;
}

static bool
DiceLimitAssign(double nlimit, bool doit, GucSource source)                                                           
{                                                                                                                      
  if (nlimit < 0 || nlimit > 1.0)
    return false;

  if (doit)
    rdkit_dice_smlar_limit = nlimit;

  return true;
}                                        

static void
initRDKitGUC()
{
  if (rdkit_guc_inited)
    return;

  DefineCustomRealVariable(
                           "rdkit.tanimoto_threshold",
                           "Lower threshold of Tanimoto similarity",
                           "Molecules with similarity lower than threshold are not similar by % operation",
                           &rdkit_tanimoto_smlar_limit,
                           0.5,
                           0.0,
                           1.0,
                           PGC_USERSET,
                           0,
#if PG_VERSION_NUM >= 90100
                           (GucRealCheckHook)TanimotoLimitAssign,
                           NULL,
#else
                           TanimotoLimitAssign,
#endif
                           NULL
                           );

  DefineCustomRealVariable(
                           "rdkit.dice_threshold",
                           "Lower threshold of Dice similarity",
                           "Molecules with similarity lower than threshold are not similar by # operation",
                           &rdkit_dice_smlar_limit,
                           0.5,
                           0.0,
                           1.0,
                           PGC_USERSET,
                           0,
#if PG_VERSION_NUM >= 90100
                           (GucRealCheckHook)DiceLimitAssign,
                           NULL,
#else
                           DiceLimitAssign,
#endif
                           NULL
                           );

  rdkit_guc_inited = true;
}

double
getTanimotoLimit(void) {
  if (!rdkit_guc_inited)
    initRDKitGUC();

  return rdkit_tanimoto_smlar_limit;
}

double
getDiceLimit(void) {
  if (!rdkit_guc_inited)
    initRDKitGUC();

  return rdkit_dice_smlar_limit;
}

void _PG_init(void);
void
_PG_init(void) {
  initRDKitGUC();
}
