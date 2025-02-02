
/** produceNtuple
 *
 * Read nanoAOD Ntuple and produce "plain" Ntuple.
 *
 * \authors Christian Veelken, Tallinn
 *
 */

#include "DataFormats/FWLite/interface/InputSource.h"                                           // fwlite::InputSource
#include "DataFormats/FWLite/interface/OutputFiles.h"                                           // fwlite::OutputFiles
#include "FWCore/ParameterSet/interface/ParameterSet.h"                                         // edm::ParameterSet
#include "FWCore/ParameterSetReader/interface/ParameterSetReader.h"                             // edm::readPSetsFrom()
#include "FWCore/PluginManager/interface/PluginManager.h"                                       // edmplugin::PluginManager::configure()
#include "FWCore/PluginManager/interface/standard.h"                                            // edmplugin::standard::config()
#include "PhysicsTools/FWLite/interface/TFileService.h"                                         // fwlite::TFileService

#include "TallinnNtupleProducer/CommonTools/interface/cmsException.h"                           // cmsException
#include "TallinnNtupleProducer/CommonTools/interface/Era.h"                                    // Era, get_era()
#include "TallinnNtupleProducer/CommonTools/interface/hadTauDefinitions.h"                      // get_tau_id_wp_int()
#include "TallinnNtupleProducer/CommonTools/interface/merge_systematic_shifts.h"                // merge_systematic_shifts()
#include "TallinnNtupleProducer/CommonTools/interface/tH_auxFunctions.h"                        // get_tH_SM_str()
#include "TallinnNtupleProducer/CommonTools/interface/TTreeWrapper.h"                           // TTreeWrapper
#include "TallinnNtupleProducer/EvtWeightTools/interface/BtagSFRatioInterface.h"                // BtagSFRatioInterface
#include "TallinnNtupleProducer/EvtWeightTools/interface/ChargeMisIdRateInterface.h"            // ChargeMisIdRateInterface
#include "TallinnNtupleProducer/EvtWeightTools/interface/Data_to_MC_CorrectionInterface_2016.h" // Data_to_MC_CorrectionInterface_2016
#include "TallinnNtupleProducer/EvtWeightTools/interface/Data_to_MC_CorrectionInterface_2017.h" // Data_to_MC_CorrectionInterface_2017
#include "TallinnNtupleProducer/EvtWeightTools/interface/Data_to_MC_CorrectionInterface_2018.h" // Data_to_MC_CorrectionInterface_2018
#include "TallinnNtupleProducer/EvtWeightTools/interface/EvtWeightManager.h"                    // EvtWeightManager
#include "TallinnNtupleProducer/EvtWeightTools/interface/EvtWeightRecorder.h"                   // EvtWeightRecorder
#include "TallinnNtupleProducer/EvtWeightTools/interface/HadTauFakeRateInterface.h"             // HadTauFakeRateInterface
#include "TallinnNtupleProducer/EvtWeightTools/interface/HHWeightInterfaceCouplings.h"          // HHWeightInterfaceCouplings
#include "TallinnNtupleProducer/EvtWeightTools/interface/HHWeightInterfaceLO.h"                 // HHWeightInterfaceLO
#include "TallinnNtupleProducer/EvtWeightTools/interface/HHWeightInterfaceNLO.h"                // HHWeightInterfaceNLO
#include "TallinnNtupleProducer/EvtWeightTools/interface/LeptonFakeRateInterface.h"             // LeptonFakeRateInterface
#include "TallinnNtupleProducer/Objects/interface/Event.h"                                      // Event
#include "TallinnNtupleProducer/Objects/interface/EventInfo.h"                                  // EventInfo
#include "TallinnNtupleProducer/Objects/interface/GenHadTau.h"                                  // GenHadTau
#include "TallinnNtupleProducer/Objects/interface/GenJet.h"                                     // GenJet
#include "TallinnNtupleProducer/Objects/interface/GenLepton.h"                                  // GenLepton
#include "TallinnNtupleProducer/Objects/interface/GenPhoton.h"                                  // GenPhoton
#include "TallinnNtupleProducer/Objects/interface/TriggerInfo.h"                                // TriggerInfo
#include "TallinnNtupleProducer/Readers/interface/EventReader.h"                                // EventReader
#include "TallinnNtupleProducer/Readers/interface/GenHadTauReader.h"                            // GenHadTauReader
#include "TallinnNtupleProducer/Readers/interface/GenJetReader.h"                               // GenJetReader
#include "TallinnNtupleProducer/Readers/interface/GenLeptonReader.h"                            // GenLeptonReader
#include "TallinnNtupleProducer/Readers/interface/GenParticleReader.h"                          // GenParticleReader
#include "TallinnNtupleProducer/Readers/interface/GenPhotonReader.h"                            // GenPhotonReader
#include "TallinnNtupleProducer/Readers/interface/L1PreFiringWeightReader.h"                    // L1PreFiringWeightReader
#include "TallinnNtupleProducer/Readers/interface/LHEInfoReader.h"                              // LHEInfoReader
#include "TallinnNtupleProducer/Readers/interface/PSWeightReader.h"                             // PSWeightReader
#include "TallinnNtupleProducer/Selectors/interface/RunLumiEventSelector.h"                     // RunLumiEventSelector
#include "TallinnNtupleProducer/Writers/interface/WriterBase.h"                                 // WriterBase, WriterPluginFactory

