#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/PropertyMgr.h"
#include "VertexFit/IVertexDbSvc.h"
#include "GaudiKernel/Bootstrap.h"
#include "GaudiKernel/ISvcLocator.h"

#include "EventModel/EventModel.h"
#include "EventModel/Event.h"

#include "EvtRecEvent/EvtRecEvent.h"
#include "EvtRecEvent/EvtRecTrack.h"
#include "DstEvent/TofHitStatus.h"
#include "EventModel/EventHeader.h"



#include "TMath.h"
#include "GaudiKernel/INTupleSvc.h"
#include "GaudiKernel/NTuple.h"
#include "GaudiKernel/Bootstrap.h"
#include "GaudiKernel/IHistogramSvc.h"
#include "CLHEP/Vector/ThreeVector.h"
#include "CLHEP/Vector/LorentzVector.h"
#include "CLHEP/Vector/TwoVector.h"
using CLHEP::Hep3Vector;
using CLHEP::Hep2Vector;
using CLHEP::HepLorentzVector;
#include "CLHEP/Geometry/Point3D.h"
#ifndef ENABLE_BACKWARDS_COMPATIBILITY
   typedef HepGeom::Point3D<double> HepPoint3D;
#endif
#include "eta2pipiAlg/eta2pipi.h"

#include "VertexFit/KinematicFit.h"
//#include "VertexFit/KalmanKinematicFit.h"
#include "VertexFit/VertexFit.h"
#include "VertexFit/Helix.h"
#include "ParticleID/ParticleID.h"
#include <vector>
//const double twopi = 6.2831853;
//const double pi = 3.1415927;
const double mpi = 0.13957;
const double xmass[5] = {0.000511, 0.105658, 0.139570,0.493677, 0.938272};
//const double velc = 29.9792458;  tof_path unit in cm.
const double velc = 299.792458;   // tof path unit in mm
typedef std::vector<int> Vint;
typedef std::vector<HepLorentzVector> Vp4;
int nCounter_PSL[] = {0,0,0,0,0,0,0,0,0};
bool debug = false;
#include "McTruth/McParticle.h"
//#include "MdcRecEvent/RecMdcTrack.h"
//#include "MdcRecEvent/RecMdcDedx.h"
//#include "TofRecEvent/RecTofTrack.h"
//#include "EmcRecEventModel/RecEmcShower.h"
//#include "MucRecEvent/RecMucTrack.h"
//#include "VertexFitRefine/VertexFitRefine.h"

/////////////////////////////////////////////////////////////////////////////

eta2pipi::eta2pipi(const std::string& name, ISvcLocator* pSvcLocator) :
  Algorithm(name, pSvcLocator) {
  
  declareProperty("Test4C", m_test4C = 1);
  declareProperty("Test5C", m_test5C = 1);
  
  declareProperty("CheckDedx", m_checkDedx = 1);
  declareProperty("CheckTof",  m_checkTof = 1);
  //Declare the properties  
}

