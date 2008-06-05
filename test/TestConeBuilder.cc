// #include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/Framework/interface/Event.h"
// #include "CondCore/DBOutputService/interface/PoolDBOutputService.h"
// #include "Geometry/Records/interface/MuonGeometryRecord.h"
// #include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
// #include "DataFormats/DetId/interface/DetId.h"

#include "CondFormats/DataRecord/interface/L1RPCConeBuilderRcd.h"
#include "CondFormats/RPCObjects/interface/L1RPCConeBuilder.h"

#include "DataFormats/RPCDigi/interface/RPCDigiCollection.h"
#include "L1Trigger/RPCTrigger/interface/RPCLogCone.h" 
#include "L1Trigger/RPCTrigger/interface/RPCTriggerGeo.h"


// based on Alignment/ CommonAlignmentProducer/ test/ GlobalPositionRcdRead.cpp

class  TestConeBuilder : public edm::EDAnalyzer {
    public:
       explicit  TestConeBuilder(const edm::ParameterSet& iConfig ) {};
       ~TestConeBuilder() {}
       virtual void beginJob(const edm::EventSetup& iSetup);
       virtual void analyze(const edm::Event& evt, const edm::EventSetup& evtSetup);
       L1RpcLogConesVec getConesFromES(edm::Handle<RPCDigiCollection> rpcDigis, 
                                       edm::ESHandle<L1RPCConeBuilder> coneBuilder);
       
       bool compareCones(L1RpcLogConesVec::iterator it1, L1RpcLogConesVec::iterator it2);
       RPCTriggerGeo m_theLinksystem;  
       
 };


void TestConeBuilder::beginJob(const edm::EventSetup& iSetup) {




}


void TestConeBuilder::analyze(const edm::Event& evt, const edm::EventSetup& evtSetup) {
  //aa
    static int ev = 1;

    std::cout << "New event " << ev++ << std::endl;
    edm::ESHandle<L1RPCConeBuilder> coneBuilder;
    evtSetup.get<L1RPCConeBuilderRcd>().get(coneBuilder);
    
    if (!m_theLinksystem.isGeometryBuilt()){
    
        edm::LogInfo("RPC") << "Building RPC links map for a RPCTrigger";
        edm::ESHandle<RPCGeometry> rpcGeom;
        evtSetup.get<MuonGeometryRecord>().get( rpcGeom );     
        m_theLinksystem.buildGeometry(rpcGeom);
        edm::LogInfo("RPC") << "RPC links map for a RPCTrigger built";
      
    } 
    
    edm::Handle<RPCDigiCollection> rpcDigis;
    //  iEvent.getByType(rpcDigis);
    evt.getByLabel("muonRPCDigis",rpcDigis);
    // iEvent.getByLabel(m_label, rpcDigis);
    
    L1RpcLogConesVec conesByTrigger = m_theLinksystem.getCones(rpcDigis,0);
    L1RpcLogConesVec conesByES = getConesFromES(rpcDigis,coneBuilder);
    
    //conesByES.begin()->setLogStrip(1,5);
    
    /*std::cout << " Cones: " 
        << conesByTrigger.size()  << " "
        << conesByES.size() << std::endl;*/
    
    // now compare
    L1RpcLogConesVec::iterator itES = conesByES.begin();
    for(;itES!=conesByES.end(); ++itES){
      RPCConst::l1RpcConeCrdnts coneCrdsES = itES->getConeCrdnts();
      L1RpcLogConesVec::iterator itTRGMatching;
      int matchingCRDS = 0;
    
      
      L1RpcLogConesVec::iterator itTRG = conesByTrigger.begin();
      for (; itTRG != conesByTrigger.end(); ++itTRG){
        RPCConst::l1RpcConeCrdnts coneCrdsTRG = itTRG->getConeCrdnts();
        if (coneCrdsTRG == coneCrdsES){
          ++matchingCRDS;
          itTRGMatching=itTRG;
        }
      
      } // trigger cones
      
      // compare cones
      if ( matchingCRDS == 0){
        std::cout << " No match !!!" << std::endl;
      } else if ( matchingCRDS > 1){
        std::cout << " More than 1 match !!!" << std::endl;
      } else {  // exact 1 match
      
        if (!compareCones(itES,itTRGMatching)){
          std::cout << std::endl;
          std::cout << "##############################################################"<< std::endl;
          std::cout << "1 match, cones differ: " << std::endl;
          std::cout << "##############################################################"<< std::endl;
          std::cout << itTRGMatching->toString();
          std::cout << itES->toString();
          std::cout << "##############################################################"<< std::endl;
          std::cout << std::endl;
          
        } else {
          //std::cout << "Cone OK" << std::endl;
        }
      }
    
    
    }
    
}


