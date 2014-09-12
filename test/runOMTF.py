import FWCore.ParameterSet.Config as cms
process = cms.Process("MakePatterns")
import os
import sys

inputFiles =  cms.vstring("/home/akalinow/scratch/CMS/OverlapTrackFinder/Crab/SingleMuFullEtaTestSample/Tree/v1/data//l1RpcTree_p19_p_v1_FullEta_1_1_mqa.root")

process.source = cms.Source("EmptySource")
process.maxEvents = cms.untracked.PSet( input = cms.untracked.int32(1))

process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.GlobalTag.globaltag = 'POSTLS162_V1::All'
process.load('Configuration.StandardSequences.GeometryRecoDB_cff')

path = "/home/akalinow/scratch/CMS/OverlapTrackFinder/Dev4/CMSSW_7_0_1/src/UserCode/OMTFSimulation/data/"

process.omtfAnalysis = cms.EDAnalyzer("OMTFROOTReader",
  treeFileNames = cms.vstring(inputFiles),
  omtf = cms.PSet(
    configXMLFile = cms.string(path+"hwToLogicLayer.xml"),
    patternsXMLFile =   cms.string(path+"Patterns_chMinus.xml")
 ),
  anaEff      = cms.PSet( maxDR= cms.double(9999.)),
)

process.p = cms.Path(process.omtfAnalysis)