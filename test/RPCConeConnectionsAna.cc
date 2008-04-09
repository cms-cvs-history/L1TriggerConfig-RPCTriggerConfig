// -*- C++ -*-
//
// Package:    RPCConeConnectionsAna
// Class:      RPCConeConnectionsAna
// 
/**\class RPCConeConnectionsAna RPCConeConnectionsAna.cc L1TriggerConfig/RPCConeConnectionsAna/src/RPCConeConnectionsAna.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Tomasz Maciej Frueboes
//         Created:  Tue Mar 18 15:15:30 CET 2008
// $Id$
//
//


// system include files
#include <memory>

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"

#include "FWCore/Framework/interface/ESHandle.h"


#include "CondFormats/DataRecord/interface/L1RPCConeBuilderRcd.h"
#include "CondFormats/RPCObjects/interface/L1RPCConeBuilder.h"

#include "Geometry/RPCGeometry/interface/RPCGeometry.h"
#include "Geometry/Records/interface/MuonGeometryRecord.h"

#include "CondFormats/RPCObjects/interface/RPCReadOutMapping.h"

#include "CondFormats/RPCObjects/interface/RPCEMap.h"
#include "CondFormats/DataRecord/interface/RPCEMapRcd.h"



//
// class decleration
//

class RPCConeConnectionsAna : public edm::EDAnalyzer {
   public:
      explicit RPCConeConnectionsAna(const edm::ParameterSet&);
      ~RPCConeConnectionsAna();


   private:
      virtual void beginJob(const edm::EventSetup&) ;
      virtual void analyze(const edm::Event&, const edm::EventSetup&);
      virtual void endJob() ;
      int getDCCNumber(int iTower, int iSec);
      int   m_towerBeg;
      int   m_towerEnd;
      int   m_sectorBeg;
      int   m_sectorEnd;

      // ----------member data ---------------------------
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
RPCConeConnectionsAna::RPCConeConnectionsAna(const edm::ParameterSet& iConfig)

{
   //now do what ever initialization is needed
   m_towerBeg = iConfig.getParameter<int>("minTower");
   m_towerEnd = iConfig.getParameter<int>("maxTower");

   m_sectorBeg = iConfig.getParameter<int>("minSector");
   m_sectorEnd = iConfig.getParameter<int>("maxSector");


}


RPCConeConnectionsAna::~RPCConeConnectionsAna()
{
 
   // do anything here that needs to be done at desctruction time
   // (e.g. close files, deallocate resources etc.)

}


//
// member functions
//

// ------------ method called to for each event  ------------
void
RPCConeConnectionsAna::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{}


// ------------ method called once each job just before starting event loop  ------------
void 
RPCConeConnectionsAna::beginJob(const edm::EventSetup& evtSetup)
{

    std::map<int,int> PACmap;


    edm::ESHandle<L1RPCConeBuilder> coneBuilder;
    evtSetup.get<L1RPCConeBuilderRcd>().get(coneBuilder);

    edm::ESHandle<RPCGeometry> rpcGeom;
    evtSetup.get<MuonGeometryRecord>().get(rpcGeom);


    edm::ESHandle<RPCEMap> nmap;
    evtSetup.get<RPCEMapRcd>().get(nmap);
    const RPCEMap* eMap=nmap.product();
    edm::ESHandle<RPCReadOutMapping>  map = eMap->convert();

    for(TrackingGeometry::DetContainer::const_iterator it = rpcGeom->dets().begin();
      it != rpcGeom->dets().end();
      ++it)
    {

      if( dynamic_cast< RPCRoll* >( *it ) == 0 ) continue;

      RPCRoll* roll = dynamic_cast< RPCRoll*>( *it );

      int detId = roll->id().rawId();
//      if ( detId != 637567014) continue;
 //     if (roll->id().station() != 2 || roll->id().ring() != 2) continue; 


      //iterate over strips
      for (int strip = 0; strip< roll->nstrips(); ++strip){

          std::pair<L1RPCConeBuilder::TStripConVec::const_iterator, 
                    L1RPCConeBuilder::TStripConVec::const_iterator> 
                    itPair = coneBuilder->getConVec(detId, strip);

          L1RPCConeBuilder::TStripConVec::const_iterator it = itPair.first;


          for (; it!=itPair.second;++it){
            //logHits.push_back( RPCLogHit(it->m_tower, it->m_PAC, it->m_logplane, it->m_logstrip) );
            // iterate over all PACs 
            for (int tower = m_towerBeg; tower <= m_towerEnd;++tower){

              if (it->m_tower != tower) continue;

              if (itPair.first == itPair.second) {
//                   std::cout << " Strip not connected : " << detId << " " << strip << " " << roll->id() << std::endl;
              }

              for (int sector = m_sectorBeg; sector <= m_sectorEnd; ++sector){

                int dccInputChannel = getDCCNumber(tower, sector);
                int PAC = sector*12;
                int PACend = PAC+11;

                for(; PAC <= PACend; ++PAC){

                   if (it->m_PAC != PAC ) continue;
                   ++PACmap[PAC];

                /*   std::cout << "Testing PAC " << PAC 
                             << " in tower " << tower 
                             << std::endl;*/
  
                   LinkBoardElectronicIndex a;
                   std::pair< LinkBoardElectronicIndex, LinkBoardPackedStrip> linkStrip =
                        std::make_pair(a, LinkBoardPackedStrip(0,0));

                   std::pair<int,int> stripInDetUnit(detId, strip);
                   std::vector< std::pair< LinkBoardElectronicIndex, LinkBoardPackedStrip> > aVec = map->rawDataFrame( stripInDetUnit);
                   std::vector< std::pair< LinkBoardElectronicIndex, LinkBoardPackedStrip> >::const_iterator CI;

                   for(CI=aVec.begin();CI!=aVec.end();CI++){

                     if(CI->first.dccInputChannelNum==dccInputChannel) linkStrip = *CI;
                     /*if ( detId == 637567014){
                        std::cout << "TMF  " << CI->first.dccInputChannelNum << std::endl;
                     }*/

                   }
            
                   if(linkStrip.second.packedStrip()==-17) {
                     std::cout<< "BAD: PAC "<< PAC  << " tower "  << tower 
                              << " detId " << detId << " strip " << strip 
                              << " lp " << (int)it->m_logplane
                              << " ls "  << (int)it->m_logstrip
                              <<" "<< RPCDetId(detId) 
                              << std::endl; 
                     std::cout << "      -> " << aVec.begin()->first.dccId << " " << aVec.begin()->first.dccInputChannelNum 
                               << " " << aVec.size()
                               << std::endl;

                     LocalPoint lStripCentre1 = roll->centreOfStrip(1);
                     LocalPoint lStripCentreMax = roll->centreOfStrip(roll->nstrips() );

                     GlobalPoint gStripCentre1 = roll->toGlobal(lStripCentre1);
                     GlobalPoint gStripCentreMax = roll->toGlobal(lStripCentreMax);
                     float phiRaw1 = gStripCentre1.phi();
                     float phiRawMax = gStripCentreMax.phi();
                     std::cout << phiRaw1 << " " << phiRawMax << std::endl; 
                   
                   } else {
                    /* std::cout<<" OK: PAC "<< PAC  << " tower "  << tower 
                              << " detId " << detId << " strip " << strip 
                              << " lp " << (int)it->m_logplane
                              << " ls "  << (int)it->m_logstrip
                              <<" "<< RPCDetId(detId) 
                              << std::endl; */
                   }

              } // PAC iteration

            } // sector iteration
           
          } // tower iteration

      } // cone connections interation

    } // strip in roll iteration

  } // roll iteration


  std::map<int,int>::iterator it =  PACmap.begin();
  for(;it!=PACmap.end();++it){

    if (it->second!=8){
   //    std::cout << "PAC " << it->first << " refcon " << it->second << std::endl;
    }

  }

}

// ------------ method called once each job just after ending the event loop  ------------
void 
RPCConeConnectionsAna::endJob() {
}

int RPCConeConnectionsAna::getDCCNumber(int iTower, int iSec){

  int tbNumber = 0;
  if(iTower<-12) tbNumber = 0;
  else if(-13<iTower && iTower<-8) tbNumber = 1;
  else if(-9<iTower && iTower<-4) tbNumber = 2;
  else if(-5<iTower && iTower<-1) tbNumber = 3;
  else if(-2<iTower && iTower<2) tbNumber = 4;
  else if(1<iTower && iTower<5) tbNumber = 5;
  else if(4<iTower && iTower<9) tbNumber = 6;
  else if(8<iTower && iTower<13) tbNumber = 7;
  else if(12<iTower) tbNumber = 8;

  int phiFactor = iSec%4;
  return (tbNumber + phiFactor*9); //Count DCC input channel from 1
}


//define this as a plug-in
DEFINE_FWK_MODULE(RPCConeConnectionsAna);
