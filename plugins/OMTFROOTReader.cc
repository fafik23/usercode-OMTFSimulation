#include <iostream>
#include <iomanip>

#include "UserCode/OMTFSimulation/plugins/OMTFROOTReader.h"

#include "UserCode/OMTFSimulation/interface/OMTFProcessor.h"
#include "UserCode/OMTFSimulation/interface/OMTFinputMaker.h"
#include "UserCode/OMTFSimulation/interface/OMTFinput.h"
#include "UserCode/OMTFSimulation/interface/OMTFSorter.h"
#include "UserCode/OMTFSimulation/interface/OMTFConfiguration.h"
#include "UserCode/OMTFSimulation/interface/OMTFConfigMaker.h"
#include "UserCode/OMTFSimulation/interface/XMLConfigWriter.h"

#include "UserCode/L1RpcTriggerAnalysis/interface/AnaEff.h"
#include "UserCode/L1RpcTriggerAnalysis/interface/AnaSiMuDistribution.h"

#include "UserCode/L1RpcTriggerAnalysis/interface/L1Obj.h"
#include "UserCode/L1RpcTriggerAnalysis/interface/L1ObjColl.h"
#include "UserCode/L1RpcTriggerAnalysis/interface/EventObj.h"
#include "UserCode/L1RpcTriggerAnalysis/interface/TrackObj.h"
#include "UserCode/L1RpcTriggerAnalysis/interface/L1ObjColl.h"
#include "UserCode/L1RpcTriggerAnalysis/interface/HitSpecObj.h"


#include "DataFormats/MuonDetId/interface/MuonSubdetId.h"
#include "DataFormats/MuonDetId/interface/RPCDetId.h"
#include "DataFormats/MuonDetId/interface/CSCDetId.h"
#include "DataFormats/MuonDetId/interface/DTChamberId.h"
#include "UserCode/L1RpcTriggerAnalysis/interface/DTphDigiSpec.h"
#include "UserCode/L1RpcTriggerAnalysis/interface/CSCDigiSpec.h"
#include "UserCode/L1RpcTriggerAnalysis/interface/RPCDigiSpec.h"