#include <TBenchmark.h>                                                                         // TBenchmark
#include <TError.h>                                                                             // gErrorAbortLevel, kError
#include <TString.h>                                                                            // TString, Form()
#include <TTree.h>                                                                              // TTree
     
#include <boost/algorithm/string/replace.hpp>                                                   // boost::replace_all_copy()
#include <boost/algorithm/string/predicate.hpp>                                                 // boost::starts_with()

#include <assert.h>                                                                             // assert
#include <cstdlib>                                                                              // EXIT_SUCCESS, EXIT_FAILURE
#include <fstream>                                                                              // std::ofstream
#include <iostream>                                                                             // std::cerr, std::fixed
#include <iomanip>                                                                              // std::setprecision(), std::setw()
#include <string>                                                                               // std::string
#include <vector>                                                                               // std::vector

typedef std::vector<std::string> vstring;

/**
 * @brief Produce "plain" Ntuple, which is used for final event selection & histogram filling.
 */
int main(int argc, char* argv[])
{
//--- throw an exception in case ROOT encounters an error
  gErrorAbortLevel = kError;

//--- stop ROOT from keeping track of all histograms
  TH1::AddDirectory(false);

//--- parse command-line arguments
  if ( argc < 2 ) {
    std::cout << "Usage: " << argv[0] << " [parameters.py]" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "<produceNtuple>:" << std::endl;

//--- keep track of time it takes the macro to execute
  TBenchmark clock;
  clock.Start("produceNtuple");
std::cout << "break-point 1 reached" << std::endl;
//--- read python configuration parameters
  if ( !edm::readPSetsFrom(argv[1])->existsAs<edm::ParameterSet>("process") )
    throw cmsException("produceNtuple", __LINE__) << "No ParameterSet 'process' found in configuration file = " << argv[1] << " !!";
std::cout << "break-point 2 reached" << std::endl;
  edm::ParameterSet cfg = edm::readPSetsFrom(argv[1])->getParameter<edm::ParameterSet>("process");
std::cout << "break-point 3 reached" << std::endl;
  edm::ParameterSet cfg_produceNtuple = cfg.getParameter<edm::ParameterSet>("produceNtuple");
  AnalysisConfig analysisConfig("produceNtuple", cfg_produceNtuple);
  std::string process = cfg_produceNtuple.getParameter<std::string>("process");
std::cout << "break-point 4 reached" << std::endl;
  std::string treeName = cfg_produceNtuple.getParameter<std::string>("treeName");

  std::string era_string = cfg_produceNtuple.getParameter<std::string>("era");
  const Era era = get_era(era_string);

  bool isMC = cfg_produceNtuple.getParameter<bool>("isMC");
  edm::VParameterSet lumiScale = cfg_produceNtuple.getParameter<edm::VParameterSet>("lumiScale");
  bool apply_genWeight = cfg_produceNtuple.getParameter<bool>("apply_genWeight");
  std::string apply_topPtReweighting_str = cfg_produceNtuple.getParameter<std::string>("apply_topPtReweighting");
  bool apply_topPtReweighting = ! apply_topPtReweighting_str.empty();
  bool apply_l1PreFireWeight = cfg_produceNtuple.getParameter<bool>("apply_l1PreFireWeight");
  bool apply_btagSFRatio = cfg_produceNtuple.getParameter<bool>("apply_btagSFRatio");
std::cout << "break-point 5 reached" << std::endl;
  unsigned int numNominalLeptons = cfg_produceNtuple.getParameter<unsigned int>("numNominalLeptons");
  unsigned int numNominalHadTaus = cfg_produceNtuple.getParameter<unsigned int>("numNominalHadTaus");

  std::string hadTauWP_againstJets = cfg_produceNtuple.getParameter<std::string>("hadTauWP_againstJets_tight");
  std::string hadTauWP_againstElectrons = cfg_produceNtuple.getParameter<std::string>("hadTauWP_againstElectrons");
  std::string hadTauWP_againstMuons = cfg_produceNtuple.getParameter<std::string>("hadTauWP_againstMuons");
  std::string lep_mva_wp = cfg_produceNtuple.getParameter<std::string>("lep_mva_wp");
std::cout << "break-point 6 reached" << std::endl;
  bool apply_chargeMisIdRate = cfg_produceNtuple.getParameter<bool>("apply_chargeMisIdRate");

  std::string selection = cfg_produceNtuple.getParameter<std::string>("selection");

  bool isDEBUG = cfg_produceNtuple.getParameter<bool>("isDEBUG");
std::cout << "break-point 7 reached" << std::endl;
  std::vector<std::string> systematic_shifts;
  // CV: process all systematic uncertainties supported by EventReader class (only for MC)
  if ( isMC )
  {
    merge_systematic_shifts(systematic_shifts, EventReader::get_supported_systematics());
  }
  // CV: add central value (for data and MC)
  merge_systematic_shifts(systematic_shifts, { "central"});
std::cout << "break-point 8 reached" << std::endl;
  edm::ParameterSet cfg_dataToMCcorrectionInterface;
  cfg_dataToMCcorrectionInterface.addParameter<std::string>("era", era_string);
  cfg_dataToMCcorrectionInterface.addParameter<std::string>("hadTauSelection_againstJets", hadTauWP_againstJets);
std::cout << "break-point 8.1 reached" << std::endl;
std::cout << "hadTauWP_againstJets = " << hadTauWP_againstJets << std::endl;
  cfg_dataToMCcorrectionInterface.addParameter<int>("hadTauSelection_againstElectrons", get_tau_id_wp_int(hadTauWP_againstElectrons));
std::cout << "break-point 8.2 reached" << std::endl;
std::cout << "hadTauWP_againstElectrons = " << get_tau_id_wp_int(hadTauWP_againstElectrons) << std::endl;
  cfg_dataToMCcorrectionInterface.addParameter<int>("hadTauSelection_againstMuons", get_tau_id_wp_int(hadTauWP_againstMuons));
std::cout << "break-point 8.3 reached" << std::endl;
std::cout << "hadTauWP_againstMuons = " << get_tau_id_wp_int(hadTauWP_againstMuons) << std::endl;
  cfg_dataToMCcorrectionInterface.addParameter<std::string>("lep_mva_wp", lep_mva_wp);
std::cout << "lep_mva_wp = " << lep_mva_wp << std::endl;
  cfg_dataToMCcorrectionInterface.addParameter<bool>("isDEBUG", isDEBUG);
  Data_to_MC_CorrectionInterface_Base * dataToMCcorrectionInterface = nullptr;
  switch ( era )
  {
    case Era::k2016: dataToMCcorrectionInterface = new Data_to_MC_CorrectionInterface_2016(cfg_dataToMCcorrectionInterface); break;
    case Era::k2017: dataToMCcorrectionInterface = new Data_to_MC_CorrectionInterface_2017(cfg_dataToMCcorrectionInterface); break;
    case Era::k2018: dataToMCcorrectionInterface = new Data_to_MC_CorrectionInterface_2018(cfg_dataToMCcorrectionInterface); break;
    default: throw cmsException("produceNtuple", __LINE__) << "Invalid era = " << static_cast<int>(era);
  }
  const ChargeMisIdRateInterface chargeMisIdRateInterface(era);
std::cout << "break-point 9 reached" << std::endl;
  edm::ParameterSet cfg_leptonFakeRateWeight = cfg_produceNtuple.getParameter<edm::ParameterSet>("leptonFakeRateWeight");
  cfg_leptonFakeRateWeight.addParameter<std::string>("era", era_string);
  LeptonFakeRateInterface* jetToLeptonFakeRateInterface = new LeptonFakeRateInterface(cfg_leptonFakeRateWeight);
std::cout << "break-point 10 reached" << std::endl;
  edm::ParameterSet cfg_hadTauFakeRateWeight = cfg_produceNtuple.getParameter<edm::ParameterSet>("hadTauFakeRateWeight");
  cfg_hadTauFakeRateWeight.addParameter<std::string>("hadTauSelection", hadTauWP_againstJets);
  HadTauFakeRateInterface* jetToHadTauFakeRateInterface = new HadTauFakeRateInterface(cfg_hadTauFakeRateWeight);
std::cout << "break-point 11 reached" << std::endl;
  std::string selEventsFileName = cfg_produceNtuple.getParameter<std::string>("selEventsFileName");
  std::cout << "selEventsFileName = " << selEventsFileName << std::endl;
  RunLumiEventSelector* run_lumi_eventSelector = 0;
  if ( selEventsFileName != "" ) {
    edm::ParameterSet cfg_run_lumi_eventSelector;
    cfg_run_lumi_eventSelector.addParameter<std::string>("inputFileName", selEventsFileName);
    cfg_run_lumi_eventSelector.addParameter<std::string>("separator", ":");
    run_lumi_eventSelector = new RunLumiEventSelector(cfg_run_lumi_eventSelector);
  }
std::cout << "break-point 12 reached" << std::endl;
  fwlite::InputSource inputFiles(cfg);
  int maxEvents = inputFiles.maxEvents();
  std::cout << " maxEvents = " << maxEvents << std::endl;
  unsigned reportEvery = inputFiles.reportAfter();
std::cout << "break-point 13 reached" << std::endl;
  fwlite::OutputFiles outputFile(cfg);
  fwlite::TFileService fs = fwlite::TFileService(outputFile.file().data());
std::cout << "break-point 14 reached" << std::endl;
  TTreeWrapper* inputTree = new TTreeWrapper(treeName.data(), inputFiles.files(), maxEvents);
  std::cout << "Loaded " << inputTree->getFileCount() << " file(s).\n";
std::cout << "break-point 15 reached" << std::endl;
  EventReader* eventReader = new EventReader(cfg_produceNtuple);
  inputTree->registerReader(eventReader);
std::cout << "break-point 16 reached" << std::endl;
  TTree* outputTree = new TTree("events", "events");
std::cout << "break-point 17 reached" << std::endl;
  const edm::ParameterSet additionalEvtWeight = cfg_produceNtuple.getParameter<edm::ParameterSet>("evtWeight");
  const bool applyAdditionalEvtWeight = additionalEvtWeight.getParameter<bool>("apply");
  EvtWeightManager* eventWeightManager = nullptr;
  if ( applyAdditionalEvtWeight )
  {
    eventWeightManager = new EvtWeightManager(additionalEvtWeight);
    eventWeightManager->set_central_or_shift("central");
    inputTree->registerReader(eventWeightManager);
  }
std::cout << "break-point 18 reached" << std::endl;
  L1PreFiringWeightReader* l1PreFiringWeightReader = nullptr;
  if ( apply_l1PreFireWeight )
  {
    l1PreFiringWeightReader = new L1PreFiringWeightReader(cfg_produceNtuple);
    inputTree->registerReader(l1PreFiringWeightReader);
  }
std::cout << "break-point 19 reached" << std::endl;
  LHEInfoReader* lheInfoReader = nullptr;
  PSWeightReader* psWeightReader = nullptr;
  if ( isMC )
  {
    lheInfoReader = new LHEInfoReader(cfg_produceNtuple);
    inputTree->registerReader(lheInfoReader);
    psWeightReader = new PSWeightReader(cfg_produceNtuple);
    inputTree->registerReader(psWeightReader);
  }
std::cout << "break-point 20 reached" << std::endl;
  BtagSFRatioInterface* btagSFRatioInterface = nullptr;
  if ( apply_btagSFRatio )
  {
    const edm::ParameterSet btagSFRatio = cfg_produceNtuple.getParameterSet("btagSFRatio");
    btagSFRatioInterface = new BtagSFRatioInterface(btagSFRatio);
  }
std::cout << "break-point 21 reached" << std::endl;
  edmplugin::PluginManager::Config config;
  edmplugin::PluginManager::configure(edmplugin::standard::config());
  edm::VParameterSet cfg_writers = cfg_produceNtuple.getParameterSetVector("writerPlugins");
  std::vector<WriterBase*> writers;
  for ( auto cfg_writer : cfg_writers )
  {
std::cout << "break-point 21.1 reached" << std::endl;
    std::string pluginType = cfg_writer.getParameter<std::string>("pluginType");
std::cout << "pluginType = " << pluginType << std::endl;
    cfg_writer.addParameter<unsigned int>("numNominalLeptons", numNominalLeptons);
    cfg_writer.addParameter<unsigned int>("numNominalHadTaus", numNominalHadTaus);
    cfg_writer.addParameter<std::string>("process", process);
    cfg_writer.addParameter<bool>("isMC", isMC);
std::cout << "break-point 21.2 reached" << std::endl;
    WriterBase* writer = WriterPluginFactory::get()->create(pluginType, cfg_writer).get();
std::cout << "break-point 21.3 reached" << std::endl;
    writer->registerReaders(inputTree);
std::cout << "break-point 21.4 reached" << std::endl;
    writer->setBranches(outputTree);
std::cout << "break-point 21.5 reached" << std::endl;
    writers.push_back(writer);
std::cout << "break-point 21.6 reached" << std::endl;
  }
std::cout << "break-point 22 reached" << std::endl;
  int analyzedEntries = 0;
  TH1* histogram_analyzedEntries = fs.make<TH1D>("analyzedEntries", "analyzedEntries", 1, -0.5, +0.5);
  for ( auto central_or_shift : systematic_shifts )
  {
std::cout << "break-point 23 reached" << std::endl;
    inputTree->reset();
std::cout << "break-point 24 reached" << std::endl;
    while ( inputTree->hasNextEvent() && (!run_lumi_eventSelector || (run_lumi_eventSelector && !run_lumi_eventSelector->areWeDone())) )
    {
std::cout << "break-point 25 reached" << std::endl;
      eventReader->set_central_or_shift(central_or_shift);
      Event event = eventReader->read();
      if ( central_or_shift == "central" )
      {
        if ( inputTree->canReport(reportEvery) )
        {
          std::cout << "processing Entry " << inputTree->getCurrentMaxEventIdx()
                    << " or " << inputTree->getCurrentEventIdx() << " entry in #"
                    << (inputTree->getProcessedFileCount() - 1)
                    << " (" << event.eventInfo()
                    << ") file\n";
        }
        ++analyzedEntries;
        histogram_analyzedEntries->Fill(0.);
      }
std::cout << "break-point 26 reached" << std::endl;
      if ( run_lumi_eventSelector )
      { 
        if ( !(*run_lumi_eventSelector)(event.eventInfo()) )
        {
          continue;
        }
        if ( central_or_shift == "central" )
        {
          std::cout << "processing Entry " << inputTree->getCurrentMaxEventIdx() << ": " << event.eventInfo() << '\n';
          if ( inputTree->isOpen() )
          {
            std::cout << "input File = " << inputTree->getCurrentFileName() << '\n';
          }
        }
      }
std::cout << "break-point 27 reached" << std::endl;
      if ( central_or_shift == "central"  && isDEBUG )
      {
        std::cout << "event #" << inputTree->getCurrentMaxEventIdx() << ' ' << event.eventInfo() << '\n';
      }

      EvtWeightRecorder evtWeightRecorder({ central_or_shift }, central_or_shift, isMC);
      if ( isMC )
      {
        if ( apply_genWeight         ) evtWeightRecorder.record_genWeight(event.eventInfo());
        if ( eventWeightManager      )
        { 
          eventWeightManager->set_central_or_shift(central_or_shift);
          evtWeightRecorder.record_auxWeight(eventWeightManager);
        }
        if ( l1PreFiringWeightReader ) evtWeightRecorder.record_l1PrefireWeight(l1PreFiringWeightReader);
        if ( apply_topPtReweighting  ) evtWeightRecorder.record_toppt_rwgt(event.eventInfo().topPtRwgtSF);
        lheInfoReader->read();
        psWeightReader->read();
        evtWeightRecorder.record_lheScaleWeight(lheInfoReader);
        evtWeightRecorder.record_psWeight(psWeightReader);
        evtWeightRecorder.record_puWeight(&event.eventInfo());
        evtWeightRecorder.record_nom_tH_weight(&event.eventInfo());
        evtWeightRecorder.record_lumiScale(lumiScale);
std::cout << "break-point 28 reached" << std::endl;
//--- compute event-level weight for data/MC correction of b-tagging efficiency and mistag rate
//   (using the method "Event reweighting using scale factors calculated with a tag and probe method",
//    described on the BTV POG twiki https://twiki.cern.ch/twiki/bin/view/CMS/BTagShapeCalibration )
        evtWeightRecorder.record_btagWeight(event.selJetsAK4());
        if ( btagSFRatioInterface )
        {
          evtWeightRecorder.record_btagSFRatio(btagSFRatioInterface, event.selJetsAK4().size());
        }
std::cout << "break-point 29 reached" << std::endl;
        if ( analysisConfig.isMC_EWK() )
        {
          evtWeightRecorder.record_ewk_jet(event.selJetsAK4());
          evtWeightRecorder.record_ewk_bjet(event.selJetsAK4_btagMedium());
        }
std::cout << "break-point 30 reached" << std::endl;
        dataToMCcorrectionInterface->setLeptons(event.fakeableLeptons(), true);

//--- apply data/MC corrections for trigger efficiency
        evtWeightRecorder.record_leptonTriggerEff(dataToMCcorrectionInterface);

//--- apply data/MC corrections for efficiencies for lepton to pass loose identification and isolation criteria
        evtWeightRecorder.record_leptonIDSF_recoToLoose(dataToMCcorrectionInterface);

//--- apply data/MC corrections for efficiencies of leptons passing the loose identification and isolation criteria
//    to also pass the fakeable and/or tight identification and isolation criteria
        evtWeightRecorder.record_leptonIDSF_looseToTight(dataToMCcorrectionInterface, false);

//--- apply data/MC corrections for hadronic tau identification efficiency
//    and for e->tau and mu->tau misidentification rates
        dataToMCcorrectionInterface->setHadTaus(event.fakeableHadTaus());
        evtWeightRecorder.record_hadTauID_and_Iso(dataToMCcorrectionInterface);
        evtWeightRecorder.record_eToTauFakeRate(dataToMCcorrectionInterface);
        evtWeightRecorder.record_muToTauFakeRate(dataToMCcorrectionInterface);

        evtWeightRecorder.record_jetToLeptonFakeRate(jetToLeptonFakeRateInterface, event.fakeableLeptons());
        evtWeightRecorder.record_jetToTauFakeRate(jetToHadTauFakeRateInterface, event.fakeableHadTaus());
        evtWeightRecorder.compute_FR();
std::cout << "break-point 31 reached" << std::endl;
        if ( apply_chargeMisIdRate )
        {
          if ( numNominalLeptons == 2 && numNominalHadTaus == 0 )
          {
            double prob_chargeMisId_sum = 1.;
            if ( event.fakeableLeptons().size() == 2 )
            {
              const RecoLepton* fakeableLepton_lead = event.fakeableLeptons().at(0);
              const RecoLepton* fakeableLepton_sublead = event.fakeableLeptons().at(1);
              if ( fakeableLepton_lead->charge()*fakeableLepton_sublead->charge() > 0 )
              {
                prob_chargeMisId_sum = chargeMisIdRateInterface.get(fakeableLepton_lead, fakeableLepton_sublead);
              }
              // Karl: reject the event, if the applied probability of charge misidentification is 0;
              //       note that this can happen only if both selected leptons are muons (their misId prob is 0).
              if ( prob_chargeMisId_sum == 0. )
              {
                if ( run_lumi_eventSelector )
                {
                  std::cout << "event " << event.eventInfo().str() << " FAILS charge flip selection\n"
                            << "(leading lepton: charge = " << fakeableLepton_lead->charge() << ", pdgId = " << fakeableLepton_lead->pdgId() << "; "
                            << " subleading lepton: charge = " << fakeableLepton_sublead->charge() << ", pdgId = " << fakeableLepton_sublead->pdgId() << ")\n";
                }
                continue;
              }
            }
            evtWeightRecorder.record_chargeMisIdProb(prob_chargeMisId_sum);
          }
          else if ( numNominalLeptons == 2 && numNominalHadTaus == 1 )
          {
            double prob_chargeMisId = 1.;
            if ( event.fakeableLeptons().size() == 2 && event.fakeableHadTaus().size() == 1 )
            {
              const RecoLepton* fakeableLepton_lead = event.fakeableLeptons().at(0);
              const RecoLepton* fakeableLepton_sublead = event.fakeableLeptons().at(1);
              const RecoHadTau* fakeableHadTau = event.fakeableHadTaus().at(0);
              if ( fakeableLepton_lead->charge()*fakeableLepton_sublead->charge() > 0 )
              {
                // CV: apply charge misidentification probability to lepton of same charge as hadronic tau
                //    (if the lepton of charge opposite to the charge of the hadronic tau "flips",
                //     the event has sum of charges equal to three and fails "lepton+tau charge" cut)
                if ( fakeableLepton_lead->charge()*fakeableHadTau->charge()    > 0 ) prob_chargeMisId *= chargeMisIdRateInterface.get(fakeableLepton_lead);
                if ( fakeableLepton_sublead->charge()*fakeableHadTau->charge() > 0 ) prob_chargeMisId *= chargeMisIdRateInterface.get(fakeableLepton_sublead);
              }
              else if ( fakeableLepton_lead->charge()*fakeableLepton_sublead->charge() < 0 )
              {
                // CV: apply charge misidentification probability to lepton of opposite charge as hadronic tau
                //    (if the lepton of same charge as the hadronic tau "flips",
                //     the event has sum of charges equal to one and fails "lepton+tau charge" cut)
                if ( fakeableLepton_lead->charge()*fakeableHadTau->charge()    < 0 ) prob_chargeMisId *= chargeMisIdRateInterface.get(fakeableLepton_lead);
                if ( fakeableLepton_sublead->charge()*fakeableHadTau->charge() < 0 ) prob_chargeMisId *= chargeMisIdRateInterface.get(fakeableLepton_sublead);
              } else assert(0);
              // Karl: reject the event, if the applied probability of charge misidentification is 0. This can happen only if
              //       1) both selected leptons are muons (their misId prob is 0).
              //       2) one lepton is a muon and the other is an electron, and the muon has the same sign as the selected tau.
              if ( prob_chargeMisId == 0. )
              {
                if ( run_lumi_eventSelector )
                {
                  std::cout << "event " << event.eventInfo().str() << " FAILS charge flip selection\n"
                            << "(leading lepton: charge = " << fakeableLepton_lead->charge() << ", pdgId = " << fakeableLepton_lead->pdgId() << ";"
                            << " subleading lepton: charge = " << fakeableLepton_sublead->charge() << ", pdgId = " << fakeableLepton_sublead->pdgId() << ";" 
                            << " hadTau: charge = " << fakeableHadTau->charge() << ")\n";
                }
                continue;
              }
            }
            evtWeightRecorder.record_chargeMisIdProb(prob_chargeMisId);
          }
          else 
          {
            throw cmsException("produceNtuple", __LINE__) 
              << "Configuration parameter 'apply_chargeMisIdRate' = " << apply_chargeMisIdRate 
              << " not supported for categories with " << numNominalLeptons << " lepton(s) and " << numNominalHadTaus << " hadronic tau(s) !!";
          }
        }
      }
std::cout << "break-point 32 reached" << std::endl;
      for ( auto writer : writers )
      {
        writer->set_central_or_shift(central_or_shift);
        writer->write(event, evtWeightRecorder);
      }
    }
std::cout << "break-point 33 reached" << std::endl;
    outputTree->Fill();
  }
std::cout << "break-point 34 reached" << std::endl;
  TFileDirectory outputDir = fs.mkdir("events");
  outputDir.cd();
std::cout << "break-point 35 reached" << std::endl;
  TTree* outputTree_selected = outputTree->CopyTree(selection.data());
  Float_t evtWeight;
  outputTree_selected->SetBranchAddress("evtWeight", &evtWeight);
  int selectedEntries = 0;
  double selectedEntries_weighted = 0.;
  TH1* histogram_selectedEntries = fs.make<TH1D>("selectedEntries", "selectedEntries", 1, -0.5, +0.5);
  int numEntries = outputTree_selected->GetEntries();
  for ( int idxEntry = 0; idxEntry < numEntries; ++idxEntry )
  {
    outputTree_selected->GetEntry(idxEntry);
    ++selectedEntries;
    selectedEntries_weighted += evtWeight;
    histogram_selectedEntries->Fill(0.);
  }
  outputTree_selected->Write();
std::cout << "break-point 36 reached" << std::endl;
  std::cout << "max num. Entries = " << inputTree->getCumulativeMaxEventCount()
            << " (limited by " << maxEvents << ") processed in "
            << inputTree->getProcessedFileCount() << " file(s) (out of "
            << inputTree->getFileCount() << ")\n"
            << " analyzed = " << analyzedEntries << '\n'
            << " selected = " << selectedEntries << " (weighted = " << selectedEntries_weighted << ")" << std::endl;
std::cout << "break-point 37 reached" << std::endl;
//--- memory clean-up
  delete run_lumi_eventSelector;
std::cout << "break-point 38 reached" << std::endl;
  delete eventReader;
  delete l1PreFiringWeightReader;
  delete lheInfoReader;
  delete psWeightReader;
std::cout << "break-point 39 reached" << std::endl;
  delete dataToMCcorrectionInterface;
  delete jetToLeptonFakeRateInterface;
  delete jetToHadTauFakeRateInterface;
  delete btagSFRatioInterface;
std::cout << "break-point 40 reached" << std::endl;
  for ( auto writer : writers )
  {
    delete writer;
  }
std::cout << "break-point 41 reached" << std::endl;
  delete inputTree;
  delete outputTree;
  delete outputTree_selected;
std::cout << "break-point 42 reached" << std::endl;
  clock.Show("produceNtuple");
std::cout << "break-point 43 reached" << std::endl;
  return EXIT_SUCCESS;
}
