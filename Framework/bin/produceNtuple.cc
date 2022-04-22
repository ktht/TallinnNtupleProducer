
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
#include "PhysicsTools/FWLite/interface/TFileService.h"                                         // fwlite::TFileService

#include "TallinnNtupleProducer/CommonTools/interface/cmsException.h"                           // cmsException
#include "TallinnNtupleProducer/CommonTools/interface/Era.h"                                    // Era, get_era()
#include "TallinnNtupleProducer/CommonTools/interface/merge_systematic_shifts.h"                // merge_systematic_shifts()
#include "TallinnNtupleProducer/CommonTools/interface/tH_auxFunctions.h"                        // get_tH_SM_str()
#include "TallinnNtupleProducer/EvtWeightTools/interface/BtagSFRatioInterface.h"                // BtagSFRatioInterface
#include "TallinnNtupleProducer/EvtWeightTools/interface/ChargeMisIdRateInterface.h"            // ChargeMisIdRateInterface
#include "TallinnNtupleProducer/EvtWeightTools/interface/Data_to_MC_CorrectionInterface_2016.h" // Data_to_MC_CorrectionInterface_2016
#include "TallinnNtupleProducer/EvtWeightTools/interface/Data_to_MC_CorrectionInterface_2017.h" // Data_to_MC_CorrectionInterface_2017
#include "TallinnNtupleProducer/EvtWeightTools/interface/Data_to_MC_CorrectionInterface_2018.h" // Data_to_MC_CorrectionInterface_2018
#include "TallinnNtupleProducer/EvtWeightTools/interface/EvtWeightRecorder.h"                   // EvtWeightRecorder
#include "TallinnNtupleProducer/EvtWeightTools/interface/HadTauFakeRateInterface.h"             // HadTauFakeRateInterface
#include "TallinnNtupleProducer/EvtWeightTools/interface/HHWeightInterfaceCouplings.h"          // HHWeightInterfaceCouplings
#include "TallinnNtupleProducer/EvtWeightTools/interface/HHWeightInterfaceLO.h"                 // HHWeightInterfaceLO
#include "TallinnNtupleProducer/EvtWeightTools/interface/HHWeightInterfaceNLO.h"                // HHWeightInterfaceNLO
#include "TallinnNtupleProducer/EvtWeightTools/interface/LeptonFakeRateInterface.h"             // LeptonFakeRateInterface
#include "TallinnNtupleProducer/Framework/interface/GenMatchInterface.h"                        // GenMatchInterface
#include "TallinnNtupleProducer/Framework/interface/TTreeWrapper.h"                             // TTreeWrapper
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

