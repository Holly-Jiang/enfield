#include "enfield/Transform/Allocators/QbitAllocator.h"
#include "enfield/Transform/RenameQbitsPass.h"
#include "enfield/Transform/InlineAllPass.h"
#include "enfield/Transform/PassCache.h"
#include "enfield/Transform/Utils.h"
#include "enfield/Arch/ArchGraph.h"
#include "enfield/Analysis/NodeVisitor.h"
#include "enfield/Support/RTTI.h"
#include "enfield/Support/uRefCast.h"
#include "enfield/Support/Timer.h"

#include <iterator>
#include <cassert>

// ------------------ Solution Implementer ----------------------
namespace efd {
    class SolutionImplPass : public PassT<void>, public NodeVisitor {
        private:
            Solution& mData;

            XbitToNumber mXbitToNumber;
            std::vector<Node::Ref> mMap;

            std::unordered_map<Node::Ref, std::vector<Node::uRef>> mReplVector;

            uint32_t mDepIdx;

            /// \brief Gets the node mapped to the string version of \p ref.
            Node::uRef getMappedNode(Node::Ref ref);
            /// \brief Wraps \p ref with \p ifstmt if \p ifstmt is not nullptr.
            Node::uRef wrapWithIfNode(Node::uRef ref, NDIfStmt::Ref ifstmt);
            /// \brief Apply the swaps for the current dependency.
            void applyOperations(Node::Ref ref);

        public:
            SolutionImplPass(Solution& sol) : mData(sol) {}

            bool run(QModule::Ref qmod) override;
            void visit(NDQOpMeasure::Ref ref) override;
            void visit(NDQOpReset::Ref ref) override;
            void visit(NDQOpU::Ref ref) override;
            void visit(NDQOpBarrier::Ref ref) override;
            void visit(NDIfStmt::Ref ref) override;
            void visit(NDList::Ref ref) override;
            void visit(NDQOpCX::Ref ref) override;
            void visit(NDQOpGen::Ref ref) override;
    };
}

efd::Node::uRef efd::SolutionImplPass::getMappedNode(Node::Ref ref) {
    uint32_t id = mXbitToNumber.getQUId(ref->toString());
    return mMap[id]->clone();
}

efd::Node::uRef efd::SolutionImplPass::wrapWithIfNode
(Node::uRef ref, NDIfStmt::Ref ifstmt) {

    if (ifstmt != nullptr) {
        auto ifclone = uniqueCastForward<NDIfStmt>(ifstmt->clone());
        ifclone->setQOp(uniqueCastForward<NDQOp>(std::move(ref)));
        ref = std::move(ifclone);
    }

    return ref;
}

void efd::SolutionImplPass::applyOperations(Node::Ref ref) {
    auto& ops = mData.mOpSeqs[mDepIdx].second;
    ++mDepIdx;

    // If there is nothing to be done in this dep, simply return.
    if (ops.empty()) return;

    NDIfStmt::Ref ifstmt = efd::dynCast<NDIfStmt>(ref->getParent());

    Node::Ref key;
    if (ifstmt != nullptr) {
        key = ifstmt;
    } else {
        key = ref;
    }

    mReplVector[key] = std::vector<Node::uRef>();

    for (auto& op : ops) {
        switch (op.mK) {
            case Operation::K_OP_CNOT:
                {
                    NDQOp::uRef clone = uniqueCastForward<NDQOp>(ref->clone());

                    auto qargs = clone->getQArgs();
                    qargs->setChild(0, mMap[op.mU]->clone());
                    qargs->setChild(1, mMap[op.mV]->clone());

                    mReplVector[key].push_back(std::move(clone));
                }
                break;

            case Operation::K_OP_SWAP:
                mReplVector[key].push_back(
                        efd::CreateISwap(mMap[op.mU]->clone(), mMap[op.mV]->clone()));
                std::swap(mMap[op.mU], mMap[op.mV]);
                break;

            case Operation::K_OP_REV:
                {
                    Node::uRef call = efd::CreateIRevCX(
                                mMap[op.mU]->clone(),
                                mMap[op.mV]->clone());
                    call = wrapWithIfNode(std::move(call), ifstmt);
                    mReplVector[key].push_back(wrapWithIfNode(std::move(call), ifstmt));
                }
                break;

            case Operation::K_OP_LCNOT:
                {
                    Node::uRef call = efd::CreateILongCX(
                                mMap[op.mU]->clone(), 
                                mMap[op.mW]->clone(), 
                                mMap[op.mV]->clone());
                    call = wrapWithIfNode(std::move(call), ifstmt);
                    mReplVector[key].push_back(wrapWithIfNode(std::move(call), ifstmt));
                }
                break;
        }
    }
}

