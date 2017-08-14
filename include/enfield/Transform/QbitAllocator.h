#ifndef __EFD_QBIT_ALLOCATOR_H__
#define __EFD_QBIT_ALLOCATOR_H__

#include "enfield/Arch/ArchGraph.h"
#include "enfield/Transform/DependencyBuilderPass.h"
#include "enfield/Support/CommandLine.h"
#include "enfield/Support/Stats.h"

namespace efd {
    /// \brief Base abstract class that allocates the qbits used in the program to
    /// the qbits that are in the physical architecture.
    class QbitAllocator : PassT<std::vector<unsigned>> {
        public:
            typedef QbitAllocator* Ref;
            typedef std::unique_ptr<QbitAllocator> uRef;

            typedef std::vector<unsigned> Mapping;
            typedef std::vector<std::string> BasisVector;
            typedef DependencyBuilder::DepsSet DepsSet;
            typedef DependencyBuilder::DepsSet::iterator Iterator;

        protected:
            DependencyBuilder mDepBuilder;
            QbitToNumber mQbitToNumber;

            bool mInlineAll;

            ArchGraph::sRef mArchGraph;
            BasisVector mBasis;

            QModule::Ref mMod;

            /// \brief Updates the mDepSet attribute. Generally it is done after
            /// running the DependencyBuilderPass.
            void updateDependencies();

            QbitAllocator(ArchGraph::sRef archGraph);

            /// \brief Inlines the gate call that generates the dependencies that are
            /// referenced by \p it. If the node is not an NDQOpGeneric, it does nothing.
            Iterator inlineDep(Iterator it);

            /// \brief Inlines all gates, but those flagged.
            void inlineAllGates();

            /// \brief Replace all qbits from the program with the architecture's qbits. 
            void replaceWithArchSpecs();

            /// \brief Rename all the qbits, taking into account the \em mMapping, and
            /// the swaps generated by the compiler.
            void renameQbits();

        public:
            void run(QModule::Ref qmod) override;

            /// \brief Inserts a swap between u and v. (note that these indexes must be
            /// the indexes of the program's qbit)
            void insertSwapBefore(Dependencies& deps, unsigned u, unsigned v);

            /// \brief Inserts a CNOT operation between two qubits that have two edges
            /// between them.
            void replaceByLCNOT(Dependencies& deps, unsigned u, unsigned w, unsigned v);

            /// \brief Returns the number of qbits in the program.
            unsigned getNumQbits();

            /// \brief Generates an assignment mapping (maps the architecture's qubits
            /// to the logical ones).
            Mapping genAssign(Mapping mapping);

            /// \brief Flags the QbitAllocator to inline all gates, but those inside the
            /// \p basis vector, before mapping.
            void setInlineAll(BasisVector basis = {});
            /// \brief Flags the QbitAllocator not to inline.
            void setDontInline();

            /// \brief Generates the final mapping of the program, inserting the swaps where
            /// needed.
            virtual Mapping solveDependencies(DepsSet& deps) = 0;
    };
}

extern efd::Stat<unsigned> TotalCost;
extern efd::Opt<unsigned> SwapCost;
extern efd::Opt<unsigned> RevCost;
extern efd::Opt<unsigned> LCXCost;

#endif
