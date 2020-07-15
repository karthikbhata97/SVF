//===- VersionedFlowSensitive.h -- Flow-sensitive pointer analysis---------------------//

/*
 * VersionedFlowSensitiveAnalysis.h
 *
 *  Created on: Jun 26, 2020
 *      Author: Mohamad Barbar
 */

#ifndef VFS_H_
#define VFS_H_

#include "Graphs/SVFGOPT.h"
#include "MSSA/SVFGBuilder.h"
#include "WPA/FlowSensitive.h"
#include "WPA/WPAFSSolver.h"
class AndersenWaveDiff;
class SVFModule;

/*!
 * Flow sensitive whole program pointer analysis
 */
class VersionedFlowSensitive : public FlowSensitive
{
public:
    enum VersionType {
        CONSUME,
        YIELD,
    };

    /// Constructor
    VersionedFlowSensitive(PAG *_pag, PTATY type = VFS_WPA) : FlowSensitive(_pag, type) { }

    /// Flow sensitive analysis
    // virtual void analyze(SVFModule* svfModule) override;

    /// Initialize analysis
    virtual void initialize() override;

    /// Finalize analysis
    virtual void finalize() override;

    /// Get PTA name
    virtual const std::string PTAName() const override
    {
        return "VersionedFlowSensitive";
    }

    /// Methods to support type inquiry through isa, cast, and dyn_cast
    //@{
    static inline bool classof(const VersionedFlowSensitive *)
    {
        return true;
    }
    static inline bool classof(const PointerAnalysis *pta)
    {
        return pta->getAnalysisTy() == VFS_WPA;
    }
    //@}

protected:
    virtual bool processLoad(const LoadSVFGNode* load) override;
    virtual bool processStore(const StoreSVFGNode* store) override;
    virtual void processNode(NodeID n) override;
    virtual void updateConnectedNodes(const SVFGEdgeSetTy& newEdges);

    /// Override to do nothing. Instead, we will use propagateVersion when necessary.
    virtual bool propAlongIndirectEdge(const IndirectSVFGEdge* edge) override { return false; }

private:
    /// Precolour the split SVFG.
    void precolour(void);
    /// Colour the precoloured split SVFG.
    void colour(void);
    /// Melds v2 into v1 (in place), returns whether a change occurred.
    bool meld(MeldVersion &mv1, MeldVersion &mv2);

    /// Moves meldConsume/Yield to consume/yield.
    void mapMeldVersions(DenseMap<NodeID, DenseMap<NodeID, MeldVersion>> &from,
                         DenseMap<NodeID, DenseMap<NodeID, Version>> &to);

    /// Returns whether l is a delta node.
    bool delta(NodeID l) const;

    /// Returns a new MeldVersion for o.
    MeldVersion newMeldVersion(NodeID o);
    /// Whether l has a consume/yield version for o.
    bool hasVersion(NodeID l, NodeID o, enum VersionType v) const;

    /// Determine which versions rely on which versions, and which statements
    /// rely on which versions.
    void determineReliance(void);

    /// Propagates version v of o to any version of o which relies on v.
    /// Recursively applies to reliant versions till no new changes are made.
    /// Adds any statements which rely on any changes made to the worklist.
    void propagateVersion(NodeID o, Version v);

    /// SVFG node (label) x object -> version to consume.
    /// Used during colouring.
    DenseMap<NodeID, DenseMap<NodeID, MeldVersion>> meldConsume;
    /// SVFG node (label) x object -> version to yield.
    /// Used during colouring.
    DenseMap<NodeID, DenseMap<NodeID, MeldVersion>> meldYield;
    /// Object -> MeldVersion counter.
    DenseMap<NodeID, unsigned> meldVersions;

    /// Actual consume map.
    DenseMap<NodeID, DenseMap<NodeID, Version>> consume;
    /// Actual yield map.
    DenseMap<NodeID, DenseMap<NodeID, Version>> yield;

    /// o -> (version -> versions which rely on it).
    DenseMap<NodeID, DenseMap<Version, DenseSet<Version>>> versionReliance;
    /// o x version -> statement nodes which rely on that o/version.
    DenseMap<NodeID, DenseMap<Version, DenseSet<NodeID>>> stmtReliance;

    /// Worklist for performing meld colouring, takes SVFG node l.
    FIFOWorkList<NodeID> vWorklist;

    /// Points-to DS for working with versions.
    BVDataPTAImpl::VDFPTDataTy *vPtD;
};

#endif /* VFS_H_ */
