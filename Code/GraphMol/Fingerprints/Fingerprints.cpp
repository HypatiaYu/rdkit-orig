// $Id$
//
//  Copyright (C) 2003-2010 Greg Landrum and Rational Discovery LLC
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//

#include <GraphMol/RDKitBase.h>
#include <GraphMol/QueryOps.h>
#include <DataStructs/ExplicitBitVect.h>
#include <DataStructs/BitOps.h>
#include "Fingerprints.h"
#include <GraphMol/Subgraphs/Subgraphs.h>
#include <GraphMol/Subgraphs/SubgraphUtils.h>
#include <RDGeneral/Invariant.h>
#include <boost/random.hpp>
#include <limits.h>
#include <boost/cstdint.hpp>
#include <RDGeneral/hash/hash.hpp>
#include <algorithm>
#include <boost/dynamic_bitset.hpp>
//#define LAYEREDFP_USE_MT

namespace RDKit{
  namespace {
    bool isComplexQuery(const Bond *b){
      if( !b->hasQuery()) return false;
      // negated things are always complex:
      if( b->getQuery()->getNegation()) return true;
      std::string descr=b->getQuery()->getDescription();
      if(descr=="BondOrder") return false;
      if(descr=="BondAnd" || descr=="BondXor") return true;
      if(descr=="BondOr") {
        // detect the types of queries that appear for unspecified bonds in SMARTS:
        if(b->getQuery()->endChildren()-b->getQuery()->beginChildren()==2){
          for(Bond::QUERYBOND_QUERY::CHILD_VECT_CI child=b->getQuery()->beginChildren();
              child!=b->getQuery()->endChildren();++child){
            if((*child)->getDescription()!="BondOrder" || (*child)->getNegation())
              return true;
            if(static_cast<BOND_EQUALS_QUERY *>(child->get())->getVal()!=Bond::SINGLE &&
               static_cast<BOND_EQUALS_QUERY *>(child->get())->getVal()!=Bond::AROMATIC)
              return true;
            return false;
          }
        }
      }
      
      return true;
    }
    bool isComplexQuery(const Atom *a){
      if( !a->hasQuery()) return false;
      // negated things are always complex:
      if( a->getQuery()->getNegation()) return true;
      std::string descr=a->getQuery()->getDescription();
      if(descr=="AtomAtomicNum") return false;
      if(descr=="AtomOr" || descr=="AtomXor") return true;
      if(descr=="AtomAnd"){
        Queries::Query<int,Atom const *,true>::CHILD_VECT_CI childIt=a->getQuery()->beginChildren();
        if( (*childIt)->getDescription()=="AtomAtomicNum" &&
            ((*(childIt+1))->getDescription()=="AtomIsAliphatic" ||
             (*(childIt+1))->getDescription()=="AtomIsAromatic") &&
            (childIt+2)==a->getQuery()->endChildren()){
          return false;
        }
        return true;
      }
      
      return true;
    }
    bool isAtomAromatic(const Atom *a){
      bool res=false;
      if( !a->hasQuery()){
        res=a->getIsAromatic();
      } else {

        std::string descr=a->getQuery()->getDescription();
        if(descr=="AtomAtomicNum"){
          res = a->getIsAromatic();
        } else if(descr=="AtomIsAromatic") {
          res=true;
          if( a->getQuery()->getNegation()) res = !res;
        } else if(descr=="AtomIsAliphatic") {
          res=false;
          if( a->getQuery()->getNegation()) res = !res;
        } else if(descr=="AtomAnd"){
          Queries::Query<int,Atom const *,true>::CHILD_VECT_CI childIt=a->getQuery()->beginChildren();
          if( (*childIt)->getDescription()=="AtomAtomicNum"){
            if( a->getQuery()->getNegation()){
              res = false;
            } else if((*(childIt+1))->getDescription()=="AtomIsAliphatic"){
              res=false;
            } else if((*(childIt+1))->getDescription()=="AtomIsAromatic") {
              res=true;
            }
          }
        }
      }
      return res;
    }
  } // end of anonymous namespace