L1RpcLogConesVec TestConeBuilder::getConesFromES(edm::Handle<RPCDigiCollection> rpcDigis, 
    edm::ESHandle<L1RPCConeBuilder> coneBuilder)
{
  std::vector<RPCLogHit> logHits;
  //const L1RPCConeBuilder::TConMap conMap = coneBuilder->getConeConnectionMap();
  
  
// Build cones from digis
  RPCDigiCollection::DigiRangeIterator detUnitIt;
  for (detUnitIt=rpcDigis->begin();
       detUnitIt!=rpcDigis->end();
       ++detUnitIt)
  {
    const RPCDetId& id = (*detUnitIt).first;

    int rawId = id.rawId();

    const RPCDigiCollection::Range& range = (*detUnitIt).second;

    for (RPCDigiCollection::const_iterator digiIt = range.first;
         digiIt!=range.second;
         ++digiIt)
    {
      if (digiIt->bx()!=0)
        continue;
      
      // what will happen for entry not in map? conMap is const..
      //L1RPCConeBuilder::TStrip2ConVec conVec = conMap[rawId];
      //L1RPCConeBuilder::TStripConVec conVec = coneBuilder->getConVec(rawId,digiIt->strip());
          //conMap[rawId][digiIt->strip()];
      
      std::pair<L1RPCConeBuilder::TStripConVec::const_iterator, L1RPCConeBuilder::TStripConVec::const_iterator> 
          itPair = coneBuilder->getConVec(rawId,digiIt->strip());
      
      L1RPCConeBuilder::TStripConVec::const_iterator it = itPair.first;
      /*
      if (it == conVec.end()){
        std::cout << rawId << " " <<digiIt->strip() << " not connected" << std::endl;
      } else {
        std::cout << rawId << " " <<digiIt->strip() << " " << conVec.size() << std::endl;
    }*/
      
      for (; it!=itPair.second;++it){
//         signed char m_tower;
//         unsigned char m_PAC;
//         unsigned char m_logplane;
//         unsigned char m_logstrip;
        logHits.push_back( RPCLogHit(it->m_tower, it->m_PAC, it->m_logplane, it->m_logstrip) );
        //std::cout << "+" << std::endl;
      }
      
    }
    
    
  }
  //std::cout << " lh.size() = " << logHits.size() << std::endl;
  
  L1RpcLogConesVec ActiveCones;

  std::vector<RPCLogHit>::iterator p_lhit;
  for (p_lhit = logHits.begin(); p_lhit != logHits.end(); p_lhit++){
    bool hitTaken = false;
    L1RpcLogConesVec::iterator p_cone;
    for (p_cone = ActiveCones.begin(); p_cone != ActiveCones.end(); p_cone++){
      hitTaken = p_cone->addLogHit(*p_lhit);
      if(hitTaken)
        break;
    }

    if(!hitTaken) {
      RPCLogCone newcone(*p_lhit);
      newcone.setIdx(ActiveCones.size());
      ActiveCones.push_back(newcone);
    }
  }// for loghits

  //std::cout << " ac.size() = " <<ActiveCones.size() << std::endl;
  return ActiveCones;
  
}


bool TestConeBuilder::compareCones(L1RpcLogConesVec::iterator it1, L1RpcLogConesVec::iterator it2){
  
  if (it1->getConeCrdnts()==it2->getConeCrdnts()){ // ok
  } else {
    throw cms::Exception("RPCInternal") << "TestConeBuilder::compareCones - crds do dont match \n";
  }
  
  bool conesMatch = true;
  for (int logplane = RPCConst::m_FIRST_PLANE;
           logplane != RPCConst::m_USED_PLANES_COUNT[std::abs(it1->getTower())];
           ++logplane  )
  {
    RPCLogCone::TLogPlane lp1 = it1->getLogPlane(logplane);
    RPCLogCone::TLogPlane lp2 = it2->getLogPlane(logplane);
    // iterate over lp1 strips. Find matching strip in lp2
    RPCLogCone::TLogPlane::iterator itLP1 = lp1.begin();
    for(;itLP1!=lp1.end();++itLP1){
      int strip = itLP1->first;
      RPCLogCone::TLogPlane::iterator itLP2 = lp2.find(strip);
      if(itLP2 == lp2.end()){
        conesMatch = false;
        break;
      }
    
    
    }
    
         
    if (!conesMatch)
      break;
  }
  
  return conesMatch;
}

DEFINE_FWK_MODULE(TestConeBuilder);

