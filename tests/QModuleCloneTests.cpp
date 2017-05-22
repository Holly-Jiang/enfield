#include "gtest/gtest.h"

#include "enfield/Pass.h"
#include "enfield/Analysis/Nodes.h"
#include "enfield/Analysis/QModule.h"
#include "enfield/Support/RTTI.h"

#include <string>

using namespace efd;

namespace {
    class ASTVectorPass : public Pass {
        public:
            std::vector<NodeRef> mV;

            ASTVectorPass() {
                mUK = Pass::K_AST_PASS;
            }

            void init() override {
                mV.clear();
            }

            void visitNode(NodeRef ref) {
                mV.push_back(ref);
                for (auto child : *ref)
                    child->apply(this);
            }

            void visit(NDQasmVersion* ref) override { visitNode(ref); }
            void visit(NDInclude* ref) override { visitNode(ref); }
            void visit(NDDecl* ref) override { visitNode(ref); }
            void visit(NDGateDecl* ref) override { visitNode(ref); }
            void visit(NDOpaque* ref) override { visitNode(ref); }
            void visit(NDQOpMeasure* ref) override { visitNode(ref); }
            void visit(NDQOpReset* ref) override { visitNode(ref); }
            void visit(NDQOpU* ref) override { visitNode(ref); }
            void visit(NDQOpCX* ref) override { visitNode(ref); }
            void visit(NDQOpBarrier* ref) override { visitNode(ref); }
            void visit(NDQOpGeneric* ref) override { visitNode(ref); }
            void visit(NDBinOp* ref) override { visitNode(ref); }
            void visit(NDUnaryOp* ref) override { visitNode(ref); }
            void visit(NDIdRef* ref) override { visitNode(ref); }
            void visit(NDList* ref) override { visitNode(ref); }
            void visit(NDStmtList* ref) override { visitNode(ref); }
            void visit(NDGOpList* ref) override { visitNode(ref); }
            void visit(NDIfStmt* ref) override { visitNode(ref); }
            void visit(NDValue<std::string>* ref) override { visitNode(ref); }
            void visit(NDValue<IntVal>* ref) override { visitNode(ref); }
            void visit(NDValue<RealVal>* ref) override { visitNode(ref); }
    };
}

void compareClonedPrograms(const std::string program) {
    std::unique_ptr<QModule> qmod = QModule::ParseString(program);
    std::unique_ptr<QModule> clone = qmod->clone();

    ASTVectorPass vQMod, vClone;

    qmod->runPass(&vQMod);
    clone->runPass(&vClone);

    ASSERT_EQ(qmod->toString(), clone->toString());
    ASSERT_EQ(vQMod.mV.size(), vClone.mV.size());

    for (unsigned i = 0, e = vQMod.mV.size(); i < e; ++i)
        ASSERT_FALSE(vQMod.mV[i] == vClone.mV[i]);
}

#define TEST_CLONE(TestName, Program) \
    TEST(NodeCloneTests, TestName) { \
        compareClonedPrograms(Program); \
    }

TEST_CLONE(QASMVersionTest, "OPENQASM 2.0;");
TEST_CLONE(IncludeTest,"include \"files/qelib1.inc\";");
TEST_CLONE(DeclTest, "qreg q0[10];qreg q1[10];creg c0[10];");
TEST_CLONE(GateTest, "gate notid a {}");
TEST_CLONE(OpaqueGateTest, "opaque ogate(x, y) a, b, c;");
TEST_CLONE(MeasureTest, "measure q[0] -> c[0];");
TEST_CLONE(ResetTest, "reset q0[0];");
TEST_CLONE(BarrierTest, "barrier q0, q1;");
TEST_CLONE(GenericTest, "notid(pi + 3 / 8) q0[0], q[1];");
TEST_CLONE(CXTest, "CX q0[0], q[1];");
TEST_CLONE(UTest, "U q0[0];");
TEST_CLONE(GOPListTest, "gate notid a, b { CX a, b; U(pi) a; }");

TEST_CLONE(WholeProgramTest, 
"\
OPENQASM 2.0;\
include \"files/qelib1.inc\";\
qreg q0[10];\
qreg q1[10];\
creg c0[10];\
gate notid(cc) a, b {\
    CX a, b;\
    U(cc) a;\
}\
opaque ogate(x, y) a, b, c;\
measure q[0] -> c[0];\
reset q0[0];\
barrier q0, q1;\
notid(pi + 3 / 8) q0[0], q[1];\
");