#include "TChain.h"
#include "TVector2.h"

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
OMTFROOTReader::OMTFROOTReader(const edm::ParameterSet & cfg){

  if (cfg.exists("anaEff")) myAnaEff = new   AnaEff(cfg.getParameter<edm::ParameterSet>("anaEff") );
  if (cfg.exists("anaSiMuDistribution")) myAnaSiMu = new AnaSiMuDistribution( cfg.getParameter<edm::ParameterSet>("anaSiMuDistribution"));

  if (cfg.exists("omtf")){
    myOMTFConfig = new OMTFConfiguration(cfg.getParameter<edm::ParameterSet>("omtf"));
    myOMTFConfigMaker = new OMTFConfigMaker(cfg.getParameter<edm::ParameterSet>("omtf"));
    myOMTF = new OMTFProcessor(cfg.getParameter<edm::ParameterSet>("omtf"));
  }
  myInputMaker = new OMTFinputMaker();
  mySorter = new OMTFSorter();

  myWriter = new XMLConfigWriter();
  std::string fName = "OMTF_Events";
  myWriter->initialiseXMLDocument(fName);

  theConfig = cfg;
}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
OMTFROOTReader::~OMTFROOTReader(){

  delete myOMTF;
  delete myOMTFConfig;
  delete myInputMaker;
  delete mySorter;

  if(myAnaEff) delete myAnaEff;
  if(myAnaSiMu) delete myAnaSiMu;

}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
void OMTFROOTReader::beginJob(){

  myHistos.SetOwner();
  myAnaEff->init(myHistos);
  myAnaSiMu->init(myHistos);

}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
void OMTFROOTReader::analyze(const edm::Event&, const edm::EventSetup& es){

  myInputMaker->initialize(es);

  // define input chain
  TChain chain("tL1Rpc");
  std::vector<std::string> treeFileNames = theConfig.getParameter<std::vector<std::string> >("treeFileNames");
  for (auto it:treeFileNames)  chain.Add(it.c_str());

  // prepare datastructures and branches
  std::vector<std::pair<uint32_t, uint32_t> > *digSpec = 0;
  TBranch *digSpecBranch = 0;

  EventObj * event = 0;
  TrackObj * simu = 0;
  L1ObjColl* l1ObjColl = 0;
  HitSpecObj* hitSpecProp = 0;
  HitSpecObj* hitSpec = 0;

  chain.SetBranchAddress("event",&event);
  chain.SetBranchAddress("simu",&simu);
  chain.SetBranchAddress("digSpec",&digSpec,&digSpecBranch);
  chain.SetBranchAddress("l1ObjColl",&l1ObjColl);
  chain.SetBranchAddress("hitSpecProp",&hitSpecProp);
  chain.SetBranchAddress("hitSpec",&hitSpec);

  // number of events
  Int_t nentries= (Int_t) chain.GetEntries();
  ///Test settings
  //nentries = 50;
  nentries = 5E4;
  /////////////////
  std::cout <<" ENTRIES: " << nentries << std::endl;

  // main loop
  unsigned int lastRun = 0;
  for (int ev=0; ev<nentries; ev+=1) {
  //for (int ev=33; ev<34; ev+=1) {

    chain.GetEntry(ev);
    L1ObjColl myL1ObjColl = *l1ObjColl;

    if ( (lastRun != (*event).run) || 
	 (nentries>1000 && ev%(nentries/10)==0) || 
	 nentries<1000) { 
      lastRun = (*event).run; 
      std::cout <<"RUN:"    << std::setw(7) << (*event).run
                <<" event:" << std::setw(8) << ev
                <<" done:"  << std::setw(6)<< std::setiosflags(std::ios::fixed) << std::setprecision(2) << ev*100./nentries<<"%";
      std::cout<<std::endl; 
    }

    if (myAnaSiMu && 
	!myAnaSiMu->filter(event, simu, hitSpec, hitSpecProp) && 
	theConfig.getParameter<bool>("filterByAnaSiMuDistribution") ) continue;

    for(unsigned int iProcessor=0;iProcessor<6;++iProcessor){
	const OMTFinput *myInput = myInputMaker->buildInputForProcessor(*digSpec,iProcessor);
	const OMTFinput myShiftedInput =  myOMTF->shiftInput(iProcessor,*myInput);	


	///Phi maps should be made with original, global phi values.
	//myOMTFConfigMaker->makeConnetionsMap(iProcessor,*myInput);
	myOMTFConfigMaker->makeConnetionsMap(iProcessor,myShiftedInput);

	continue;

	const OMTFProcessor::resultsMap & myResults = myOMTF->processInput(iProcessor,*myInput);
	L1Obj myOTFCandidate = mySorter->sortResults(myResults);
	//std::cout<<"iProcessor: "<<iProcessor<<std::endl;     
	if(ev<-100){
	  if(iProcessor==0){
	    for (auto it:*digSpec){
	      DetId detId(it.first);
	      switch (detId.subdetId()) {
	      case MuonSubdetId::RPC: { std::cout << std::endl <<RPCDetId(it.first)<<" "<<RPCDigiSpec(it.first, it.second);  break; }
	      case MuonSubdetId::DT:  { std::cout << std::endl <<DTChamberId(it.first)<<" "<<DTphDigiSpec(it.first, it.second); break; }
	      case MuonSubdetId::CSC: { std::cout << std::endl <<CSCDetId(it.first)<<" "<<CSCDigiSpec(it.first, it.second);  break; }
	      };
	      std::cout<<std::endl;
	    }
	  }
	  ///Print input and output      
	  std::cout<<"Original input"<<std::endl;
	  myInput->print(std::cout);
	  std::cout<<"Shifted input"<<std::endl;
	  myShiftedInput.print(std::cout);
	  std::cout<<"-------------------"<<std::endl;
	}
      /*
      for(auto & itGP: myResults){
	std::cout<<itGP.first<<std::endl;
	itGP.second.print(std::cout);
	std::cout<<std::endl;
	} */          
      //if(myOTFCandidate.pt) std::cout<<"iProcessor: "<<iProcessor<<" "<<myOTFCandidate<<std::endl;
      //////////////////////////////////
      if(myOTFCandidate.pt) myL1ObjColl.push_back(myOTFCandidate, false, 0.); 
    }
    ///Write to XML
    //xercesc::DOMElement *aTopElement = myWriter->writeEventHeader(ev);
    //myWriter->writeEventData(aTopElement,*myInput);
    //for(auto itKey: myResults) myWriter->writeResultsData(aTopElement, itKey.first,itKey.second);    
    if (myAnaEff) myAnaEff->run(simu, &myL1ObjColl, hitSpecProp);
  }

  myOMTFConfigMaker->printPhiMap(std::cout);
  myOMTFConfigMaker->printConnections(std::cout,0,0);


}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
void OMTFROOTReader::endJob(){

  //std::string fName = "TestEvents.xml";
  //myWriter->finaliseXMLDocument(fName);

  
  std::string fName = "Connections.xml";  
  myWriter->writeConnectionsData(OMTFConfiguration::measurements4D);
  myWriter->finaliseXMLDocument(fName);
}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

