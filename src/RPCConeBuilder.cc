// -*- C++ -*-
//
// Package:    RPCConeBuilder
// Class:      RPCConeBuilder
// 
/**\class RPCConeBuilder RPCConeBuilder.h L1TriggerConfig/RPCConeBuilder/src/RPCConeBuilder.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Tomasz Maciej Frueboes
//         Created:  Fri Feb 22 13:57:06 CET 2008
// $Id: RPCConeBuilder.cc,v 1.1 2008/03/03 14:30:11 fruboes Exp $
//
//


// system include files

// user include files

#include "L1TriggerConfig/RPCTriggerConfig/interface/RPCConeBuilder.h"
#include "L1TriggerConfig/RPCTriggerConfig/interface/RPCStripsRing.h"

//#include "L1TriggerConfig/RPCConeBuilder/interface/RPCConeBuilder.h"
//#include "L1TriggerConfig/RPCConeBuilder/interface/RPCStripsRing.h"

#include "FWCore/Framework/interface/ModuleFactory.h"



#include "Geometry/Records/interface/MuonGeometryRecord.h"

#include <sstream>
#include <vector>

#include "DataFormats/MuonDetId/interface/RPCDetId.h"

RPCConeBuilder::RPCConeBuilder(const edm::ParameterSet& iConfig) :
      m_towerBeg(iConfig.getParameter<int>("towerBeg")),
      m_towerEnd(iConfig.getParameter<int>("towerEnd")),
      m_rollBeg(iConfig.getParameter<int>("rollBeg")),
      m_rollEnd(iConfig.getParameter<int>("rollEnd")),
      m_hwPlaneBeg(iConfig.getParameter<int>("hwPlaneBeg")),
      m_hwPlaneEnd(iConfig.getParameter<int>("hwPlaneEnd"))
{
   
   setWhatProduced(this, dependsOn( &RPCConeBuilder::geometryCallback ));

   for (int i = m_towerBeg; i <= m_towerEnd; ++i){
      
      std::stringstream name;
      name << "lpSizeTower" << i;
      
      L1RPCConeBuilder::TLogPlaneSize newSizes = 
            iConfig.getParameter<std::vector<int> >(name.str().c_str());
      
      m_LPSizesInTowers.push_back(newSizes);
      
   }
   
   for (int roll = m_rollBeg; roll <= m_rollEnd; ++roll){
      L1RPCConeBuilder::THWplaneToTower newHwPlToTower;
      L1RPCConeBuilder::THWplaneToLP newHWplaneToLP;
      for (int hwpl = m_hwPlaneBeg; hwpl <= m_hwPlaneEnd; ++hwpl){
         std::stringstream name;
         name << "rollConnLP_" << roll << "_" << hwpl;
         L1RPCConeBuilder::TTowerList newListLP = 
               iConfig.getParameter<std::vector<int> >(name.str().c_str());
         newHWplaneToLP.push_back(newListLP);
         
         
         std::stringstream name1;
         name1 << "rollConnT_" << roll << "_" << hwpl;
         L1RPCConeBuilder::TLPList newListT = 
               iConfig.getParameter<std::vector<int> >(name1.str().c_str());
         newHwPlToTower.push_back(newListT);
      }
      m_RingsToTowers.push_back(newHwPlToTower);
      m_RingsToLP.push_back(newHWplaneToLP);
   }
   
}




//
// member functions
//

// ------------ method called to produce the data  ------------
RPCConeBuilder::ReturnType
RPCConeBuilder::produce(const L1RPCConeBuilderRcd& iRecord)
//RPCConeBuilder::produce(const L1RPCConfigRcd& iRecord)
{

  
   //std::cout << " RPCConeBuilder::produce called " << std::endl;
   using namespace edm::es;
   boost::shared_ptr<L1RPCConeBuilder> pL1RPCConeBuilder( ( new L1RPCConeBuilder ) );
   
   pL1RPCConeBuilder->setFirstTower(m_towerBeg);
   pL1RPCConeBuilder->setLastTower(m_towerEnd);
   
   pL1RPCConeBuilder->setLPSizeForTowers(m_LPSizesInTowers);
   pL1RPCConeBuilder->setRingsToLP(m_RingsToLP);

   
   edm::ESHandle<RPCGeometry> rpcGeom;
   iRecord.getRecord<MuonGeometryRecord>().get(rpcGeom);   
   
   buildCones(rpcGeom);
   
   // getConnectionsMap() returns reference to static class member, so we are getting all connections
   
   //std::cout << " conMap has: " << m_ringsMap.begin()->second.getConnectionsMap().size() << " rings" << std::endl;
   pL1RPCConeBuilder->setConeConnectionMap(m_ringsMap.begin()->second.getConnectionsMap());
      
   m_ringsMap.clear(); // free mem
      
   return pL1RPCConeBuilder;
   
}

// ----------------------------------------------------------
void RPCConeBuilder::geometryCallback( const MuonGeometryRecord& record ){

  //std::cout << " Geometry callback called " << std::endl; 
  
  //edm::ESHandle<RPCGeometry> rpcGeom;
  //record.get(rpcGeom);
  
  //buildCones(rpcGeom);
  
}


void RPCConeBuilder::buildCones(const edm::ESHandle<RPCGeometry> & rpcGeom ){
  

  //std::cout << "    ---> buildCones called " << std::endl; 
  
  // fetch geometricall data
  int rolls = 0;
  for(TrackingGeometry::DetContainer::const_iterator it = rpcGeom->dets().begin();
      it != rpcGeom->dets().end();
      ++it)
  {
  
      if( dynamic_cast< RPCRoll* >( *it ) == 0 ) continue;
      
      ++rolls;
      RPCRoll* roll = dynamic_cast< RPCRoll*>( *it );
      
      int ringId = RPCStripsRing::getRingId(roll);
      if ( m_ringsMap.find(ringId)  == m_ringsMap.end() ) {
         m_ringsMap[ringId]=RPCStripsRing(roll);
      } else {
         m_ringsMap[ringId].addRoll(roll);
      }
      //RPCStripsRing sr(roll);
      
  }

  //std::cout << " found: " << rolls << " dets" << std::endl;
  
  RPCStripsRing::TIdToRindMap::iterator it = m_ringsMap.begin();
  
  // filtermixed strips, fill gaps with virtual strips
  for (;it != m_ringsMap.end(); ++it){
     
    //int before = it->second.size();
    
    it->second.filterOverlapingChambers();    
    it->second.fillWithVirtualStrips();

    
    //std::cout << it->first << " " <<  it->second.isReferenceRing() << " " 
     //   << before << " -> " 
      //  << it->second.size() << std::endl;
    
    // In reference plane we should have 144*8 = 1152 strips
    if (it->second.isReferenceRing() && (it->second.size() != 1152)){
      throw cms::Exception("RPCInternal") << "Problem: refring " << it->first 
          << " has " << it->second.size() << " strips \n";
    }
    
    
  }
    
  // Xcheck, if rings are symettrical 
  it = m_ringsMap.begin();
  for (;it != m_ringsMap.end(); ++it){
    int key = it->first;
    int sign = key/100 - (key/1000)*10;
    
    if (sign == 0) {
      key += 100;
    } else {
      key -= 100;
    }
    
    if  (key != 2000){// Hey 2100 has no counterring
      if (it->second.size() !=  m_ringsMap[key].size())  
      {
         throw cms::Exception("RPCInternal") << " Size differs for ring " << key << " +- 100 \n";
      }
    }
  
  
  }
      
  buildConnections();
}


void RPCConeBuilder::buildConnections(){



   RPCStripsRing::TIdToRindMap::iterator itRef = m_ringsMap.begin();
   for (;itRef != m_ringsMap.end(); ++itRef){ // iterate over reference rings
      
      
      RPCStripsRing::TOtherConnStructVec ringsToConnect; 
      
      if (!itRef->second.isReferenceRing()) continue; // iterate over reference rings
      
      RPCStripsRing::TIdToRindMap::iterator itOther = m_ringsMap.begin();
      for (;itOther != m_ringsMap.end(); ++itOther){ // iterate over nonreference rings
         
         if (itOther->second.isReferenceRing()) continue; // iterate over nonreference rings
         
         std::pair<int,int> pr = areConnected(itRef, itOther);
         if ( pr.first != -1 ) {
            RPCStripsRing::TOtherConnStruct newOtherConn;
            newOtherConn.m_it = itOther;
            newOtherConn.m_logplane = pr.first;
            newOtherConn.m_logplaneSize = pr.second;
            ringsToConnect.push_back(newOtherConn);
         }
         
         
      } // OtherRings iteration ends
      
      // 
      std::pair<int,int> prRef = areConnected(itRef, itRef);
      if (prRef.first == -1){
        throw cms::Exception("RPCConfig") << " Cannot determine logplane for reference ring "
            << itRef->first << "\n ";
      }
      if (prRef.second != 8){
        // XXX        
        throw cms::Exception("RPCConfig") << " logplaneSize for reference ring "
            << itRef->first << " wrong "
            << " logplane: " << prRef.first
            << " etaPart: " << itRef->second.getEtaPartition()
            << " tower: " << itRef->second.getTowerForRefRing()
            << " hwPlane: " << itRef->second.getHwPlane()
            << " strips " << prRef.second << "\n";
      }
      
      itRef->second.createRefConnections(ringsToConnect, prRef.first, prRef.second);
      
   } // RefRings iteration ends

   
   // Fetch connection data, and save in one place
   /*
   RPCStripsRing::TIdToRindMap::iterator it = m_ringsMap.begin();
   for (;it != m_ringsMap.end(); ++it) { 
     
     L1RPCConeBuilder::TConMap nmap = it->second.getConnectionsMap();
     L1RPCConeBuilder::TConMap::iterator newMapIt = nmap.begin();
     for (; newMapIt != nmap.end(); ++ newMapIt) {
       uint32_t raw = newMapIt->first;
       TStrip2ConVec stripsVec = newMapIt->second;
       TStrip2ConVec::iterator stripIt = stripsVec.first
       //unsigned char strip = 
     
     
     }
     
     
  }*/
   
}