bool efd::SolutionImplPass::run(QModule::Ref qmod) {
    auto xtonpass = PassCache::Get<XbitToNumberWrapperPass>(qmod);

    mXbitToNumber = xtonpass->getData();
    mMap.assign(mXbitToNumber.getQSize(), nullptr);

    for (uint32_t i = 0, e = mXbitToNumber.getQSize(); i < e; ++i)
        mMap[i] = mXbitToNumber.getQNode(mData.mInitial[i]);

    mDepIdx = 0;
    for (auto it = qmod->stmt_begin(), end = qmod->stmt_end(); it != end; ++it) {
        (*it)->apply(this);
    }

    for (auto& pair : mReplVector) {
        if (!pair.second.empty())
            qmod->replaceStatement(pair.first, std::move(pair.second));
    }

    return true;
}

void efd::SolutionImplPass::visit(NDQOpMeasure::Ref ref) {
    ref->setQBit(getMappedNode(ref->getQBit()));
}

void efd::SolutionImplPass::visit(NDQOpReset::Ref ref) {
    ref->setQArg(getMappedNode(ref->getQArg()));
}

void efd::SolutionImplPass::visit(NDQOpU::Ref ref) {
    ref->setQArg(getMappedNode(ref->getQArg()));
}

void efd::SolutionImplPass::visit(NDQOpBarrier::Ref ref) {
    ref->getQArgs()->apply(this);
}

void efd::SolutionImplPass::visit(NDIfStmt::Ref ref) {
    ref->getQOp()->apply(this);
}

void efd::SolutionImplPass::visit(NDList::Ref ref) {
    for (uint32_t i = 0, e = ref->getChildNumber(); i < e; ++i) {
        ref->setChild(i, getMappedNode(ref->getChild(i)));
    }
}

void efd::SolutionImplPass::visit(NDQOpCX::Ref ref) {
    ref->setLhs(getMappedNode(ref->getLhs()));
    ref->setRhs(getMappedNode(ref->getRhs()));

    assert(ref == mData.mOpSeqs[mDepIdx].first &&
            "Wrong cnot dependency.");

    applyOperations(ref);
}

void efd::SolutionImplPass::visit(NDQOpGen::Ref ref) {
    ref->getQArgs()->apply(this);

    if (!mData.mOpSeqs.empty() && ref == mData.mOpSeqs[mDepIdx].first) {
        applyOperations(ref);
    }
}


// ------------------ QbitAllocator ----------------------
static efd::Stat<uint32_t> DepStat
("Dependencies", "The number of dependencies of this program.");
static efd::Stat<double> AllocTime
("AllocTime", "Time to allocate all qubits.");
static efd::Stat<double> InlineTime
("InlineTime", "Time to inline all gates.");
static efd::Stat<double> ReplaceTime
("ReplaceTime", "Time to replace all qubits to the corresponding architechture ones.");
static efd::Stat<double> RenameTime
("RenameTime", "Time to rename all qubits to the mapped qubits.");

efd::Stat<uint32_t> TotalCost
("TotalCost", "Total cost after allocating the qubits.");
efd::Opt<uint32_t> SwapCost
("-swap-cost", "Cost of using a swap function.", 7, false);
efd::Opt<uint32_t> RevCost
("-rev-cost", "Cost of using a reverse edge.", 4, false);
efd::Opt<uint32_t> LCXCost
("-lcx-cost", "Cost of using long cnot gate.", 10, false);

efd::QbitAllocator::QbitAllocator(ArchGraph::sRef archGraph) 
    : mInlineAll(false), mArchGraph(archGraph) {
}

