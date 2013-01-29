//
//  Copyright (C) 2013 Greg Landrum
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//
#ifndef _RD_MOLFRAGMENTER_H__
#define _RD_MOLFRAGMENTER_H__

#include <istream>
#include <GraphMol/ROMol.h>

namespace RDKit {
  namespace MolFragmenter{
    typedef struct {
      unsigned int atom1Label,atom2Label;
      Bond::BondType bondType;
      ROMOL_SPTR query;
    } FragmenterBondType;


    //! \brief Fragments a molecule by breaking a set of bonds
    //!   
    /*!

      \param mol            - the molecule to be modified
      \param bondIndices    - indices of the bonds to be broken

      optional:
      \param addDummies  - toggles addition of dummy atoms to indicate where 
      bonds were broken
      \param dummyLabels - used to provide the labels to be used for the dummies.
      the first element in each pair is the label for the dummy
      that replaces the bond's beginAtom, the second is for the 
      dummy that replaces the bond's endAtom. If not provided, the
      dummies are labeled with atom indices.

      \return a new ROMol with the modifications
      The client is responsible for deleting this molecule.
      
    */
    ROMol *fragmentOnBonds(const ROMol &mol,const std::vector<unsigned int> &bondIndices,
                           bool addDummies=true,
                           const std::vector< std::pair<unsigned int,unsigned int> > *dummyLabels=0,
                           const std::vector< Bond::BondType > *bondTypes=0);

    ROMol *fragmentOnBonds(const ROMol &mol,const std::vector<FragmenterBondType> &bondPatterns);

    void constructFragmenterAtomTypes(std::istream *inStream,std::map<unsigned int,std::string> &defs,
                                      std::string comment="//",bool validate=true);
    void constructFragmenterAtomTypes(const std::string &str,std::map<unsigned int,std::string> &defs,
                                      std::string comment="//",bool validate=true);
    void constructBRICSAtomTypes(std::map<unsigned int,std::string> &defs);
    void constructFragmenterBondTypes(std::istream *inStream,
                                      const std::map<unsigned int,std::string> &atomTypes,
                                      std::vector<FragmenterBondType> &defs,
                                      std::string comment="//",bool validate=true,
                                      bool labelByConnector=true);
    void constructFragmenterBondTypes(const std::string &str,
                                      const std::map<unsigned int,std::string> &atomTypes,
                                      std::vector<FragmenterBondType> &defs,
                                      std::string comment="//",bool validate=true,
                                      bool labelByConnector=true);
    void constructBRICSBondTypes(std::vector<FragmenterBondType> &defs);
  }
}
#endif