  // caller owns the result, it must be deleted
  ExplicitBitVect *RDKFingerprintMol(const ROMol &mol,unsigned int minPath,
                                     unsigned int maxPath,
                                     unsigned int fpSize,unsigned int nBitsPerHash,
                                     bool useHs,
                                     double tgtDensity,unsigned int minSize,
                                     bool branchedPaths){
    PRECONDITION(minPath!=0,"minPath==0");
    PRECONDITION(maxPath>=minPath,"maxPath<minPath");
    PRECONDITION(fpSize!=0,"fpSize==0");
    PRECONDITION(nBitsPerHash!=0,"nBitsPerHash==0");

    typedef boost::mt19937 rng_type;
    typedef boost::uniform_int<> distrib_type;
    typedef boost::variate_generator<rng_type &,distrib_type> source_type;
    rng_type generator(42u);

    //
    // if we generate arbitrarily sized ints then mod them down to the
    // appropriate size, we can guarantee that a fingerprint of
    // size x has the same bits set as one of size 2x that's been folded
    // in half.  This is a nice guarantee to have.
    //
    distrib_type dist(0,INT_MAX);
    source_type randomSource(generator,dist);

    ExplicitBitVect *res = new ExplicitBitVect(fpSize);

    INT_PATH_LIST_MAP allPaths;
    if(branchedPaths){
     allPaths = findAllSubgraphsOfLengthsMtoN(mol,minPath,maxPath,
                                              useHs);
    } else {
      allPaths = findAllPathsOfLengthsMtoN(mol,minPath,maxPath,
                                           useHs);
    }
    std::vector<const Bond *> bondCache;
    bondCache.resize(mol.getNumBonds());
    ROMol::EDGE_ITER firstB,lastB;
    boost::tie(firstB,lastB) = mol.getEdges();
    while(firstB!=lastB){
      BOND_SPTR bond = mol[*firstB];
      bondCache[bond->getIdx()]=bond.get();
      ++firstB;
    }
    
    boost::dynamic_bitset<> atomsInPath(mol.getNumAtoms());
    for(INT_PATH_LIST_MAP_CI paths=allPaths.begin();paths!=allPaths.end();paths++){
      for( PATH_LIST_CI pathIt=paths->second.begin();
	   pathIt!=paths->second.end();
	   pathIt++ ){
	const PATH_TYPE &path=*pathIt;
#ifdef VERBOSE_FINGERPRINTING        
        std::cerr<<"Path: ";
        std::copy(path.begin(),path.end(),std::ostream_iterator<int>(std::cerr,", "));
        std::cerr<<std::endl;
#endif
        // initialize the bond hashes to the number of neighbors the bond has in the path:
        std::vector<unsigned int> bondNbrs(path.size());
        std::fill(bondNbrs.begin(),bondNbrs.end(),0);
        atomsInPath.reset();
        std::vector<unsigned int> bondHashes;
        bondHashes.reserve(path.size()+1);
        for(unsigned int i=0;i<path.size();++i){
          const Bond *bi = bondCache[path[i]];
          atomsInPath.set(bi->getBeginAtomIdx());
          atomsInPath.set(bi->getEndAtomIdx());
          for(unsigned int j=i+1;j<path.size();++j){
            const Bond *bj = bondCache[path[j]];
            if(bi->getBeginAtomIdx()==bj->getBeginAtomIdx() ||
               bi->getBeginAtomIdx()==bj->getEndAtomIdx() ||
               bi->getEndAtomIdx()==bj->getBeginAtomIdx() ||
               bi->getEndAtomIdx()==bj->getEndAtomIdx() ){
              ++bondNbrs[i];
              ++bondNbrs[j];
            }
          }
#ifdef VERBOSE_FINGERPRINTING        
          std::cerr<<"   bond("<<i<<"):"<<bondNbrs[i]<<std::endl;
#endif
          // we have the count of neighbors for bond bi, compute its hash:
          unsigned int a1Hash,a2Hash;
          a1Hash = (bi->getBeginAtom()->getAtomicNum()%128)<<1 | bi->getBeginAtom()->getIsAromatic();
          a2Hash = (bi->getEndAtom()->getAtomicNum()%128)<<1 | bi->getEndAtom()->getIsAromatic();
          if(a1Hash<a2Hash) std::swap(a1Hash,a2Hash);
          unsigned int bondHash;
          if(bi->getIsAromatic()){
            // makes sure aromatic bonds always hash the same:
            bondHash = Bond::AROMATIC;
          } else {
            bondHash = bi->getBondType();
          }
          boost::uint32_t nBitsInHash=0;
          boost::uint32_t ourHash=bondNbrs[i]%8; // 3 bits here
          nBitsInHash+=3;
          ourHash |= (bondHash%16)<<nBitsInHash; // 4 bits here
          nBitsInHash+=4;
          ourHash |= a1Hash<<nBitsInHash; // 8 bits
          nBitsInHash+=8;
          ourHash |= a2Hash<<nBitsInHash; // 8 bits
          bondHashes.push_back(ourHash);
        }
        std::sort(bondHashes.begin(),bondHashes.end());

        // finally, we will add the number of distinct atoms in the path at the end
        // of the vect. This allows us to distinguish C1CC1 from CC(C)C
        bondHashes.push_back(atomsInPath.count());
        
        // hash the path to generate a seed:
	unsigned long seed = gboost::hash_range(bondHashes.begin(),bondHashes.end());

#ifdef VERBOSE_FINGERPRINTING        
        std::cerr<<" hash: "<<seed<<std::endl;
#endif
        // originally it seemed like a good idea to track hashes we've already
        // seen in order to avoid resetting them. In some benchmarking I did, that
        // seemed to actually result in a longer runtime (at least when using
        // an std::set to store the hashes)
        generator.seed(static_cast<rng_type::result_type>(seed));
        for(unsigned int i=0;i<nBitsPerHash;i++){
          unsigned int bit = randomSource();
          bit %= fpSize;
          res->setBit(bit);
#ifdef VERBOSE_FINGERPRINTING        
          std::cerr<<"   bit: "<<i<<" "<<bit<<std::endl;
#endif
        }
      }
    }

    // EFF: this could be faster by folding by more than a factor
    // of 2 each time, but we're not going to be spending much
    // time here anyway
    if(tgtDensity>0.0){
      while( static_cast<double>(res->getNumOnBits())/res->getNumBits() < tgtDensity &&
             res->getNumBits() >= 2*minSize ){
        ExplicitBitVect *tmpV=FoldFingerprint(*res,2);
        delete res;
        res = tmpV;
      }
    }
    
    return res;
  }