//--------------------------------------------------------------------------
StatusCode eta2pipi::initialize(){
//--------------------------------------------------------------------------
  MsgStream log(msgSvc(), name());
  log << MSG::INFO << "in initialize()" << endmsg;
  
  StatusCode status;
  nevt = 0;
  nrun = 0;
// add m_cutflow
  status = service("THistSvc", m_histSvc);
  if(status.isFailure() ){
    log << MSG::INFO << "Unable to retrieve pointer to THistSvc" << endreq;
    return status;
  }
  m_cutflow = new TH1F("cutflow", "cutflow", 9, -0.5, 8.5);//8 is bin from -0.5 to 6.5.
  status = m_histSvc->regHist("/HIST/cutflow", m_cutflow);

//mc information
   NTuplePtr nt0(ntupleSvc(), "FILE1/mc");
     if ( nt0 ) m_mcTuple = nt0;
     else
     {
       m_mcTuple = ntupleSvc()->book ("FILE1/mc", CLID_ColumnWiseTuple, "Data Analysis");
       if ( m_mcTuple )
       {
       // McTruth Information
      status = m_mcTuple->addItem ("jpsi_mom",   n_jpsi_mom); 
      status = m_mcTuple->addItem ("jpsi_cos",   n_jpsi_cos);
      status = m_mcTuple->addItem ("gam1_mom",   n_gam1_mom); 
      status = m_mcTuple->addItem ("gam1_cos",   n_gam1_cos);
      status = m_mcTuple->addItem ("eta_mom",   n_eta_mom); 
      status = m_mcTuple->addItem ("eta_cos",   n_eta_cos);
      status = m_mcTuple->addItem ("gam2_mom",   n_gam2_mom); 
      status = m_mcTuple->addItem ("gam2_cos",   n_gam2_cos);
      status = m_mcTuple->addItem ("pip_mom",   n_pip_mom); 
      status = m_mcTuple->addItem ("pip_cos",   n_pip_cos);
      status = m_mcTuple->addItem ("pim_mom",   n_pim_mom); 
      status = m_mcTuple->addItem ("pim_cos",   n_pim_cos);
       }
       else
       {
         log << MSG::ERROR << "    Cannot book N-tuple:" << long(m_mcTuple) << endmsg;
         return StatusCode::FAILURE;
       }
     }
//analysis information    
  NTuplePtr nt1(ntupleSvc(), "FILE1/REC");
    if ( nt1 ) m_anaTuple = nt1;
    else
    {
     m_anaTuple = ntupleSvc()->book ("FILE1/REC", CLID_ColumnWiseTuple, "Data Analysis");
     if ( m_anaTuple )
     { //McTruth Information
       status = m_anaTuple->addItem ("indexmc",         m_idxmc, 0, 100);
       status = m_anaTuple->addIndexedItem("trkidx",    m_idxmc, m_trkidx);
       status = m_anaTuple->addIndexedItem("pdgid",     m_idxmc, m_pdgid);
       status = m_anaTuple->addIndexedItem("motherpdgid",m_idxmc, m_motherpdgid);
       status = m_anaTuple->addIndexedItem("motheridx", m_idxmc, m_motheridx);
       //jpsi Info.
       status = m_anaTuple->addItem ("mc_jpsi_mom",   m_jpsi_mom);
       status = m_anaTuple->addItem ("mc_jpsi_cos",   m_jpsi_cos);
       status = m_anaTuple->addItem ("mc_jpsi_pdgid",   m_jpsi_pdgid);
       status = m_anaTuple->addItem ("mc_e_jpsi_mom",   e_jpsi_mom);
       status = m_anaTuple->addItem ("mc_px_jpsi_mom",   px_jpsi_mom);
       status = m_anaTuple->addItem ("mc_py_jpsi_mom",   py_jpsi_mom);
       status = m_anaTuple->addItem ("mc_pz_jpsi_mom",   pz_jpsi_mom);
       //gam1 Info.(from jpsi)
       status = m_anaTuple->addItem ("mc_gam1_mom",   m_gam1_mom);
       status = m_anaTuple->addItem ("mc_gam1_cos",   m_gam1_cos);
       status = m_anaTuple->addItem ("mc_gam1_pdgid",   m_gam1_pdgid);
       status = m_anaTuple->addItem ("mc_e_gam1_mom",   e_gam1_mom);
       status = m_anaTuple->addItem ("mc_px_gam1_mom",   px_gam1_mom);
       status = m_anaTuple->addItem ("mc_py_gam1_mom",   py_gam1_mom);
       status = m_anaTuple->addItem ("mc_pz_gam1_mom",   pz_gam1_mom);
      //eta Info.
       status = m_anaTuple->addItem ("mc_eta_mom",   m_eta_mom);
       status = m_anaTuple->addItem ("mc_eta_cos",   m_eta_cos);
       status = m_anaTuple->addItem ("mc_eta_pdgid",   m_eta_pdgid);
       status = m_anaTuple->addItem ("mc_e_eta_mom",   e_eta_mom);
       status = m_anaTuple->addItem ("mc_px_eta_mom",   px_eta_mom);
       status = m_anaTuple->addItem ("mc_py_eta_mom",   py_eta_mom);
       status = m_anaTuple->addItem ("mc_pz_eta_mom",   pz_eta_mom);
     //gam2(from eta) Info.
       status = m_anaTuple->addItem ("mc_gam2_mom",   m_gam2_mom);      
       status = m_anaTuple->addItem ("mc_gam2_cos",   m_gam2_cos);
       status = m_anaTuple->addItem ("mc_gam2_pdgid",   m_gam2_pdgid);
       status = m_anaTuple->addItem ("mc_e_gam2_mom",   e_gam2_mom);
       status = m_anaTuple->addItem ("mc_px_gam2_mom",   px_gam2_mom);
       status = m_anaTuple->addItem ("mc_py_gam2_mom",   py_gam2_mom);
       status = m_anaTuple->addItem ("mc_pz_gam2_mom",   pz_gam2_mom);
     //pip Info.
       status = m_anaTuple->addItem ("mc_pip_mom",   m_pip_mom);      
       status = m_anaTuple->addItem ("mc_pip_cos",   m_pip_cos);
       status = m_anaTuple->addItem ("mc_pip_pdgid",   m_pip_pdgid);
       status = m_anaTuple->addItem ("mc_e_pip_mom",   e_pip_mom);
       status = m_anaTuple->addItem ("mc_px_pip_mom",   px_pip_mom);
       status = m_anaTuple->addItem ("mc_py_pip_mom",   py_pip_mom);
       status = m_anaTuple->addItem ("mc_pz_pip_mom",   pz_pip_mom);
     //pim Info.
       status = m_anaTuple->addItem ("mc_pim_mom",   m_pim_mom);      
       status = m_anaTuple->addItem ("mc_pim_cos",   m_pim_cos);
       status = m_anaTuple->addItem ("mc_pim_pdgid",   m_pim_pdgid);
       status = m_anaTuple->addItem ("mc_e_pim_mom",   e_pim_mom);
       status = m_anaTuple->addItem ("mc_px_pim_mom",   px_pim_mom);
       status = m_anaTuple->addItem ("mc_py_pim_mom",   py_pim_mom);
       status = m_anaTuple->addItem ("mc_pz_pim_mom",   pz_pim_mom);
    //kmfit Info.
    //lab_pip
       status = m_anaTuple->addItem ("kmfit_lab_pip_e",    m_kmfit_lab_pip_e);
       status = m_anaTuple->addItem ("kmfit_lab_pip_px",   m_kmfit_lab_pip_px);
       status = m_anaTuple->addItem ("kmfit_lab_pip_py",   m_kmfit_lab_pip_py);
       status = m_anaTuple->addItem ("kmfit_lab_pip_pz",   m_kmfit_lab_pip_pz);
    //lab_pim
       status = m_anaTuple->addItem ("kmfit_lab_pim_e",    m_kmfit_lab_pim_e);
       status = m_anaTuple->addItem ("kmfit_lab_pim_px",   m_kmfit_lab_pim_px);
       status = m_anaTuple->addItem ("kmfit_lab_pim_py",   m_kmfit_lab_pim_py);
       status = m_anaTuple->addItem ("kmfit_lab_pim_pz",   m_kmfit_lab_pim_pz);
    //lab_Etagam
       status = m_anaTuple->addItem ("kmfit_lab_Etagam_e",    m_kmfit_lab_Etagam_e);
       status = m_anaTuple->addItem ("kmfit_lab_Etagam_px",   m_kmfit_lab_Etagam_px);
       status = m_anaTuple->addItem ("kmfit_lab_Etagam_py",   m_kmfit_lab_Etagam_py);
       status = m_anaTuple->addItem ("kmfit_lab_Etagam_pz",   m_kmfit_lab_Etagam_pz);
       
    //Four momentum boosted to eta 
     //cms_pim
       status = m_anaTuple->addItem ("kmfit_cms_pim_e",    m_kmfit_cms_pim_e);  
       status = m_anaTuple->addItem ("kmfit_cms_pim_px",   m_kmfit_cms_pim_px);
       status = m_anaTuple->addItem ("kmfit_cms_pim_py",   m_kmfit_cms_pim_py);
       status = m_anaTuple->addItem ("kmfit_cms_pim_pz",   m_kmfit_cms_pim_pz);
    
    //cms_pip
       status = m_anaTuple->addItem ("kmfit_cms_pip_e",    m_kmfit_cms_pip_e);  
       status = m_anaTuple->addItem ("kmfit_cms_pip_px",   m_kmfit_cms_pip_px);
       status = m_anaTuple->addItem ("kmfit_cms_pip_py",   m_kmfit_cms_pip_py);
       status = m_anaTuple->addItem ("kmfit_cms_pip_pz",   m_kmfit_cms_pip_pz);

    //cms_Etagam
      status = m_anaTuple->addItem ("kmfit_cms_Etagam_e",    m_kmfit_cms_Etagam_e);     
      status = m_anaTuple->addItem ("kmfit_cms_Etagam_px",   m_kmfit_cms_Etagam_px);
      status = m_anaTuple->addItem ("kmfit_cms_Etagam_py",   m_kmfit_cms_Etagam_py);
      status = m_anaTuple->addItem ("kmfit_cms_Etagam_pz",   m_kmfit_cms_Etagam_pz);

    //Inv.Masses[kmfit]
      status = m_anaTuple->addItem ("mg1pippim",   m_mg1pippim);
      status = m_anaTuple->addItem ("mg2pippim",   m_mg2pippim);
      status = m_anaTuple->addItem ("mpippim",       m_mpippim);
    //4C Info.
	  status = m_anaTuple->addItem ("chi2_ggpippim",   m_chi2_ggpippim);
	  status = m_anaTuple->addItem ("chi2_gggpippim",   m_chi2_gggpippim);
    //5C Info.
	  status = m_anaTuple->addItem ("chi2",   m_chi2);
	  status = m_anaTuple->addItem ("meta",   m_meta); 
     
     //dE/dx
     status = m_anaTuple->addItem ("nGood",   m_nGood, 0, 200);
     status = m_anaTuple->addIndexedItem ("ptrk",   m_nGood,   m_ptrk);      
     status = m_anaTuple->addIndexedItem ("chie",   m_nGood,  m_chie);
     status = m_anaTuple->addIndexedItem ("chimu",  m_nGood,  m_chimu);
     status = m_anaTuple->addIndexedItem ("chipi",  m_nGood, m_chipi);
     status = m_anaTuple->addIndexedItem ("chik",   m_nGood, m_chik);
     status = m_anaTuple->addIndexedItem ("chip",   m_nGood,  m_chip);
     status = m_anaTuple->addIndexedItem ("probPH", m_nGood,   m_probPH);
     status = m_anaTuple->addIndexedItem ("normPH", m_nGood,  m_normPH);
	 status = m_anaTuple->addIndexedItem ("ghit",   m_nGood,  m_ghit);
	 status = m_anaTuple->addIndexedItem ("thit",   m_nGood,  m_thit);
   //
     status = m_anaTuple->addItem ("ptrk_etof",  m_nGood, m_ptot_etof);
     status = m_anaTuple->addItem ("cntr_etof",  m_nGood, m_cntr_etof);      
     status = m_anaTuple->addItem ("ph_etof",    m_nGood, m_ph_etof);     
     status = m_anaTuple->addItem ("rhit_etof",  m_nGood, m_rhit_etof);
     status = m_anaTuple->addItem ("qual_etof",  m_nGood, m_qual_etof);
     status = m_anaTuple->addItem ("te_etof",    m_nGood, m_te_etof);        
     status = m_anaTuple->addItem ("tmu_etof",   m_nGood, m_tmu_etof);
     status = m_anaTuple->addItem ("tpi_etof",   m_nGood, m_tpi_etof);
     status = m_anaTuple->addItem ("tk_etof",    m_nGood, m_tk_etof);  
     status = m_anaTuple->addItem ("tp_etof",    m_nGood, m_tp_etof);
   //
     status = m_anaTuple->addItem ("ptrk_btof1",   m_ptot_btof1);
     status = m_anaTuple->addItem ("cntr_btof1",   m_cntr_btof1);
     status = m_anaTuple->addItem ("ph_btof1",  m_ph_btof1);         
     status = m_anaTuple->addItem ("zhit_btof1", m_zhit_btof1);
     status = m_anaTuple->addItem ("qual_btof1", m_qual_btof1);
     status = m_anaTuple->addItem ("te_btof1",   m_te_btof1);
     status = m_anaTuple->addItem ("tmu_btof1",   m_tmu_btof1);
     status = m_anaTuple->addItem ("tpi_btof1",   m_tpi_btof1);
     status = m_anaTuple->addItem ("tk_btof1",   m_tk_btof1);
     status = m_anaTuple->addItem ("tp_btof1",   m_tp_btof1);
   //
     status = m_anaTuple->addItem ("ptrk_btof2",   m_ptot_btof2);
     status = m_anaTuple->addItem ("cntr_btof2",   m_cntr_btof2);
     status = m_anaTuple->addItem ("ph_btof2",  m_ph_btof2);
     status = m_anaTuple->addItem ("zhit_btof2", m_zhit_btof2);
     status = m_anaTuple->addItem ("qual_btof2", m_qual_btof2);
     status = m_anaTuple->addItem ("te_btof2",   m_te_btof2);
     status = m_anaTuple->addItem ("tmu_btof2",   m_tmu_btof2);
     status = m_anaTuple->addItem ("tpi_btof2",   m_tpi_btof2);
     status = m_anaTuple->addItem ("tk_btof2",   m_tk_btof2);
     status = m_anaTuple->addItem ("tp_btof2",   m_tp_btof2);
   //
     status = m_anaTuple->addItem ("ptrk_pid",  m_nGood, m_ptrk_pid);
     status = m_anaTuple->addItem ("cost_pid",  m_nGood, m_cost_pid);
     status = m_anaTuple->addItem ("dedx_pid",  m_nGood, m_dedx_pid);
     status = m_anaTuple->addItem ("tof1_pid",  m_nGood, m_tof1_pid);
     status = m_anaTuple->addItem ("tof2_pid",  m_nGood, m_tof2_pid);
     status = m_anaTuple->addItem ("prob_pid",  m_nGood, m_prob_pid);
   //photon
     status = m_anaTuple->addItem ("nGam",   m_nGam, 0, 200);
     status = m_anaTuple->addIndexedItem ("dthe", m_nGam,  m_dthe);
     status = m_anaTuple->addIndexedItem ("dphi", m_nGam,  m_dphi);
     status = m_anaTuple->addIndexedItem ("dang", m_nGam,  m_dang);
     status = m_anaTuple->addIndexedItem ("eraw", m_nGam,  m_eraw);
   //









     }
     else
     {
      log << MSG::ERROR << "Cannot book N-tuple:" << long(m_anaTuple) << endmsg;
      return StatusCode::FAILURE;
     }
    }

  }

