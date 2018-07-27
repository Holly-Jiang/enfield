#include "enfield/Transform/LayerBasedOrderingWrapperPass.h"
#include "enfield/Transform/PassCache.h"
#include <cassert>

uint32_t efd::LayerBasedOrderingWrapperPass::getNodeId(Node::Ref ref) {
    if (mStmtId.find(ref) != mStmtId.end()) {
        ERR << "Unknown node: `" << (ref == nullptr) ? "nullptr" : ref->toString(false)
            << "`." << std::endl;
        ExitWith(ExitCode::EXIT_unreachable);
    }

    return mStmtId[ref];
}

bool efd::LayerBasedOrderingWrapperPass::run(QModule* qmod) {
    mStmtId.clear();

    for (auto it = qmod->stmt_begin(), end = qmod->stmt_end(); it != end; ++it) {
        mStmtId.insert(std::make_pair(it->get(), mStmtId.size()));
    }

    auto cgbpass = PassCache::Get<CircuitGraphBuilderPass>(qmod);
    mData.ordering = generate(cgbpass->getData());
    qmod->orderby(mData.ordering);

    return true;
}