void efd::QbitAllocator::inlineAllGates() {
    auto inlinePass = InlineAllPass::Create(mBasis);
    PassCache::Run(mMod, inlinePass.get());
}

void efd::QbitAllocator::replaceWithArchSpecs() {
    // Renaming program qbits to architecture qbits.
    RenameQbitPass::ArchMap toArchMap;

    auto xtn = PassCache::Get<XbitToNumberWrapperPass>(mMod);
    auto xbitToNumber = xtn->getData();

    for (uint32_t i = 0, e = xbitToNumber.getQSize(); i < e; ++i) {
        toArchMap[xbitToNumber.getQStrId(i)] = mArchGraph->getNode(i);
    }

    auto renamePass = RenameQbitPass::Create(toArchMap);
    PassCache::Run(mMod, renamePass.get());

    // Replacing the old qbit declarations with the architecture's qbit
    // declaration.
    mMod->removeAllQRegs();
    for (auto it = mArchGraph->reg_begin(), e = mArchGraph->reg_end(); it != e; ++it)
        mMod->insertReg(NDRegDecl::CreateQ
                (NDId::Create(it->first), NDInt::Create(std::to_string(it->second))));
}

bool efd::QbitAllocator::run(QModule::Ref qmod) {
    Timer timer;

    // Setting the class QModule.
    mMod = qmod;

    if (mInlineAll) {
        // Setting up timer ----------------
        timer.start();
        // ---------------------------------

        inlineAllGates();

        // Stopping timer and setting the stat -----------------
        timer.stop();
        InlineTime = ((double) timer.getMicroseconds() / 1000000.0);
        // -----------------------------------------------------
    }

    if (!mArchGraph->isGeneric()) {
        // Setting up timer ----------------
        timer.start();
        // ---------------------------------

        replaceWithArchSpecs();

        // Stopping timer and setting the stat -----------------
        timer.stop();
        ReplaceTime = ((double) timer.getMicroseconds() / 1000000.0);
        // -----------------------------------------------------
    }

    // Getting the new information, since it can be the case that the qmodule
    // was modified.
    auto depPass = PassCache::Get<DependencyBuilderWrapperPass>(mMod);
    auto depBuilder = depPass->getData();
    auto& deps = depBuilder.getDependencies();

    // Counting total dependencies.
    uint32_t totalDeps = 0;
    for (auto& d : deps)
        totalDeps += d.mDeps.size();
    DepStat = totalDeps;

    // Setting up timer ----------------
    timer.start();
    // ---------------------------------

    mData = executeAllocation(mMod);

    // Stopping timer and setting the stat -----------------
    timer.stop();
    AllocTime = ((double) timer.getMicroseconds() / 1000000.0);
    // -----------------------------------------------------

    TotalCost = mData.mCost;

    // Setting up timer ----------------
    timer.start();
    // ---------------------------------

    SolutionImplPass pass(mData);
    PassCache::Run(mMod, &pass);

    // Stopping timer and setting the stat -----------------
    timer.stop();
    RenameTime = ((double) timer.getMicroseconds() / 1000000.0);
    // -----------------------------------------------------

    return true;
}

void efd::QbitAllocator::setInlineAll(BasisVector basis) {
    mInlineAll = true;
    mBasis = basis;
}

void efd::QbitAllocator::setDontInline() {
    mInlineAll = false;
}

efd::QbitAllocator::Mapping efd::GenAssignment
(uint32_t archQ, QbitAllocator::Mapping mapping) {
    // 'archQ' is the number of qubits from the architecture.
    std::vector<uint32_t> assign(archQ, archQ);

    // for 'u' in arch; and 'a' in prog:
    // if 'a' -> 'u', then 'u' -> 'a'
    for (uint32_t i = 0, e = mapping.size(); i < e; ++i)
        assign[mapping[i]] = i;

    // Fill the qubits in the architecture that were not mapped.
    uint32_t id = mapping.size();
    for (uint32_t i = 0; i < archQ; ++i)
        assign[i] = (assign[i] == archQ) ? id++ : assign[i];

    return assign;
}
