#include "TallinnNtupleProducer/Writers/interface/WriterBase.h"

#include "TallinnNtupleProducer/CommonTools/interface/contains.h"     // contains()
#include "TallinnNtupleProducer/CommonTools/interface/TTreeWrapper.h" // TTreeWrapper

WriterBase::WriterBase(const edm::ParameterSet & cfg)
  : current_central_or_shift_("central")
{}

WriterBase::~WriterBase()
{}

void
WriterBase::registerReaders(TTreeWrapper * inputTree)
{}

void
WriterBase::setBranches(TTree * outputTree)
{
  // CV: this function should never be called, 
  //     as it is supposed to the overwritten by all derrived classes !!
  assert(0);
}

void
WriterBase::set_central_or_shift(const std::string & central_or_shift) const
{
  current_central_or_shift_ = central_or_shift;
}

void
WriterBase::write(const Event & event, const EvtWeightRecorder & evtWeightRecorder)
{
  if ( contains(supported_systematics_, current_central_or_shift_) )
  {
    writeImp(event, evtWeightRecorder);
  }
}

EDM_REGISTER_PLUGINFACTORY(WriterPluginFactory, "WriterPluginFactory");