//-----------------------------------------------------------------------
StatusCode eta2pipi::execute() {
//-----------------------------------------------------------------------
  MsgStream log(msgSvc(), name());
  log << MSG::INFO << "in execute()" << endreq;

  nCounter_PSL[0]++;    // Total Events
  m_cutflow->Fill(0);

//---------------------------------------------------------------------------
//Session #0:At the beginning
//---------------------------------------------------------------------------
  nevt +=1;
  SmartDataPtr<Event::EventHeader> eventHeader(eventSvc(),"/Event/EventHeader");
  if(eventHeader->eventNumber() == 1)
  {
    std::cout  << "************************************" << std::endl;
    std::cout  << "* New Run begin =========> " <<eventHeader->runNumber()<< " ****" << std::endl;
    std::cout  << "************************************" << std::endl;
  }

  int runNo=eventHeader->runNumber();
  int event=eventHeader->eventNumber();
   if(eventHeader->eventNumber() == 1) nrun+=1;
   if(fmod(double(nevt),1000.0) <= 0.5) { std::cout << nevt << "   Events executed!" << std::endl; }

  SmartDataPtr<EvtRecEvent> evtRecEvent(eventSvc(), EventModel::EvtRec::EvtRecEvent);
    log << MSG::DEBUG  
      <<"No. of charged tracks = " << evtRecEvent->totalCharged() << " , "
      <<"No. of neutral tracks = " << evtRecEvent->totalNeutral() << " , "
      <<"No. of total tracks   = " << evtRecEvent->totalTracks() <<endreq;
  SmartDataPtr<EvtRecTrackCol> evtRecTrkCol(eventSvc(),  EventModel::EvtRec::EvtRecTrackCol);
  if(evtRecEvent->totalCharged() < 2) { return StatusCode::SUCCESS; }
  nCounter_PSL[1]++;    // No. of Events with "nchrg>=2"
  m_cutflow->Fill(1);
//-------------------------------------------------
//Session #0:truth MC
//-------------------------------------------------
  if(debug) cout<<__LINE__<<endl;
  if(eventHeader->runNumber()<0)
  {//MC information
   //MC particle collection
    SmartDataPtr<Event::McParticleCol> mcParticleCol(eventSvc(),"/Event/MC/McParticleCol");
    if(debug) cout<<__LINE__<<endl;
    int m_numParticle = 0;
    if(debug) cout<<__LINE__<<endl;
    if(!mcParticleCol)
    {  
      std::cout << "Could not retrieve McParticelCol" << std::endl;
      return StatusCode::FAILURE;
    }
    else
    {
      bool jpsiDecay = false;
      int rootIndex = -1;
      Event::McParticleCol::iterator  iter_mc ;
      HepLorentzVector p4_jpsi,p4_gam1,p4_eta,p4_gam2,p4_pip,p4_pim;
      for(iter_mc = mcParticleCol->begin(); iter_mc != mcParticleCol->end(); iter_mc++)
      {
        if((*iter_mc)->primaryParticle()) continue;
        if(!(*iter_mc)->decayFromGenerator()) continue;
        if((*iter_mc)->particleProperty()==443)// 443=Jpsi; 100443=Psip; 30443=Psipp
        {
           jpsiDecay = true;
           rootIndex = (*iter_mc)->trackIndex();
        }//loop of 443
        if (!jpsiDecay) continue;
        int trkidx = (*iter_mc)->trackIndex() - rootIndex;  // 0ID= 4  // c
        int mcidx = ((*iter_mc)->mother()).trackIndex() - rootIndex; // 1ID= -4// c-bar
        int pdgid = (*iter_mc)->particleProperty();// 2ID= 91// cluste
        int motherpdgid = ((*iter_mc)->mother()).particleProperty();// 2ID= 91// cluste

        m_trkidx[m_numParticle] = trkidx;  
        m_pdgid[m_numParticle] = pdgid;     // 6ID= 310 or 2112           // Ks0
        m_motherpdgid[m_numParticle] = ((*iter_mc)->mother()).particleProperty();
        m_motheridx[m_numParticle] = mcidx;   // 7ID= -2112 or 310          // n-bar
        double costmc = cos((*iter_mc)->initialFourMomentum().theta()); // 9ID= 211                   // pion+
        double ptmc = (*iter_mc)->initialFourMomentum().rho();                    // 9ID= 211                   // pion+
        double pxmc = (*iter_mc)->initialFourMomentum().px();                     // 10ID= -211                 // pion-
        double pymc = (*iter_mc)->initialFourMomentum().py();                     // 11ID= 2212                 // pion+
        double pzmc = (*iter_mc)->initialFourMomentum().pz();                     // 12ID= -211                 // pion-
        double pemc = (*iter_mc)->initialFourMomentum().e();
        //if(debug) cout<<__LINE__<<endl;  
        //p4 and costheta
        if(fabs(pdgid) == 443)//jpsi=443
     {
         p4_jpsi.setPx(pxmc);
         p4_jpsi.setPy(pymc);
         p4_jpsi.setPz(pzmc);
         p4_jpsi.setE(pemc);

         n_jpsi_mom = ptmc;
         m_jpsi_mom = ptmc;
         n_jpsi_cos = costmc;
         m_jpsi_cos = costmc;
         m_jpsi_pdgid == pdgid;
         e_jpsi_mom = pemc;
         px_jpsi_mom = pxmc; 
         py_jpsi_mom = pymc;
         pz_jpsi_mom = pzmc;
     }
        if(pdgid == 22 && motherpdgid ==443)//gammaid = 22
     {
        p4_gam1.setPx(pxmc);
        p4_gam1.setPy(pymc);
        p4_gam1.setPz(pzmc);
        p4_gam1.setE(pemc);
        n_gam1_mom = ptmc;   
        m_gam1_mom = ptmc;
        n_gam1_cos = costmc;
        m_gam1_cos = costmc;
        m_gam1_pdgid == pdgid;
        e_gam1_mom = pemc;
        px_gam1_mom = pxmc; 
        py_gam1_mom = pymc;
        pz_gam1_mom = pzmc;
     }//loop of pdgid==22
        if(pdgid == 221 && motherpdgid ==443)//etaid=221
     {
        p4_eta.setPx(pxmc);
        p4_eta.setPy(pymc);
        p4_eta.setPz(pzmc);
        p4_eta.setE(pemc);
        n_eta_mom = ptmc;  
        m_eta_mom = ptmc;
        n_eta_cos = costmc;
        m_eta_cos = costmc;
        m_eta_pdgid == pdgid;
        e_eta_mom = pemc;
        px_eta_mom = pxmc; 
        py_eta_mom = pymc;
        pz_eta_mom = pzmc;
      }//loop of pdg=221
       if(pdgid == 22 && motherpdgid == 221)//gam2 from eta
     {
       p4_gam2.setPx(pxmc);       
       p4_gam2.setPy(pymc);
       p4_gam2.setPz(pzmc);
       p4_gam2.setE(pemc);
       n_gam2_mom = ptmc;  
       m_gam2_mom = ptmc;
       n_gam2_cos = costmc;
       m_gam2_cos = costmc;
       m_gam2_pdgid == pdgid;
       e_gam2_mom = pemc;
       px_gam2_mom = pxmc; 
       py_gam2_mom = pymc;
       pz_gam2_mom = pzmc;
     }//loop of pdg==22&&pdg==221    
      if(pdgid ==211 && motherpdgid ==221)//pion^+
    {
      p4_pip.setPx(pxmc);
      p4_pip.setPy(pymc);
      p4_pip.setPz(pzmc);
      p4_pip.setE(pemc);
      n_pip_mom = ptmc;  
      m_pip_mom = ptmc;
      n_pip_cos = costmc;
      m_pip_cos = costmc;
      m_pip_pdgid == pdgid;
      e_pip_mom = pemc;
      px_pip_mom = pxmc; 
      py_pip_mom = pymc;
      pz_pip_mom = pzmc;
   }//loop of pdgid ==211 && motherpdgid ==221
      if(pdgid == -211 && motherpdgid ==221)//pion^-
   {
      p4_pim.setPx(pxmc);
      p4_pim.setPy(pymc);
      p4_pim.setPz(pzmc);
      p4_pim.setE(pemc);
      n_pim_mom = ptmc;  
      m_pim_mom = ptmc;
      n_pim_cos = costmc;
      m_pim_cos = costmc;
      m_pim_pdgid == pdgid;
      e_pim_mom = pemc;
      px_pim_mom = pxmc; 
      py_pim_mom = pymc;
      pz_pim_mom = pzmc;
   }//loop of pdgid == -211 && motherpdgid ==221
   //if(debug) cout<<__LINE__<<endl;
   m_numParticle += 1;
   if(debug) cout<<__LINE__<<endl;
      }//loop of all particles  (for(ter_mc = mcParticleCol->begin()...))
  //cms (center mass system) 
  const Hep3Vector mc_eta_cms = -p4_eta.boostVector();
  p4_gam2.boost(mc_eta_cms);
  p4_pip.boost(mc_eta_cms);
  p4_pim.boost(mc_eta_cms);
  
  e_pip_mom = p4_pip.e();
  px_pip_mom = p4_pip.px();
  py_pip_mom = p4_pip.py();
  pz_pip_mom = p4_pip.pz();

  e_pim_mom = p4_pim.e();
  px_pim_mom = p4_pim.px();
  py_pim_mom = p4_pim.py();
  pz_pim_mom = p4_pim.pz();

  e_gam2_mom = p4_gam2.e();
  px_gam2_mom = p4_gam2.px();
  py_gam2_mom = p4_gam2.py();
  pz_gam2_mom = p4_gam2.pz();
   }//loop of #413 else
   m_idxmc = m_numParticle;
}//loop of (!mcParticleCol)
  
  m_mcTuple->write();

//--------------------------------------------------
//Session #1:Charged Tracks Selection
//--------------------------------------------------
  Vint iGood, ipip, ipim;
  iGood.clear();
  ipip.clear();
  ipim.clear();
  Vp4 ppip, ppim;
  ppip.clear();
  ppim.clear();

  Hep3Vector xorigin(0,0,0);
  IVertexDbSvc*  vtxsvc;
  Gaudi::svcLocator()->service("VertexDbSvc", vtxsvc);
  bool vertexValid = false;
    if (vtxsvc){
      if(vtxsvc->isVertexValid()){
        vertexValid = true;
        if(debug) cout<<__LINE__<<endl; 
        double* dbv = vtxsvc->PrimaryVertex(); 
        double*  vv = vtxsvc->SigmaPrimaryVertex();  
        xorigin.setX(dbv[0]);
        xorigin.setY(dbv[1]);
        xorigin.setZ(dbv[2]);
      } 
    }

  if(!vertexValid){
      return StatusCode::SUCCESS;
  }

   nCounter_PSL[2]++;
   m_cutflow->Fill(2);

   if(debug) cout<<__LINE__<<endl;
   int nCharge = 0;
  for(int i = 0; i < evtRecEvent->totalCharged(); i++)
  {
    EvtRecTrackIterator itTrk=evtRecTrkCol->begin() + i;
    if(!(*itTrk)->isMdcTrackValid()) continue;
    RecMdcTrack *mdcTrk = (*itTrk)->mdcTrack();
    double pt=mdcTrk->p();
    double x0=mdcTrk->x();
    double y0=mdcTrk->y();
    double z0=mdcTrk->z();
    double r0=sqrt(x0*x0+y0*y0); //add
    double phi0=mdcTrk->helix(1);
    double xv=xorigin.x();
    double yv=xorigin.y();
    double zv=xorigin.z();//add
    double Rxy=(x0-xv)*cos(phi0)+(y0-yv)*sin(phi0);

    HepVector a = mdcTrk->helix();
    HepSymMatrix Ea = mdcTrk->err();
    HepPoint3D point0(0.,0.,0.);   // the initial point for MDC recosntruction
    HepPoint3D IP(xorigin[0],xorigin[1],xorigin[2]); 
    VFHelix helixip(point0,a,Ea); 
    //Extrapolate to the IP
    helixip.pivot(IP);
    HepVector vecipa = helixip.a();
    double  Rvxy0=fabs(vecipa[0]);  //the nearest distance to IP in xy plane
    double  Rvz0=vecipa[3];         //the nearest distance to IP in z direction
    double  Rvphi0=vecipa[1];
    double costheta0 = cos(mdcTrk -> theta());
    if(fabs(costheta0)>=0.93) continue;
    if(fabs(Rvz0) >= 10.0) continue;
    if(fabs(Rvxy0) >= 1.0) continue;
    
    iGood.push_back(i);
   // iGood.push_back((*itTrk)->trackid());
    nCharge += mdcTrk->charge();
  }//loop of total tracks selection
  
  //
  // Finish Good Charged Track Selection
  //
//-------------------------------------------------------
//Session #2 Neutral Tracks Selection
//-------------------------------------------------------
  int nGood = iGood.size();
  log << MSG::DEBUG << "ngood, totcharge = " << nGood << " , " << nCharge << endreq;
  if((nGood != 2)||(nCharge!=0)){
    return StatusCode::SUCCESS;
  }
 nCounter_PSL[3]++;
 m_cutflow->Fill(3);
//-------------------------------------------------------
  Vint iGam;
  iGam.clear();
  int nGam = 0;
    for(int i = evtRecEvent->totalCharged(); i< evtRecEvent->totalTracks(); i++)
    {
      EvtRecTrackIterator itTrk=evtRecTrkCol->begin() + i;
        if(!(*itTrk)->isEmcShowerValid()) continue;
      RecEmcShower *emcTrk = (*itTrk)->emcShower();
      Hep3Vector emcpos(emcTrk->x(), emcTrk->y(), emcTrk->z());
    // find the nearest charged track
      double dthe = 200.;
      double dphi = 200.;
      double dang = 200.; 
        for(int j = 0; j < evtRecEvent->totalCharged(); j++) {
          EvtRecTrackIterator jtTrk = evtRecTrkCol->begin() + j;
            if(!(*jtTrk)->isExtTrackValid()) continue;
          RecExtTrack *extTrk = (*jtTrk)->extTrack();
            if(extTrk->emcVolumeNumber() == -1) continue;
          Hep3Vector extpos = extTrk->emcPosition();
      //      double ctht = extpos.cosTheta(emcpos);
           double angd = extpos.angle(emcpos);
           double thed = extpos.theta() - emcpos.theta();
           double phid = extpos.deltaPhi(emcpos);
           thed = fmod(thed+CLHEP::twopi+CLHEP::twopi+pi, CLHEP::twopi) - CLHEP::pi;
           phid = fmod(phid+CLHEP::twopi+CLHEP::twopi+pi, CLHEP::twopi) - CLHEP::pi;
             if(angd < dang){ dang = angd;dthe = thed; dphi = phid;}
        }
        if(dang>=200) continue;
      double eraw = emcTrk->energy();
      dthe = dthe * 180 / (CLHEP::pi);
      dphi = dphi * 180 / (CLHEP::pi);
      dang = dang * 180 / (CLHEP::pi);
      double time = emcTrk->time();
      double costheta = emcpos.cosTheta();
    //
    // good photon cut will be set here
    //
       if(time>=14.||time<=0.)continue;
       if(fabs(costheta) > 0.92)continue;
       if(!((fabs(costheta)<0.80 && eraw >0.025)||(fabs(costheta)>0.86||(fabs(costheta)<0.92) && eraw > 0.05))) continue;
       if(fabs(dang)<10.0) continue;
    
     m_dthe[nGam] = dthe;
     m_dphi[nGam] = dphi;
     m_dang[nGam] = dang;
     m_eraw[nGam] = eraw;
     iGam.push_back(i); 
     nGam++;
  } // end of loop of all photons

  m_nGam = nGam;
  
  // Finish Good Photon Selection

  log << MSG::DEBUG << "num Good Photon " << nGam  << " , " <<evtRecEvent->totalNeutral()<<endreq;
  if(nGam<2 || nGam > 200){
    return StatusCode::SUCCESS;
  }
  nCounter_PSL[4]++;
  m_cutflow->Fill(4);
  if(debug) cout<<__LINE__<<endl;

//----------------------------------------------------
//  check dedx infomation
//----------------------------------------------------
  if(m_checkDedx == 1) {
    for(int i = 0; i < nGood; i++) {
      EvtRecTrackIterator  itTrk = evtRecTrkCol->begin() + iGood[i];
      if(!(*itTrk)->isMdcTrackValid()) continue;
      if(!(*itTrk)->isMdcDedxValid())continue;
      RecMdcTrack* mdcTrk = (*itTrk)->mdcTrack();
      RecMdcDedx* dedxTrk = (*itTrk)->mdcDedx();
      m_nGood = nGood;
      m_ptrk[m_nGood] = mdcTrk->p();
      m_chie [m_nGood] = dedxTrk->chiE();
      m_chimu [m_nGood] = dedxTrk->chiMu();
      m_chipi [m_nGood] = dedxTrk->chiPi();
      m_chik [m_nGood] = dedxTrk->chiK();
      m_chip [m_nGood] = dedxTrk->chiP();
      m_ghit [m_nGood] = dedxTrk->numGoodHits();
      m_thit [m_nGood] = dedxTrk->numTotalHits();
      m_probPH [m_nGood] = dedxTrk->probPH();
      m_normPH [m_nGood] = dedxTrk->normPH();
    }
  }
  if(debug) cout<<__LINE__<<endl;

 //-------------------------------------------------
 // check TOF infomation
 //------------------------------------------------

  if(m_checkTof == 1) {
    for(int i = 0; i < nGood; i++) {
      EvtRecTrackIterator  itTrk = evtRecTrkCol->begin() + iGood[i];
      if(!(*itTrk)->isMdcTrackValid()) continue;
      if(!(*itTrk)->isTofTrackValid()) continue;

      RecMdcTrack * mdcTrk = (*itTrk)->mdcTrack();
      SmartRefVector<RecTofTrack> tofTrkCol = (*itTrk)->tofTrack();

      double ptrk = mdcTrk->p();

      SmartRefVector<RecTofTrack>::iterator iter_tof = tofTrkCol.begin();
      for(;iter_tof != tofTrkCol.end(); iter_tof++ ) { 
        TofHitStatus *status = new TofHitStatus; 
        status->setStatus((*iter_tof)->status());
        if(!(status->is_barrel())){//endcap
          if( !(status->is_counter()) ) continue; // ? 
          if( status->layer()!=0 ) continue;//layer1
          double path=(*iter_tof)->path(); // ? 
          double tof  = (*iter_tof)->tof();
          double ph   = (*iter_tof)->ph();
          double rhit = (*iter_tof)->zrhit();
          double qual = 0.0 + (*iter_tof)->quality();
          double cntr = 0.0 + (*iter_tof)->tofID();
          double texp[5];
          for(int j = 0; j < 5; j++) {
            double gb = ptrk/xmass[j];
            double beta = gb/sqrt(1+gb*gb);
            texp[j] = 10 * path /beta/velc;
          }
          m_cntr_etof[nGood] = cntr;
          m_ptot_etof[nGood]= ptrk;
          m_ph_etof  [nGood]  = ph;
          m_rhit_etof[nGood] = rhit;
          m_qual_etof[nGood]= qual;
          m_te_etof  [nGood]= tof - texp[0];
          m_tmu_etof [nGood]= tof - texp[1];
          m_tpi_etof [nGood]= tof - texp[2];
          m_tk_etof  [nGood]= tof - texp[3];
          m_tp_etof  [nGood]= tof - texp[4];
        }
        else {//barrel
          if( !(status->is_counter()) ) continue; // ? 
          if(status->layer()==1){ //layer1
            double path=(*iter_tof)->path(); // ? 
            double tof  = (*iter_tof)->tof();
            double ph   = (*iter_tof)->ph();
            double rhit = (*iter_tof)->zrhit();
            double qual = 0.0 + (*iter_tof)->quality();
            double cntr = 0.0 + (*iter_tof)->tofID();
            double texp[5];
            for(int j = 0; j < 5; j++) {
              double gb = ptrk/xmass[j];
              double beta = gb/sqrt(1+gb*gb);
              texp[j] = 10 * path /beta/velc;
            }
 
            m_cntr_btof1  = cntr;
            m_ptot_btof1 = ptrk;
            m_ph_btof1   = ph;
            m_zhit_btof1  = rhit;
            m_qual_btof1  = qual;
            m_te_btof1    = tof - texp[0];
            m_tmu_btof1   = tof - texp[1];
            m_tpi_btof1   = tof - texp[2];
            m_tk_btof1    = tof - texp[3];
            m_tp_btof1    = tof - texp[4];
          }

          if(status->layer()==2){//layer2
            double path=(*iter_tof)->path(); // ? 
            double tof  = (*iter_tof)->tof();
            double ph   = (*iter_tof)->ph();
            double rhit = (*iter_tof)->zrhit();
            double qual = 0.0 + (*iter_tof)->quality();
            double cntr = 0.0 + (*iter_tof)->tofID();
            double texp[5];
            for(int j = 0; j < 5; j++) {
              double gb = ptrk/xmass[j];
              double beta = gb/sqrt(1+gb*gb);
              texp[j] = 10 * path /beta/velc;
            }
 
            m_cntr_btof2  = cntr;
            m_ptot_btof2 = ptrk;
            m_ph_btof2   = ph;
            m_zhit_btof2  = rhit;
            m_qual_btof2  = qual;
            m_te_btof2    = tof - texp[0];
            m_tmu_btof2   = tof - texp[1];
            m_tpi_btof2   = tof - texp[2];
            m_tk_btof2    = tof - texp[3];
            m_tp_btof2    = tof - texp[4];
          } 
        }

        delete status; 
      } 
    } // loop all charged track
  }  // check tof
  if(debug) cout<<__LINE__<<endl;

 //---------------------------------------------------
 // Assign 4-momentum to each photon
 //-------------------------------------------------- 

  Vp4 pGam;
  pGam.clear();
  for(int i = 0; i < nGam; i++) {
    EvtRecTrackIterator itTrk = evtRecTrkCol->begin() + iGam[i]; 
    RecEmcShower* emcTrk = (*itTrk)->emcShower();
    double eraw = emcTrk->energy();
    double phi = emcTrk->phi();
    double the = emcTrk->theta();
    HepLorentzVector ptrk;
    ptrk.setPx(eraw*sin(the)*cos(phi));
    ptrk.setPy(eraw*sin(the)*sin(phi));
    ptrk.setPz(eraw*cos(the));
    ptrk.setE(eraw);

//    ptrk = ptrk.boost(-0.011,0,0);// boost to cms

    pGam.push_back(ptrk);
  }
  if(debug) cout<<__LINE__<<endl;
  
 //-------------------------------------------------
 // Session#3 Particle indentification for the charged tracks
 // 1) particle indentification for charged trks
 // 2) Assign 4-momentum to each charged track (679-702)
 //-------------------------------------------------
 
  ParticleID *pid = ParticleID::instance();

  if(debug) cout<<__LINE__<<endl;
  for(int i = 0; i < nGood; i++) 
  {
    if(debug) std::cout<<"Check pid info for good charged track " << i << std::endl; 
    EvtRecTrackIterator itTrk = evtRecTrkCol->begin() + iGood[i]; 
    //    if(pid) delete pid;
    pid->init();
    pid->setMethod(pid->methodProbability());
//    pid->setMethod(pid->methodLikelihood());  //for Likelihood Method  

    pid->setChiMinCut(4);
    pid->setRecTrack(*itTrk);
    pid->usePidSys(pid->useDedx() | pid->useTof1() | pid->useTof2() | pid->useTofE()); // use PID sub-system
    pid->identify(pid->onlyPion() | pid->onlyKaon());    // seperater Pion/Kaon
    //    pid->identify(pid->onlyPion());
    //  pid->identify(pid->onlyKaon());
    pid->calculate();
    if(!(pid->IsPidInfoValid())) continue;
    RecMdcTrack* mdcTrk = (*itTrk)->mdcTrack();
    m_ptrk_pid [m_nGood] = mdcTrk->p();
    m_cost_pid [m_nGood] = cos(mdcTrk->theta());
    m_dedx_pid [m_nGood] = pid->chiDedx(2);
    m_tof1_pid [m_nGood] = pid->chiTof1(2);
    m_tof2_pid [m_nGood] = pid->chiTof2(2);
    m_prob_pid [m_nGood] = pid->probPion();

//  if(pid->probPion() < 0.001 || (pid->probPion() < pid->probKaon())) continue;
    if(pid->probPion() < 0.001) continue;
//    if(pid->pdf(2)<pid->pdf(3)) continue; //  for Likelihood Method(0=electron 1=muon 2=pion 3=kaon 4=proton) 

    RecMdcKalTrack* mdcKalTrk = (*itTrk)->mdcKalTrack();//After ParticleID, use RecMdcKalTrack substitute RecMdcTrack
    RecMdcKalTrack::setPidType  (RecMdcKalTrack::pion);//PID can set to electron, muon, pion, kaon and proton;The default setting is pion

    if(mdcKalTrk->charge() >0) {
      ipip.push_back(iGood[i]);
      HepLorentzVector ptrk;
      ptrk.setPx(mdcKalTrk->px());
      ptrk.setPy(mdcKalTrk->py());
      ptrk.setPz(mdcKalTrk->pz());
      double p3 = ptrk.mag();
      ptrk.setE(sqrt(p3*p3+mpi*mpi));

//      ptrk = ptrk.boost(-0.011,0,0);//boost to cms

      ppip.push_back(ptrk);
    } else {
      ipim.push_back(iGood[i]);
      HepLorentzVector ptrk;
      ptrk.setPx(mdcKalTrk->px());
      ptrk.setPy(mdcKalTrk->py());
      ptrk.setPz(mdcKalTrk->pz());
      double p3 = ptrk.mag();
      ptrk.setE(sqrt(p3*p3+mpi*mpi));

//      ptrk = ptrk.boost(-0.011,0,0);//boost to cms

      ppim.push_back(ptrk);
    }
    if(debug) cout<<__LINE__<<endl;
  }
  int npip = ipip.size();
  int npim = ipim.size();
  if(npip*npim != 1) return SUCCESS;
  nCounter_PSL[5]++;    // After PID
  m_cutflow->Fill(5);
//-------------------------------------------------------
//Session #4: Kinematic Fit
//-------------------------------------------------------
  int iok =-1;
  RecMdcKalTrack *pipTrk = (*(evtRecTrkCol->begin()+ipip[0]))->mdcKalTrack();
  RecMdcKalTrack *pimTrk = (*(evtRecTrkCol->begin()+ipim[0]))->mdcKalTrack();

  WTrackParameter wvpipTrk, wvpimTrk;
  wvpipTrk = WTrackParameter(mpi, pipTrk->getZHelix(), pipTrk->getZError());
  wvpimTrk = WTrackParameter(mpi, pimTrk->getZHelix(), pimTrk->getZError());

  HepPoint3D vx(0.0, 0.0, 0.0);
  HepSymMatrix Evx(3, 0);
  double bx = 1E+6;
  double by = 1E+6;
  double bz = 1E+6;
  Evx[0][0] = bx*bx;
  Evx[1][1] = by*by;
  Evx[2][2] = bz*bz;
//
//Set a common vertex with huge error[eta--->pim pip ]
//
  VertexParameter vxpar;
  vxpar.setVx(vx);
  vxpar.setEvx(Evx);
  VertexFit* vtxfit = VertexFit::instance();
  vtxfit->init();
  vtxfit->AddTrack(0,  wvpipTrk);
  vtxfit->AddTrack(1,  wvpimTrk);
  vtxfit->AddVertex(0, vxpar,0, 1);
  if(!vtxfit->Fit(0)) return SUCCESS;
  vtxfit->Swim(0);
   
  nCounter_PSL[6]++;    // After vertex fit
  m_cutflow->Fill(6);
  WTrackParameter wpip = vtxfit->wtrk(0);
  WTrackParameter wpim = vtxfit->wtrk(1);

  KinematicFit * kmfit = KinematicFit::instance();
//KalmanKinematicFit * kmfit = KalmanKinematicFit::instance();
//
//   Kinematic 4C fit
// 
  if(m_test4C==1) {
//    double ecms = 3.097;
    HepLorentzVector ecms(0.034,0,0,3.097);
//1) select the ig1 and ig2 which give the best chisq
    double chisq_ggpippim = 9999.;
    double chisq_gggpippim = 9999.;
    int ig1 = -1;
    int ig2 = -1;
    double mg1pippim = -9999;
    double mg2pippim = -9999;
    double mpippim = -9999;
    for(int i = 0; i < nGam-1; i++) 
  {
      RecEmcShower *g1Trk = (*(evtRecTrkCol->begin()+iGam[i]))->emcShower();
      for(int j = i+1; j < nGam; j++) 
    {
	RecEmcShower *g2Trk = (*(evtRecTrkCol->begin()+iGam[j]))->emcShower();
	kmfit->init();
	kmfit->AddTrack(0, wpip);
	kmfit->AddTrack(1, wpim);
	kmfit->AddTrack(2, 0.0, g1Trk);
	kmfit->AddTrack(3, 0.0, g2Trk);
	kmfit->AddFourMomentum(0, ecms);
	bool oksq = kmfit->Fit();
	if(oksq) {
	  double chi2 = kmfit->chisq();
      iok =1;
	  if(chi2 < chisq_ggpippim) {
	    chisq_ggpippim = chi2;
	    ig1 = iGam[i];
	    ig2 = iGam[j];
	    HepLorentzVector g1pippim = kmfit->pfit(0) + kmfit->pfit(1) + kmfit->pfit(2);
	    HepLorentzVector g2pippim = kmfit->pfit(0) + kmfit->pfit(1) + kmfit->pfit(3);
	    HepLorentzVector pippim = kmfit->pfit(0) + kmfit->pfit(1);
        double ene_g1 = kmfit->pfit(2).e();
        double ene_g2 = kmfit->pfit(3).e();
        HepLorentzVector p4_pip = kmfit->pfit(0); 
        HepLorentzVector p4_pim= kmfit->pfit(1);
        m_kmfit_lab_pip_e = p4_pip.e();
        m_kmfit_lab_pip_px = p4_pip.px();
        m_kmfit_lab_pip_py = p4_pip.py();
        m_kmfit_lab_pip_pz = p4_pip.pz();
        m_kmfit_lab_pim_e = p4_pim.e();
        m_kmfit_lab_pim_px = p4_pim.px();
        m_kmfit_lab_pim_py = p4_pim.py();
        m_kmfit_lab_pim_pz = p4_pim.pz();

        HepLorentzVector p4_eta, p4_ISRgam, p4_Etagam;
        if(ene_g1>=ene_g2) 
      {
         p4_eta = kmfit->pfit(3) + kmfit->pfit(0) + kmfit->pfit(1);
         p4_ISRgam = kmfit->pfit(2);
         p4_Etagam = kmfit->pfit(3);
      } else
        {
         p4_eta = kmfit->pfit(2) + kmfit->pfit(0) + kmfit->pfit(1);
         p4_ISRgam = kmfit->pfit(3);
         p4_Etagam = kmfit->pfit(2);
        }
       m_kmfit_lab_Etagam_e = p4_Etagam.e();
       m_kmfit_lab_Etagam_px = p4_Etagam.px();
       m_kmfit_lab_Etagam_py = p4_Etagam.py();
       m_kmfit_lab_Etagam_pz = p4_Etagam.pz();

	    mg1pippim = g1pippim.m();
	    mg2pippim = g2pippim.m();
        mpippim = pippim.m();
        //boost the Etagam, pip, pim to eta center of mass frame
        const Hep3Vector eta_cms = -p4_eta.boostVector();
        p4_pip.boost(eta_cms);
        p4_pim.boost(eta_cms);
        p4_Etagam.boost(eta_cms);
    
        m_kmfit_cms_pip_e = p4_pip.e(); 
        m_kmfit_cms_pip_px = p4_pip.px(); 
        m_kmfit_cms_pip_py = p4_pip.py(); 
        m_kmfit_cms_pip_pz = p4_pip.pz(); 
        m_kmfit_cms_pim_e = p4_pim.e(); 
        m_kmfit_cms_pim_px = p4_pim.px(); 
        m_kmfit_cms_pim_py = p4_pim.py(); 
        m_kmfit_cms_pim_pz = p4_pim.pz(); 
        m_kmfit_cms_Etagam_e = p4_Etagam.e(); 
        m_kmfit_cms_Etagam_px = p4_Etagam.px();
        m_kmfit_cms_Etagam_py = p4_Etagam.py();
        m_kmfit_cms_Etagam_pz = p4_Etagam.pz();
	  }
	}
      }
    }

    // 2) perform the kinematic fit with the selected ig1, ig2, pi+, pi- 
    if(chisq_ggpippim < 200) { 
     //Inv.Masses[kmfit]
     m_mg1pippim = mg1pippim;
     m_mg2pippim = mg2pippim;
     m_mpippim = mpippim;
     m_chi2_ggpippim = chisq_ggpippim;

     // 3) if nGam >=3
     if(nGam>=3){ 
       for(int i = 0; i < nGam-2; i++) {
         RecEmcShower *g1Trk = (*(evtRecTrkCol->begin()+iGam[i]))->emcShower();
         for(int j = i+1; j < nGam-1; j++) {
           RecEmcShower *g2Trk = (*(evtRecTrkCol->begin()+iGam[j]))->emcShower();
           for(int k = j+1; k < nGam; k++) {
             RecEmcShower *g3Trk = (*(evtRecTrkCol->begin()+iGam[k]))->emcShower();
             kmfit->init();
             kmfit->AddTrack(0, wpip);
             kmfit->AddTrack(1, wpim);
             kmfit->AddTrack(2, 0.0, g1Trk);
             kmfit->AddTrack(3, 0.0, g2Trk);
             kmfit->AddTrack(4, 0.0, g3Trk);
             kmfit->AddFourMomentum(0, ecms);
             bool oksq = kmfit->Fit();
             if(oksq) {
               double chi2 = kmfit->chisq();
               if(chi2 < chisq_ggpippim) {
                 chisq_gggpippim = chi2;
               }
             } // if oksq
           } // end of loop for k   
         }  // end of loop for j 
       }  //end of loop for i 
     } 
     m_chi2_gggpippim = chisq_gggpippim; 
     
     nCounter_PSL[7]++; 
     m_cutflow->Fill(7);
    }// loop of chi_ggpippim <200
  } // if apply_4C
//
//  Kinematic 5C Fit
//
   //find the best combination over all possible pi+ pi- gamma gamma pair
  if(m_test5C==1) {
  //  double ecms = 3.097;
    HepLorentzVector ecms(0.034,0,0,3.097);
    double chisq = 9999.;
    int ig1 = -1;
    int ig2 = -1;
   for(int i =0; i <nGam; i++){ 
      RecEmcShower *g1Trk = (*(evtRecTrkCol->begin()+iGam[i]))->emcShower();
        for(int j=0;j<nGam; j++){
	  RecEmcShower *g2Trk = (*(evtRecTrkCol->begin()+iGam[j]))->emcShower();
          if(i == j)continue; 
	  kmfit->init();
	  kmfit->AddTrack(0, wpip);
	  kmfit->AddTrack(1, wpim);
	  kmfit->AddTrack(2, 0.0, g1Trk);
	  kmfit->AddTrack(3, 0.0, g2Trk);
	  kmfit->AddResonance(0, 0.547, 0, 1, 2);//eta ->gamma pip  pim
	  kmfit->AddFourMomentum(1, ecms);
	  if(!kmfit->Fit(0)) continue;
	  if(!kmfit->Fit(1)) continue;
	  bool oksq = kmfit->Fit();
	    if(oksq) {
	    double chi2 = kmfit->chisq();
	     if(chi2 < chisq) {
	    chisq = chi2;
	    ig1 = iGam[i];
	    ig2 = iGam[j];
	  }
	}
      }
    }
  

    log << MSG::INFO << " chisq = " << chisq <<endreq;

    if(chisq < 200) {
      RecEmcShower* g1Trk = (*(evtRecTrkCol->begin()+ig1))->emcShower();
      RecEmcShower* g2Trk = (*(evtRecTrkCol->begin()+ig2))->emcShower();

      kmfit->init();
      kmfit->AddTrack(0, wpip);
      kmfit->AddTrack(1, wpim);
      kmfit->AddTrack(2, 0.0, g1Trk);
      kmfit->AddTrack(3, 0.0, g2Trk);
      kmfit->AddResonance(0, 0.547, 0, 1, 2);
      kmfit->AddFourMomentum(1, ecms);
      bool oksq = kmfit->Fit();
      if(oksq){
	HepLorentzVector peta = kmfit->pfit(0)+kmfit->pfit(1) + kmfit->pfit(2);
	
	m_chi2  = kmfit->chisq();
	m_meta = peta.m();
    nCounter_PSL[8]++;    // After 5C
    m_cutflow->Fill(8);
      }  //oksq
    }//loop of chisq 
  }//loop of m_test5C

    m_anaTuple->write();

  return StatusCode::SUCCESS;
} 


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
StatusCode eta2pipi::finalize() {
  cout<<"#Total run    :         "<<nrun<<endl;
  cout<<"#Total events :         "<<nCounter_PSL[0]<<endl;
  cout<<"#nChrgtrks>=2 :         "<<nCounter_PSL[1]<<endl;
  cout<<"#Load vertex  :         "<<nCounter_PSL[2]<<endl;
  cout<<"#nGood = 2||nCharge=0:  "<<nCounter_PSL[3]<<endl;
  cout<<"#nGam         :         "<<nCounter_PSL[4]<<endl;
  cout<<"#After PID    :         "<<nCounter_PSL[5]<<endl;
  cout<<"#After vertex Fit:      "<<nCounter_PSL[6]<<endl;
  cout<<"#After 4C     :         "<<nCounter_PSL[7]<<endl;
  cout<<"#After 5C     :         "<<nCounter_PSL[8]<<endl;
  //cout<<"#Final events :         "<<nCounter_PSL[8]<<endl;
  MsgStream log(msgSvc(), name());
  log << MSG::INFO << "in finalize()" << endmsg;
  return StatusCode::SUCCESS;
}

