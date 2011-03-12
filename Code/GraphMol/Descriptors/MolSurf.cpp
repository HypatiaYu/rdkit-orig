// $Id$
//
//  Copyright (C) 2007-2011 Greg Landrum
//
//   @@ All Rights Reserved @@
//  This file is part of the RDKit.
//  The contents are covered by the terms of the BSD license
//  which is included in the file license.txt, found at the root
//  of the RDKit source tree.
//

#include <GraphMol/RDKitBase.h>
#include <GraphMol/Descriptors/MolDescriptors.h>
#include <vector>

namespace RDKit{
  namespace Descriptors {
    double getLabuteAtomContribs(const ROMol &mol,
				 std::vector<double> &Vi,
				 double &hContrib,
				 bool includeHs,
				 bool force){
      TEST_ASSERT(Vi.size()==mol.getNumAtoms());
      if(!force && mol.hasProp("_labuteAtomContribs")){
	mol.getProp("_labuteAtomContribs",Vi);
	mol.getProp("_labuteAtomHContrib",hContrib);
	double res;
	mol.getProp("_labuteASA",res);
	return res;
      }
      unsigned int nAtoms=mol.getNumAtoms();
      std::vector<double> rads(nAtoms);
      for(unsigned int i=0;i<nAtoms;++i){
	rads[i]=PeriodicTable::getTable()->getRb0(mol.getAtomWithIdx(i)->getAtomicNum());
	Vi[i]=0.0;
      }

      for(ROMol::ConstBondIterator bondIt=mol.beginBonds();
	  bondIt!=mol.endBonds();++bondIt){
	const double bondScaleFacts[4]={.1,0,.2,.3};
	double Ri=rads[(*bondIt)->getBeginAtomIdx()];
	double Rj=rads[(*bondIt)->getEndAtomIdx()];
	double bij=Ri+Rj;
	if(!(*bondIt)->getIsAromatic()){
	  if((*bondIt)->getBondType()<4){
	    bij -= bondScaleFacts[(*bondIt)->getBondType()];
	  }
	} else {
	  bij -= bondScaleFacts[0];
	}
	double dij=std::min(std::max(fabs(Ri-Rj),bij),Ri+Rj);
	Vi[(*bondIt)->getBeginAtomIdx()] += Rj*Rj-(Ri-dij)*(Ri-dij)/dij;
	Vi[(*bondIt)->getEndAtomIdx()] += Ri*Ri-(Rj-dij)*(Rj-dij)/dij;
      }
      hContrib=0.0;
      if(includeHs){
	double Rj=PeriodicTable::getTable()->getRb0(1);
	for(unsigned int i=0;i<nAtoms;++i){
	  double Ri=rads[i];
	  double bij=Ri+Rj;
	  double dij=std::min(std::max(fabs(Ri-Rj),bij),Ri+Rj);
	  Vi[i] += Rj*Rj-(Ri-dij)*(Ri-dij)/dij;
	  hContrib += Ri*Ri-(Rj-dij)*(Rj-dij)/dij;
	}
      }

      double res=0.0;
      for(unsigned int i=0;i<nAtoms;++i){
	double Ri=rads[i];
	Vi[i] = M_PI*Ri*(4.*Ri-Vi[i]);
	res+=Vi[i];
      }
      if(includeHs){
	double Rj=PeriodicTable::getTable()->getRb0(1);
	hContrib = M_PI*Rj*(4.*Rj-hContrib);
	res+=hContrib;
      }
      mol.setProp("_labuteAtomContribs",Vi,true);
      mol.setProp("_labuteAtomHContrib",hContrib,true);
      mol.setProp("_labuteASA",res,true);

      return res;
    }
    double calcLabuteASA(const ROMol &mol,bool includeHs,bool force){
      if(!force && mol.hasProp("_labuteASA")){
	double res;
	mol.getProp("_labuteASA",res);
	return res;
      }
      std::vector<double> contribs;
      contribs.resize(mol.getNumAtoms());
      double hContrib;
      double res;
      res=getLabuteAtomContribs(mol,contribs,hContrib,includeHs,force);
      return res;
    }