//--- read python configuration parameters
  if ( !edm::readPSetsFrom(argv[1])->existsAs<edm::ParameterSet>("process") )
    throw cmsException("produceNtuple", __LINE__) << "No ParameterSet 'process' found in configuration file = " << argv[1] << " !!";

  edm::ParameterSet cfg = edm::readPSetsFrom(argv[1])->getParameter<edm::ParameterSet>("process");

  edm::ParameterSet cfg_produceNtuple = cfg.getParameter<edm::ParameterSet>("produceNtuple");
  AnalysisConfig analysisConfig("produceNtuple", produceNtuple);

  std::string treeName = cfg_produceNtuple.getParameter<std::string>("treeName");

  std::string process_string = cfg_produceNtuple.getParameter<std::string>("process");
  const bool isMC_tH     = analysisConfig.isMC_tH();
  const bool isMC_VH     = analysisConfig.isMC_VH();
  const bool isMC_H      = analysisConfig.isMC_H();
  const bool isMC_HH     = analysisConfig.isMC_HH();
  const bool isMC_EWK    = analysisConfig.isMC_EWK();
  const bool isMC_signal = analysisConfig.isMC_ttH();
  const bool isSignal    = isMC_signal || isMC_tH || isMC_VH || isMC_HH || isMC_H;

  std::string era_string = cfg_produceNtuple.getParameter<std::string>("era");
  const Era era = get_era(era_string);

  bool isMC = cfg_produceNtuple.getParameter<bool>("isMC");
  edm::VParameterSet lumiScale = cfg_produceNtuple.getParameter<edm::VParameterSet>("lumiScale");
  bool apply_genWeight = cfg_produceNtuple.getParameter<bool>("apply_genWeight");
  bool hasLHE = cfg_produceNtuple.getParameter<bool>("hasLHE");
  bool hasPS = cfg_produceNtuple.getParameter<bool>("hasPS");
  bool apply_LHE_nom = cfg_produceNtuple.getParameter<bool>("apply_LHE_nom");
  std::string apply_topPtReweighting_str = cfg_produceNtuple.getParameter<std::string>("apply_topPtReweighting");
  bool apply_topPtReweighting = ! apply_topPtReweighting_str.empty();
  bool apply_l1PreFireWeight = cfg_produceNtuple.getParameter<bool>("apply_l1PreFireWeight");
  bool apply_btagSFRatio = cfg_produceNtuple.getParameter<bool>("applyBtagSFRatio");

  unsigned int numNominalLeptons = cfg_produceNtuple.getParameter<unsigned int>("numNominalLeptons");
  unsigned int numNominalHadTaus = cfg_produceNtuple.getParameter<unsigned int>("numNominalHadTaus");

  bool apply_chargeMisIdRate = cfg_produceNtuple.getParameter<bool>("applyChargeMisIdRate");

  std::string selection = cfg_produceNtuple.getParameter<std::string>("selection");

  bool isDEBUG = cfg_produceNtuple.getParameter<bool>("isDEBUG");

  std::vector<std::string> systematic_shifts;
  // CV: process all systematic uncertainties supported by EventReader class (only for MC)
  if ( isMC_ )
  {
    merge_systematic_shifts(systematic_shifts, EventReader::get_supported_systematics());
  }
  // CV: add central value (for data and MC)
  systematic_shifts.insert("central");

  edm::ParameterSet cfg_dataToMCcorrectionInterface;
  cfg_dataToMCcorrectionInterface.addParameter<std::string>("era", era_string);
  cfg_dataToMCcorrectionInterface.addParameter<std::string>("hadTauSelection", hadTauSelection_part2);
  cfg_dataToMCcorrectionInterface.addParameter<int>("hadTauSelection_antiElectron", hadTauSelection_antiElectron);
  cfg_dataToMCcorrectionInterface.addParameter<int>("hadTauSelection_antiMuon", hadTauSelection_antiMuon);
  cfg_dataToMCcorrectionInterface.addParameter<std::string>("lep_mva_wp", lep_mva_wp);
  cfg_dataToMCcorrectionInterface.addParameter<bool>("isDEBUG", isDEBUG);
  Data_to_MC_CorrectionInterface_Base * dataToMCcorrectionInterface = nullptr;
  switch(era)
  {
    case Era::k2016: dataToMCcorrectionInterface = new Data_to_MC_CorrectionInterface_2016(cfg_dataToMCcorrectionInterface); break;
    case Era::k2017: dataToMCcorrectionInterface = new Data_to_MC_CorrectionInterface_2017(cfg_dataToMCcorrectionInterface); break;
    case Era::k2018: dataToMCcorrectionInterface = new Data_to_MC_CorrectionInterface_2018(cfg_dataToMCcorrectionInterface); break;
    default: throw cmsException("produceNtuple", __LINE__) << "Invalid era = " << static_cast<int>(era);
  }
  const ChargeMisIdRate chargeMisIdRate(era);

  edm::ParameterSet cfg_leptonFakeRateWeight = cfg_produceNtuple.getParameter<edm::ParameterSet>("leptonFakeRateWeight");
  cfg_leptonFakeRateWeight.addParameter<std::string>("era", era_string);
  LeptonFakeRateInterface* jetToLeptonFakeRateInterface = new LeptonFakeRateInterface(cfg_leptonFakeRateWeight);

  edm::ParameterSet cfg_hadTauFakeRateWeight = cfg_produceNtuple.getParameter<edm::ParameterSet>("hadTauFakeRateWeight");
  cfg_hadTauFakeRateWeight.addParameter<std::string>("hadTauSelection", hadTauSelection_part2);
  HadTauFakeRateInterface* jetToHadTauFakeRateInterface = new HadTauFakeRateInterface(cfg_hadTauFakeRateWeight);

  bool redoGenMatching = cfg_analyze.getParameter<bool>("redoGenMatching");
  bool genMatchingByIndex = cfg_analyze.getParameter<bool>("genMatchingByIndex");

  std::string selEventsFileName = cfg_analyze.getParameter<std::string>("selEventsFileName");
  std::cout << "selEventsFileName = " << selEventsFileName << std::endl;
  RunLumiEventSelector* run_lumi_eventSelector = 0;
  if ( selEventsFileName_input != "" ) {
    edm::ParameterSet cfg_run_lumi_eventSelector;
    cfg_run_lumi_eventSelector.addParameter<std::string>("inputFileName", selEventsFileName);
    cfg_run_lumi_eventSelector.addParameter<std::string>("separator", ":");
    run_lumi_eventSelector = new RunLumiEventSelector(cfg_run_lumi_eventSelector);
  }

  fwlite::InputSource inputFiles(cfg);
  int maxEvents = inputFiles.maxEvents();
  std::cout << " maxEvents = " << maxEvents << std::endl;
  unsigned reportEvery = inputFiles.reportAfter();

  fwlite::OutputFiles outputFile(cfg);
  fwlite::TFileService fs = fwlite::TFileService(outputFile.file().data());

  TTreeWrapper* inputTree = new TTreeWrapper(treeName.data(), inputFiles.files(), maxEvents);
  std::cout << "Loaded " << inputTree->getFileCount() << " file(s).\n";

  EventReader* eventReader = new EventReader();
  inputTree->registerReader(eventReader);

  TTree* outputTree = new TTree("events");

