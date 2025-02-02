#ifndef TallinnNtupleProducer_Writers_GenPhotonFilterWriter_h
#define TallinnNtupleProducer_Writers_GenPhotonFilterWriter_h

#include "FWCore/ParameterSet/interface/ParameterSet.h"                       // edm::ParameterSet

#include "TallinnNtupleProducer/EvtWeightTools/interface/EvtWeightRecorder.h" // EvtWeightRecorder
#include "TallinnNtupleProducer/Objects/interface/Event.h"                    // Event
#include "TallinnNtupleProducer/Writers/interface/WriterBase.h"               // WriterBase

#include <string>                                                             // std::string
#include <vector>                                                             // std::vector

// forward declarations
class GenParticleReader;
class GenPhotonReader;
class GenPhotonFilter;
class TTree;
class TTreeWrapper;

class GenPhotonFilterWriter : public WriterBase
{
 public:
  GenPhotonFilterWriter(const edm::ParameterSet & cfg);
  ~GenPhotonFilterWriter();

  /**
   * @brief Call inputTree->registerReader(reader)
   *        for "private" GenPhotonReader and GenParticleReader instances
   */
  void
  registerReaders(TTreeWrapper * inputTree);

  /**
   * @brief Call tree->Branch for all branches
   */
  void
  setBranches(TTree * outputTree);

 private:
  /**
   * @brief Write relevant information to tree
   */
  void
  writeImp(const Event & event, const EvtWeightRecorder & evtWeightRecorder);

  /// name of branch in outputTree
  std::string branchName_;

  /// names of branches in inputTree
  std::string branchName_genPhotons_; 
  std::string branchName_genProxyPhotons_;
  std::string branchName_genParticlesFromHardProcess_;

  GenPhotonReader * genPhotonReader_;
  GenPhotonReader * genProxyPhotonReader_;
  GenParticleReader * genParticlesFromHardProcessReader_;

  GenPhotonFilter * genPhotonFilter_;
  bool apply_genPhotonFilter_;

  bool passesGenPhotonFilter_;
};

#endif // TallinnNtupleProducer_Writers_GenPhotonFilterFilterWriter_h