    double getTPSAAtomContribs(const ROMol &mol,
                               std::vector<double> &Vi,
                               bool force){
      TEST_ASSERT(Vi.size()>=mol.getNumAtoms());
      double res=0;
      if(!force && mol.hasProp("_tpsaAtomContribs")){
	mol.getProp("_tpsaAtomContribs",Vi);
	mol.getProp("_tpsa",res);
	return res;
      }
      unsigned int nAtoms=mol.getNumAtoms();
      std::vector<int> nNbrs(nAtoms,0),nSing(nAtoms,0),nDoub(nAtoms,0),nTrip(nAtoms,0),nArom(nAtoms,0),nHs(nAtoms,0);
      for(ROMol::ConstBondIterator bIt=mol.beginBonds();bIt!=mol.endBonds();++bIt){
        const Bond *bnd=(*bIt);
        if(bnd->getBeginAtom()->getAtomicNum()==1){
          nNbrs[bnd->getEndAtomIdx()]-=1;
          nHs[bnd->getEndAtomIdx()]+=1;
        } else if(bnd->getEndAtom()->getAtomicNum()==1){
          nNbrs[bnd->getBeginAtomIdx()]-=1;
          nHs[bnd->getBeginAtomIdx()]+=1;
        } else if(bnd->getIsAromatic()){
          nArom[bnd->getBeginAtomIdx()]+=1;          
          nArom[bnd->getEndAtomIdx()]+=1;          
        } else {
          switch(bnd->getBondType()){
          case Bond::SINGLE:
            nSing[bnd->getBeginAtomIdx()]+=1;
            nSing[bnd->getEndAtomIdx()]+=1;
            break;
          case Bond::DOUBLE: 
            nDoub[bnd->getBeginAtomIdx()]+=1;
            nDoub[bnd->getEndAtomIdx()]+=1;
            break;
          case Bond::TRIPLE: 
            nTrip[bnd->getBeginAtomIdx()]+=1;
            nTrip[bnd->getEndAtomIdx()]+=1;
            break;
          }
        }
      }
      
      for(unsigned int i=0;i<nAtoms;++i){
        const Atom *atom=mol.getAtomWithIdx(i);
        int atNum=atom->getAtomicNum();
        if(atNum!=7 && atNum!=8) continue;
        nHs[i] += atom->getTotalNumHs();
        int chg=atom->getFormalCharge();
        bool isArom=atom->getIsAromatic();
        bool in3Ring = mol.getRingInfo()->isAtomInRingOfSize(i,3);
        nNbrs[i]+=atom->getDegree();

        double tmp=-1;
        if(atNum==7){
          switch(nNbrs[i]){
          case 1:
            if(nHs[i]==0 && chg==0 && nTrip[i]==1) tmp=23.79;
            else if(nHs[i]==1 && chg==0 && nDoub[i]==1) tmp=23.85;
            else if(nHs[i]==2 && chg==0 && nSing[i]==1) tmp=26.02;
            else if(nHs[i]==2 && chg==1 && nDoub[i]==1) tmp=25.59;
            else if(nHs[i]==3 && chg==1 && nSing[i]==1) tmp=27.64;
            break;
          case 2:
            if(nHs[i]==0 && chg==0 && nSing[i]==1 && nDoub[i]==1) tmp=12.36;
            else if(nHs[i]==0 && chg==0 && nTrip[i]==1 && nDoub[i]==1) tmp=13.60;
            else if(nHs[i]==1 && chg==0 && nSing[i]==2 && in3Ring) tmp=21.94;
            else if(nHs[i]==1 && chg==0 && nSing[i]==2 && !in3Ring) tmp=12.03;
            else if(nHs[i]==0 && chg==1 && nTrip[i]==1 && nSing[i]==1) tmp=4.36;
            else if(nHs[i]==1 && chg==1 && nDoub[i]==1 && nSing[i]==1) tmp=13.97;
            else if(nHs[i]==2 && chg==1 && nSing[i]==2) tmp=16.61;
            else if(nHs[i]==0 && chg==0 && nArom[i]==2) tmp=12.89;
            else if(nHs[i]==1 && chg==0 && nArom[i]==2) tmp=15.79;
            else if(nHs[i]==1 && chg==1 && nArom[i]==2) tmp=14.14;
            break;
          case 3:
            if(nHs[i]==0 && chg==0 && nSing[i]==3 && in3Ring ) tmp=3.01;
            else if(nHs[i]==0 && chg==0 && nSing[i]==3 && !in3Ring ) tmp=3.24;

            else if(nHs[i]==0 && chg==0 && nSing[i]==1 && nDoub[i]==2 ) tmp=11.68;
            else if(nHs[i]==0 && chg==1 && nSing[i]==2 && nDoub[i]==1 ) tmp=3.01;
            else if(nHs[i]==1 && chg==1 && nSing[i]==3 ) tmp=4.44;
            else if(nHs[i]==0 && chg==0 && nArom[i]==3 ) tmp=4.41;
            else if(nHs[i]==0 && chg==0 && nSing[i]==1 && nArom[i]==2 ) tmp=4.93;
            else if(nHs[i]==0 && chg==0 && nDoub[i]==1 && nArom[i]==2 ) tmp=8.39;
            else if(nHs[i]==0 && chg==1 && nArom[i]==3 ) tmp=4.10;
            else if(nHs[i]==0 && chg==1 && nSing[i]==1 && nArom[i]==2 ) tmp=3.88;
            break;
          case 4:
            if(nHs[i]==0 && nSing[i]==4 && chg==1) tmp=0.0;
            break;
          }
          if(tmp < 0.0){
            tmp = 30.5 - nNbrs[i] * 8.2 + nHs[i] * 1.5;
            if(tmp<0) tmp=0.0;
          }
        } else if(atNum==8){
          switch(nNbrs[i]){
          case 1:
            if(nHs[i]==0 && chg==0 && nDoub[i]==1) tmp=17.07;
            else if(nHs[i]==1 && chg==0 && nSing[i]==1 ) tmp=20.23;
            else if(nHs[i]==0 && chg==-1 && nSing[i]==1 ) tmp=23.06;
            break;
          case 2:
            if(nHs[i]==0 && chg==0 && nSing[i]==2 && in3Ring) tmp=12.53;
            else if(nHs[i]==0 && chg==0 && nSing[i]==2 && !in3Ring) tmp=9.23;
            else if(nHs[i]==0 && chg==0 && nArom[i]==2) tmp=13.14;
            break;
          }
          if(tmp<0.0){
            tmp=28.5-nNbrs[i]*8.6+nHs[i]*1.5;
            if(tmp<0) tmp=0.0;
          }
        }
        Vi[i]=tmp;
        res+=tmp;
      }

      mol.setProp("_tpsaAtomContribs",Vi,true);
      mol.setProp("_tpsa",res,true);
      return res;
    }
    double calcTPSA(const ROMol &mol,bool force){
      if(!force && mol.hasProp("_tpsa")){
	double res;
	mol.getProp("_tpsa",res);
	return res;
      }
      std::vector<double> contribs;
      contribs.resize(mol.getNumAtoms());
      double res;
      res=getTPSAAtomContribs(mol,contribs,force);
      return res;
    }
    
  } // end of namespace Descriptors
} // end of namespace RDKit