//--- declare event-level variables
  EventInfo eventInfo(analysisConfig);
  if ( isMC )
  {
    const double ref_genWeight = cfg_analyze.getParameter<double>("ref_genWeight");
    eventInfo.set_refGetWeight(ref_genWeight);
  }
  const std::vector<std::pair<std::string, int>> evt_htxs_binning = get_htxs_binning(isMC_signal);
  eventInfo.read_htxs(!evt_htxs_binning.empty());
  
  EventInfoReader eventInfoReader(&eventInfo);
  if ( apply_topPtReweighting )
  {
    eventInfoReader.setTopPtRwgtBranchName(apply_topPtReweighting_str);
  }
  inputTree->registerReader(&eventInfoReader);

  L1PreFiringWeightReader* l1PreFiringWeightReader = nullptr;
  if ( apply_l1PreFireWeight )
  {
    l1PreFiringWeightReader = new L1PreFiringWeightReader(era);
    inputTree->registerReader(l1PreFiringWeightReader);
  }

  BtagSFRatioInterface* btagSFRatioInterface = nullptr;
  if ( apply_btagSFRatio )
  {
    const edm::ParameterSet btagSFRatio = cfg_analyze.getParameterSet("btagSFRatio");
    btagSFRatioInterface = new BtagSFRatioInterface(btagSFRatio);
  }

  edm::VParameterSet cfg_writers = cfg_analyze.getParameterSetVector("writerPlugins");
  std::vector<WriterBase*> writers;
  for ( auto cfg_writer : cfg_writers )
  {
    std::string pluginType = cfg_writer.getParameterSet("pluginType");
    cfg_writer.addParameter<unsigned int>("numNominalLeptons", numNominalLeptons);
    cfg_writer.addParameter<unsigned int>("numNominalHadTaus", numNominalHadTaus);
    WriterBase* writer = WriterPluginFactory::get()->create(pluginType, cfg_writer);
    writer->setBranches(outputTree);
    writers.push_back(writer);
  }

  int analyzedEntries = 0;
  TH1* histogram_analyzedEntries = fs.make<TH1D>("analyzedEntries", "analyzedEntries", 1, -0.5, +0.5);
  while ( inputTree->hasNextEvent() && (!run_lumi_eventSelector || (run_lumi_eventSelector && !run_lumi_eventSelector->areWeDone())) )
  {
    if ( inputTree->canReport(reportEvery) )
    {
      std::cout << "processing Entry " << inputTree->getCurrentMaxEventIdx()
                << " or " << inputTree->getCurrentEventIdx() << " entry in #"
                << (inputTree->getProcessedFileCount() - 1)
                << " (" << eventInfo
                << ") file (" << selectedEntries << " Entries selected)\n";
    }
    ++analyzedEntries;
    histogram_analyzedEntries->Fill(0.);

    if ( run_lumi_eventSelector )
    { 
      if ( !(*run_lumi_eventSelector)(eventInfo) )
      {
        continue;
      }
      std::cout << "processing Entry " << inputTree->getCurrentMaxEventIdx() << ": " << eventInfo << '\n';
      if ( inputTree->isOpen() )
      {
        std::cout << "input File = " << inputTree->getCurrentFileName() << '\n';
      }
    }

    if ( isDEBUG ) 
    {
      std::cout << "event #" << inputTree->getCurrentMaxEventIdx() << ' ' << eventInfo << '\n';
    }

    for ( auto central_or_shift : systematic_shifts )
    {
      eventInfo.set_central_or_shift(central_or_shift);

      Event event = eventReader->read();

      EvtWeightRecorder evtWeightRecorder(central_or_shifts_local, central_or_shift_main, isMC);
      if ( isMC )
      {
        if ( apply_genWeight         ) evtWeightRecorder.record_genWeight(event.eventInfo());
        if ( eventWeightManager      ) evtWeightRecorder.record_auxWeight(eventWeightManager);
        if ( l1PreFiringWeightReader ) evtWeightRecorder.record_l1PrefireWeight(l1PreFiringWeightReader);
        if ( apply_topPtReweighting  ) evtWeightRecorder.record_toppt_rwgt(event.eventInfo().topPtRwgtSF);
        lheInfoReader->read();
        psWeightReader->read();
        evtWeightRecorder.record_lheScaleWeight(lheInfoReader);
        evtWeightRecorder.record_psWeight(psWeightReader);
        evtWeightRecorder.record_puWeight(&event.eventInfo());
        evtWeightRecorder.record_nom_tH_weight(&event.eventInfo());
        evtWeightRecorder.record_lumiScale(lumiScale);

//--- compute event-level weight for data/MC correction of b-tagging efficiency and mistag rate
//   (using the method "Event reweighting using scale factors calculated with a tag and probe method",
//    described on the BTV POG twiki https://twiki.cern.ch/twiki/bin/view/CMS/BTagShapeCalibration )
        evtWeightRecorder.record_btagWeight(event.selJetsAK4());
        if ( btagSFRatioInterface )
        {
          evtWeightRecorder.record_btagSFRatio(btagSFRatioInterface, event.selJetsAK4().size());
        }

        if ( isMC_EWK )
        {
          evtWeightRecorder.record_ewk_jet(event.selJetsAK4());
          evtWeightRecorder.record_ewk_bjet(event.selJetsAK4_btagMedium());
        }

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

        evtWeightRecorder.record_jetToLeptonRate(jetToLeptonFakeRateInterface, event.fakeableLeptons());
        evtWeightRecorder.record_jetToTauFakeRate(jetToHadTauFakeRateInterface, event.fakeableHadTaus());
        evtWeightRecorder.compute_FR();

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
                const double prob_chargeMisID_sum = chargeMisIdRate.get(fakeableLepton_lead, fakeableLepton_sublead);
              }
            }
            // Karl: reject the event, if the applied probability of charge misidentification is 0;
            //       note that this can happen only if both selected leptons are muons (their misId prob is 0).
            if ( prob_chargeMisID_sum == 0. )
            {
              if ( run_lumi_eventSelector )
              {
                std::cout << "event " << eventInfo.str() << " FAILS charge flip selection\n"
                          << "(leading lepton: charge = " << fakeableLepton_lead->charge() << ", pdgId = " << fakeableLepton_lead->pdgId() << "; "
                          << " subleading lepton: charge = " << fakeableLepton_sublead->charge() << ", pdgId = " << fakeableLepton_sublead->pdgId() << ")\n";
              }
              continue;
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
              const RecoHadTau* fakeableHadTau_sublead = event.fakeableHadTaus().at(0);
              if ( fakeableLepton_lead->charge()*fakeableLepton_sublead->charge() > 0 )
              {
                // CV: apply charge misidentification probability to lepton of same charge as hadronic tau
                //    (if the lepton of charge opposite to the charge of the hadronic tau "flips",
                //     the event has sum of charges equal to three and fails "lepton+tau charge" cut)
                if ( fakeableLepton_lead->charge()*fakeableHadTau->charge()    > 0 ) prob_chargeMisId *= chargeMisIdRate.get(fakeableLepton_lead);
                if ( fakeableLepton_sublead->charge()*fakeableHadTau->charge() > 0 ) prob_chargeMisId *= chargeMisIdRate.get(fakeableLepton_sublead);
              }
              else if ( fakeableLepton_lead->charge()*fakeableLepton_sublead->charge() < 0 )
              {
                // CV: apply charge misidentification probability to lepton of opposite charge as hadronic tau
                //    (if the lepton of same charge as the hadronic tau "flips",
                //     the event has sum of charges equal to one and fails "lepton+tau charge" cut)
                if ( fakeableLepton_lead->charge()*fakeableHadTau->charge()    < 0 ) prob_chargeMisId *= chargeMisIdRate.get(fakeableLepton_lead);
                if ( fakeableLepton_sublead->charge()*fakeableHadTau->charge() < 0 ) prob_chargeMisId *= chargeMisIdRate.get(fakeableLepton_sublead);
              } else assert(0);
              // Karl: reject the event, if the applied probability of charge misidentification is 0. This can happen only if
              //       1) both selected leptons are muons (their misId prob is 0).
              //       2) one lepton is a muon and the other is an electron, and the muon has the same sign as the selected tau.
              if ( prob_chargeMisId == 0. )
              {
              if ( run_lumi_eventSelector )
              {
                std::cout << "event " << eventInfo.str() << " FAILS charge flip selection\n"
                          << "(leading lepton: charge = " << fakeableLepton_lead->charge() << ", pdgId = " << fakeableLepton_lead->pdgId() << ";"
                          << " subleading lepton: charge = " << fakeableLepton_sublead->charge() << ", pdgId = " << fakeableLepton_sublead->pdgId() << ";" 
                          << " hadTau: charge = " << selHadTau->charge() << ")\n";
              }
              continue;
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
 
      for ( auto writer : writers )
      {
        writer->set_central_or_shift(central_or_shift);
        writer->write(event, evtWeightRecorder);
      }
    }

    outputTree->Fill();
  }

  TDirectory* outputDir = fs.make<TDirectory>("events");
  outputDir->cd();

  TTree* outputTree_selected = outputTree->CopyTree(selection.data());
  Float_t evtWeight;
  outputTree_selected->SetBranchAddress("evtWeight", &evtWeight);
  int selectedEntries = 0;
  double selectedEntries_weighted = 0.;
  TH1* histogram_selectedEntries = fs.make<TH1D>("selectedEntries", "selectedEntries", 1, -0.5, +0.5);
  int numEntries = outputTree_selected->GetEntries();
  for ( int idxEntry = 0; idxEntry < numEntries; ++idxEntry )
  {
    idxEntry->GetEntry(idxEntry);
    ++selectedEntries;
    selectedEntries_weighted += evtWeight;
    histogram_selectedEntries->Fill(0.);
  }
  outputTree_selected->write();

  std::cout << "max num. Entries = " << inputTree->getCumulativeMaxEventCount()
            << " (limited by " << maxEvents << ") processed in "
            << inputTree->getProcessedFileCount() << " file(s) (out of "
            << inputTree->getFileCount() << ")\n"
            << " analyzed = " << analyzedEntries << '\n'
            << " selected = " << selectedEntries << " (weighted = " << selectedEntries_weighted << ")" << std::endl;

//--- memory clean-up
  delete run_lumi_eventSelector;

  delete selEventsFile;

  delete eventReader;
  delete l1PreFiringWeightReader;

  delete dataToMCcorrectionInterface;
  delete jetToLeptonFakeRateInterface;
  delete jetToTauFakeRateInterface;
  delete btagSFRatioInterface;

  delete inputTree;
  delete outputTree;
  delete outputTree_selected;
  
  clock.Show("produceNtuple");

  return EXIT_SUCCESS;
}