// first - logplane
// second - logplanesize
std::pair<int, int> RPCConeBuilder::areConnected(RPCStripsRing::TIdToRindMap::iterator ref,
                                  RPCStripsRing::TIdToRindMap::iterator other){

  int logplane = -1;
  
  //std::cout << "Checking " << ref->first << " and " << other->first << std::endl;
  
  // Do not connect  rolls lying on the oposite side of detector
  if ( ref->second.getEtaPartition()*other->second.getEtaPartition()<0  )
    return std::make_pair(-1,0);  
  
  
  // refRing and otherRing areConnected, if they contribute to the same tower
  L1RPCConeBuilder::TTowerList refTowList 
        = m_RingsToTowers.at(std::abs(ref->second.getEtaPartition()))
                         .at(ref->second.getHwPlane()-1);
      

  L1RPCConeBuilder::TTowerList otherTowList 
        = m_RingsToTowers.at(std::abs(other->second.getEtaPartition()))
                         .at(other->second.getHwPlane()-1);
  
  int refTower = -1;
  
  L1RPCConeBuilder::TTowerList::iterator rtlIt = refTowList.begin();
  for (; rtlIt != refTowList.end(); ++rtlIt){
  
     if ( *rtlIt >= 0 && refTower < 0){
        refTower = *rtlIt;
     }
     else if ( *rtlIt >= 0 && refTower >= 0) {
      throw cms::Exception("RPCConfig") << " Reference(?) ring "
            << ref->first << " "
            << "wants to be connected more than one tower: "
            << refTower << " "
            << *rtlIt << "\n";
     
     }
  
  }
  
  if (refTower < 0) {
     throw cms::Exception("RPCConfig") << " Reference(?) ring "
           << ref->first
           << " is not connected anywhere \n";
  }
  
  L1RPCConeBuilder::TTowerList::iterator otlIt = otherTowList.begin();
  
  int index = -1, i = 0;
  for (; otlIt != otherTowList.end(); ++otlIt){
     if (*otlIt == refTower) {
        index = i;
     }
     ++i;
  }
  
  int lpSize = 0;
  if (index != -1){
     logplane = m_RingsToLP.at(std::abs(other->second.getEtaPartition()))
           .at(other->second.getHwPlane()-1)
           .at(index);
     lpSize = m_LPSizesInTowers.at(refTower).at(logplane-1);
     
  }
  
  
  /*
  if (logplane != -1){
    
     std::cout << ref->first << " <-> " << other->first 
           << " logplane " << logplane
           << " lpsize " << lpSize 
           << std::endl;
                                  }*/
  
  return std::make_pair(logplane,lpSize);

}


//define this as a plug-in
DEFINE_FWK_EVENTSETUP_MODULE(RPCConeBuilder);
