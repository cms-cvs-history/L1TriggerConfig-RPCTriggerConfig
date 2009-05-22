import FWCore.ParameterSet.Config as cms

process = cms.Process("dump")

# process.load("L1TriggerConfig.RPCTriggerConfig.L1RPCConfig_cff")

useGlobalTag = 'IDEAL_30X'
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.GlobalTag.globaltag = useGlobalTag + '::All'




process.source = cms.Source("EmptySource")

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(1)
)


process.write = cms.EDAnalyzer("DumpL1RPCConfig",
          fileName = cms.string("L1RPCConfigDump.txt"),
)


process.p1 = cms.Path(process.write)