  // caller owns the result, it must be deleted
  ExplicitBitVect *LayeredFingerprintMol(const ROMol &mol,
                                         unsigned int layerFlags,
                                         unsigned int minPath,
                                         unsigned int maxPath,
                                         unsigned int fpSize,
                                         double tgtDensity,unsigned int minSize,
                                         std::vector<unsigned int> *atomCounts,
                                         ExplicitBitVect *setOnlyBits,
                                         bool branchedPaths){
    PRECONDITION(minPath!=0,"minPath==0");
    PRECONDITION(maxPath>=minPath,"maxPath<minPath");
    PRECONDITION(fpSize!=0,"fpSize==0");
    PRECONDITION(!atomCounts || atomCounts->size()>=mol.getNumAtoms(),"bad atomCounts size");
    PRECONDITION(!setOnlyBits || setOnlyBits->getNumBits()==fpSize,"bad setOnlyBits size");

    if(!mol.getRingInfo()->isInitialized()){
      MolOps::findSSSR(mol);
    }
    
#ifdef LAYEREDFP_USE_MT
    // create a mersenne twister with customized parameters. 
    // The standard parameters (used to create boost::mt19937) 
    // result in an RNG that's much too computationally intensive
    // to seed.
    typedef boost::random::mersenne_twister<boost::uint32_t,32,4,2,31,0x9908b0df,11,7,0x9d2c5680,15,0xefc60000,18, 3346425566U>  rng_type;
    
    typedef boost::uniform_int<> distrib_type;
    typedef boost::variate_generator<rng_type &,distrib_type> source_type;
    rng_type generator(42u);

    //
    // if we generate arbitrarily sized ints then mod them down to the
    // appropriate size, we can guarantee that a fingerprint of
    // size x has the same bits set as one of size 2x that's been folded
    // in half.  This is a nice guarantee to have.
    //
    distrib_type dist(0,INT_MAX);
    source_type randomSource(generator,dist);
#endif

    std::vector<const Bond *> bondCache;
    bondCache.resize(mol.getNumBonds());
    std::vector<short> isQueryBond(mol.getNumBonds(),0);
    ROMol::EDGE_ITER firstB,lastB;
    boost::tie(firstB,lastB) = mol.getEdges();
    while(firstB!=lastB){
      const Bond *bond = mol[*firstB].get();
      isQueryBond[bond->getIdx()] = 0x0;
      bondCache[bond->getIdx()]=bond;
      if(isComplexQuery(bond)){
        isQueryBond[bond->getIdx()] = 0x1;
      }
      if(isComplexQuery(bond->getBeginAtom())){
        isQueryBond[bond->getIdx()] |= 0x2;
      }
      if(isComplexQuery(bond->getEndAtom())){
        isQueryBond[bond->getIdx()] |= 0x4;
      }
      ++firstB;
    }

    std::vector<bool> aromaticAtoms(mol.getNumAtoms(),false);
    ROMol::VERTEX_ITER firstA,lastA;
    boost::tie(firstA,lastA) = mol.getVertices();
    while(firstA!=lastA){
      const Atom *atom = mol[*firstA].get();
      if(isAtomAromatic(atom)) aromaticAtoms[atom->getIdx()]=true;
      ++firstA;
    }
    
    ExplicitBitVect *res = new ExplicitBitVect(fpSize);
    INT_PATH_LIST_MAP allPaths;
    if(branchedPaths){
      allPaths = findAllSubgraphsOfLengthsMtoN(mol,minPath,maxPath);
    } else {
      allPaths = findAllPathsOfLengthsMtoN(mol,minPath,maxPath);
    }

    boost::dynamic_bitset<> atomsInPath(mol.getNumAtoms());
    for(INT_PATH_LIST_MAP_CI paths=allPaths.begin();paths!=allPaths.end();++paths){
      for( PATH_LIST_CI pathIt=paths->second.begin();
	   pathIt!=paths->second.end();
	   ++pathIt ){
	const PATH_TYPE &path=*pathIt;

        std::vector< std::vector<unsigned int> > hashLayers(maxFingerprintLayers);
        for(unsigned int i=0;i<maxFingerprintLayers;++i){
          if(layerFlags & (0x1<<i)) hashLayers[i].reserve(maxPath);
        }

        // details about what kinds of query features appear on the path:
        unsigned int pathQueries=0;
        //std::cerr<<" path: ";
        for(PATH_TYPE::const_iterator pIt=path.begin();pIt!=path.end();++pIt){
          pathQueries |= isQueryBond[*pIt];
          //std::cerr<< *pIt <<"("<<isQueryBond[*pIt]<<") ";
        }
        //std::cerr<<" : "<<pathQueries<<std::endl;


        // calculate the number of neighbors each bond has in the path:
        std::vector<unsigned int> bondNbrs(path.size(),0);
        atomsInPath.reset();
        for(unsigned int i=0;i<path.size();++i){
          const Bond *bi = bondCache[path[i]];
          atomsInPath.set(bi->getBeginAtomIdx());
          atomsInPath.set(bi->getEndAtomIdx());
          for(unsigned int j=i+1;j<path.size();++j){
            const Bond *bj = bondCache[path[j]];
            if(bi->getBeginAtomIdx()==bj->getBeginAtomIdx() ||
               bi->getBeginAtomIdx()==bj->getEndAtomIdx() ||
               bi->getEndAtomIdx()==bj->getBeginAtomIdx() ||
               bi->getEndAtomIdx()==bj->getEndAtomIdx() ){
              ++bondNbrs[i];
              ++bondNbrs[j];
            }
          }
#ifdef VERBOSE_FINGERPRINTING        
          std::cerr<<"   bond("<<i<<"):"<<bondNbrs[i]<<std::endl;
#endif
          // we have the count of neighbors for bond bi, compute its hash layers:
          unsigned int ourHash=0;

          if(layerFlags & 0x1){
            // layer 1: straight topology
            ourHash = bondNbrs[i]%8; // 3 bits here
            hashLayers[0].push_back(ourHash);
          }
          if(layerFlags & 0x2 && !(pathQueries&0x1) ){
            // layer 2: include bond orders:
            unsigned int bondHash;
            // makes sure aromatic bonds and single bonds  always hash the same:
            if(!bi->getIsAromatic() && bi->getBondType()!=Bond::SINGLE && bi->getBondType()!=Bond::AROMATIC){
              bondHash = bi->getBondType();
            } else {
              bondHash = Bond::SINGLE;
            }
            ourHash = bondHash%8;
            ourHash |= (bondNbrs[i]%8)<<6;
            
            hashLayers[1].push_back(ourHash);
          }
          if(layerFlags & 0x4 && !(pathQueries&0x6) ){
            //std::cerr<<" consider: "<<bi->getBeginAtomIdx()<<" - " <<bi->getEndAtomIdx()<<std::endl;
            // layer 3: include atom types:
            unsigned int a1Hash,a2Hash;
            a1Hash = (bi->getBeginAtom()->getAtomicNum()%128);
            a2Hash = (bi->getEndAtom()->getAtomicNum()%128);
            if(a1Hash<a2Hash) std::swap(a1Hash,a2Hash);
            ourHash = a1Hash;
            ourHash |= a2Hash<<7;
            ourHash |= (bondNbrs[i]%8)<<17;
            hashLayers[2].push_back(ourHash);
          }
          if(layerFlags & 0x8 && !(pathQueries&0x6) ){
            // layer 4: include ring information
            if(queryIsBondInRing(bi)){
              hashLayers[3].push_back(1);
            }
          }
          if(layerFlags & 0x10 && !(pathQueries&0x6) ){
            // layer 5: include ring size information
            ourHash = (queryBondMinRingSize(bi)%8);
            hashLayers[4].push_back(ourHash);
          }
          if(layerFlags & 0x20 && !(pathQueries&0x6) ){
            //std::cerr<<" consider: "<<bi->getBeginAtomIdx()<<" - " <<bi->getEndAtomIdx()<<std::endl;
            // layer 6: aromaticity:
            bool a1Hash = aromaticAtoms[bi->getBeginAtom()->getIdx()];
            bool a2Hash = aromaticAtoms[bi->getEndAtom()->getIdx()];

            if((!a1Hash) && a2Hash) std::swap(a1Hash,a2Hash);
            ourHash = a1Hash;
            ourHash |= a2Hash<<1;
            ourHash |= (bondNbrs[i]%8)<<5;
            hashLayers[5].push_back(ourHash);
          }
        }
        unsigned int l=0;
        bool flaggedPath=false;
        for(std::vector< std::vector<unsigned int> >::iterator layerIt=hashLayers.begin();
            layerIt!=hashLayers.end();++layerIt,++l){
          if(!layerIt->size()) continue;
          // ----
          std::sort(layerIt->begin(),layerIt->end());
        
          // finally, we will add the number of distinct atoms in the path at the end
          // of the vect. This allows us to distinguish C1CC1 from CC(C)C
          layerIt->push_back(atomsInPath.count());

          layerIt->push_back(l+1);

          // hash the path to generate a seed:
          unsigned long seed = gboost::hash_range(layerIt->begin(),layerIt->end());

          //std::cerr<<"  "<<l+1<<" "<<seed%fpSize<<std::endl;

#ifdef LAYEREDFP_USE_MT
          generator.seed(static_cast<rng_type::result_type>(seed));
          unsigned int bitId=randomSource()%fpSize;
#else
          unsigned int bitId=seed%fpSize;
#endif
          if(!setOnlyBits || (*setOnlyBits)[bitId]){
            res->setBit(bitId);
            if(atomCounts && !flaggedPath){
              for(unsigned int aIdx=0;aIdx<atomsInPath.size();++aIdx){
                if(atomsInPath[aIdx]){
                  (*atomCounts)[aIdx]+=1;
                }
              }
              flaggedPath=true;
            }
          }
        }
      }
      // EFF: this could be faster by folding by more than a factor
      // of 2 each time, but we're not going to be spending much
      // time here anyway
      if(tgtDensity>0.0){
        while( static_cast<double>(res->getNumOnBits())/res->getNumBits() < tgtDensity &&
               res->getNumBits() >= 2*minSize ){
          ExplicitBitVect *tmpV=FoldFingerprint(*res,2);
          delete res;
          res = tmpV;
        }
      }
    }
    return res;
  }



}
