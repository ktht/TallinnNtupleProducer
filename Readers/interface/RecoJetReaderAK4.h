#ifndef TallinnNtupleProducer_Readers_RecoJetReaderAK4_h
#define TallinnNtupleProducer_Readers_RecoJetReaderAK4_h

#include "FWCore/ParameterSet/interface/ParameterSet.h"         // edm::ParameterSet

#include "TallinnNtupleProducer/Objects/interface/RecoJetAK4.h" // RecoJetAK4
#include "TallinnNtupleProducer/Readers/interface/ReaderBase.h" // ReaderBase

#include <map>                                                  // std::map

// forward declarations
class TTree;
class GenLeptonReader;
class GenHadTauReader;
class GenJetReader;

enum class Btag;
enum class Era;

class RecoJetReaderAK4 : public ReaderBase
{
 public:
  RecoJetReaderAK4(const edm::ParameterSet & cfg);
  ~RecoJetReaderAK4() override;

  void
  setPtMass_central_or_shift(int central_or_shift);

  void
  setBranchName_BtagWeight(int central_or_shift);

  void
  read_Btag(Btag btag);

  void
  read_ptMass_systematics(bool flag);

  void
  read_btag_systematics(bool flag);

  /**
   * @brief Call tree->SetBranchAddress for all RecoJet branches
   */
  std::vector<std::string>
  setBranchAddresses(TTree * tree) override;

  /**
   * @brief Read branches from tree and use information to fill collection of RecoJet objects
   * @return Collection of RecoJet objects
   */
  std::vector<RecoJetAK4>
  read() const;

  /**
    * @brief Return list of systematic uncertainties supported by RecoJetReaderAK4 class
    */
  static
  std::vector<std::string>
  get_supported_systematics()
  {
    return {  
      "CMS_ttHl_JESAbsoluteUp",           "CMS_ttHl_JESAbsoluteDown",
      "CMS_ttHl_JESAbsolute_EraUp",       "CMS_ttHl_JESAbsolute_EraDown",
      "CMS_ttHl_JESBBEC1Up",              "CMS_ttHl_JESBBEC1Down",
      "CMS_ttHl_JESBBEC1_EraUp",          "CMS_ttHl_JESBBEC1_EraDown",
      "CMS_ttHl_JESEC2Up",                "CMS_ttHl_JESEC2Down",
      "CMS_ttHl_JESEC2_EraUp",            "CMS_ttHl_JESEC2_EraDown",
      "CMS_ttHl_JESFlavorQCDUp",          "CMS_ttHl_JESFlavorQCDDown",
      "CMS_ttHl_JESHFUp",                 "CMS_ttHl_JESHFDown",
      "CMS_ttHl_JESHF_EraUp",             "CMS_ttHl_JESHF_EraDown",
      "CMS_ttHl_JESRelativeBalUp",        "CMS_ttHl_JESRelativeBalDown",
      "CMS_ttHl_JESRelativeSample_EraUp", "CMS_ttHl_JESRelativeSample_EraDown",
      "CMS_ttHl_JESHEMDown", // addresses HEM15/16, see https://hypernews.cern.ch/HyperNews/CMS/get/JetMET/2000.html
    };
  }

 protected:
 /**
   * @brief Initialize names of branches to be read from tree
   */
  void
  setBranchNames();

  Era era_;
  bool isMC_;
  unsigned int max_nJets_;
  std::string branchName_num_;
  std::string branchName_obj_;

  /**
   * @brief Read branches containing information on matching of RecoJet objects
   *        to generator level electrons, muons, hadronic taus, and jets from tree
   *        and add this information to collection of RecoJet objects given as function argument
   */
  void
  readGenMatching(std::vector<RecoJetAK4> & jets) const;

  GenLeptonReader * genLeptonReader_;
  GenHadTauReader * genHadTauReader_;
  GenJetReader * genJetReader_;
  bool readGenMatching_;
 
  std::string branchName_eta_;
  std::string branchName_phi_;
  std::string branchName_jetCharge_;
  std::string branchName_QGDiscr_;
  std::string branchName_bRegCorr_;
  std::string branchName_bRegRes_;
  std::string branchName_pullEta_;
  std::string branchName_pullPhi_;
  std::string branchName_pullMag_;
  std::string branchName_jetId_;
  std::string branchName_puId_;
  std::string branchName_jetIdx_;
  std::string branchName_genMatchIdx_;

  std::map<int, std::string> branchNames_pt_systematics_;
  std::map<int, std::string> branchNames_mass_systematics_;
  std::map<Btag, std::string> branchNames_btag_;
  std::map<Btag, std::map<int, std::string>> branchNames_BtagWeight_systematics_;

  Btag btag_;
  int btag_central_or_shift_;
  int ptMassOption_;
  bool read_ptMass_systematics_;
  bool read_btag_systematics_;

  UInt_t nJets_;
  Float_t * jet_eta_;
  Float_t * jet_phi_;
  Float_t * jet_charge_;
  Float_t * jet_QGDiscr_;
  Float_t * jet_bRegCorr_;
  Float_t * jet_bRegRes_;
  Float_t * jet_pullEta_;
  Float_t * jet_pullPhi_;
  Float_t * jet_pullMag_;
  Int_t * jet_jetId_;
  Int_t * jet_puId_;
  Int_t * jet_jetIdx_;
  Int_t * jet_genMatchIdx_;

  std::map<int, Float_t *> jet_pt_systematics_;
  std::map<int, Float_t *> jet_mass_systematics_;
  std::map<Btag, Float_t *> jet_BtagCSVs_;
  std::map<Btag, std::map<int, Float_t *>> jet_BtagWeights_systematics_;

  // CV: make sure that only one RecoJetReader instance exists for a given branchName,
  //     as ROOT cannot handle multiple TTree::SetBranchAddress calls for the same branch.
  static std::map<std::string, int> numInstances_;
  static std::map<std::string, RecoJetReaderAK4 *> instances_;
};

#endif // TallinnNtupleProducer_Readers_RecoJetReaderAK4_h
