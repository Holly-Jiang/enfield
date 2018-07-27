#include "gtest/gtest.h"

#include "enfield/Analysis/Driver.h"
#include "enfield/Analysis/NodeVisitor.h"
#include "enfield/Transform/QModule.h"
#include "enfield/Transform/XbitToNumberPass.h"
#include "enfield/Transform/PassCache.h"
#include "enfield/Support/RTTI.h"
#include "enfield/Support/Defs.h"
#include "enfield/Support/uRefCast.h"

#include <string>
#include <unordered_map>

using namespace efd;

TEST(XbitToNumberWrapperPassTests, WholeProgramTest) {
    {
        const std::string program = \
"\
qreg q[5];\
";

        auto qmod = toShared(QModule::ParseString(program));
        auto pass = XbitToNumberWrapperPass::Create();
        pass->run(qmod.get());

        auto data = pass->getData();
        ASSERT_EXIT({ data.getQUId("q"); },
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)), "");
        ASSERT_TRUE(data.getQUId("q[0]") == 0);
        ASSERT_TRUE(data.getQUId("q[1]") == 1);
        ASSERT_TRUE(data.getQUId("q[2]") == 2);
        ASSERT_TRUE(data.getQUId("q[3]") == 3);
        ASSERT_TRUE(data.getQUId("q[4]") == 4);

        PassCache::Clear();
    }

    {
        const std::string program = \
"\
gate mygate(a, b, c) x, y, z {\
    cx x, y; cx y, z;\
}\
";

        auto qmod = toShared(QModule::ParseString(program));
        auto pass = XbitToNumberWrapperPass::Create();
        pass->run(qmod.get());

        auto sign = qmod->getQGate("mygate");
        ASSERT_FALSE(sign == nullptr);

        auto data = pass->getData();
        ASSERT_EXIT({ data.getQUId("mygate"); },
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("x"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("y"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("z"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");

        NDGateDecl::Ref gate = dynCast<NDGateDecl>(sign);
        ASSERT_FALSE(gate == nullptr);
        ASSERT_TRUE(data.getQUId("x", gate) == 0);
        ASSERT_TRUE(data.getQUId("y", gate) == 1);
        ASSERT_TRUE(data.getQUId("z", gate) == 2);

        PassCache::Clear();
    }

    {
        const std::string program = \
"\
gate id a {\
}\
gate cnot a, b {\
    cx a, b;\
}\
qreg q[5];\
creg c[5];\
id q[0];\
id q[1];\
cnot q[0], q;\
measure q[0] -> c[0];\
measure q[1] -> c[1];\
measure q[2] -> c[2];\
measure q[3] -> c[3];\
measure q[4] -> c[4];\
";

        auto qmod = toShared(QModule::ParseString(program));
        auto pass = XbitToNumberWrapperPass::Create();
        pass->run(qmod.get());

        auto data = pass->getData();

        auto idSign = qmod->getQGate("id");
        ASSERT_FALSE(idSign == nullptr);
        NDGateDecl::Ref idGate = dynCast<NDGateDecl>(idSign);
        ASSERT_FALSE(idGate == nullptr);

        auto cnotSign = qmod->getQGate("cnot");
        ASSERT_FALSE(cnotSign == nullptr);
        NDGateDecl::Ref cnotGate = dynCast<NDGateDecl>(cnotSign);
        ASSERT_FALSE(cnotGate == nullptr);

        ASSERT_EXIT({ data.getQUId("id"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("cnot"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("a"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("b"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("q"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("c"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("c[0]"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("c[1]"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("c[2]"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("c[3]"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");
        ASSERT_EXIT({ data.getQUId("c[4]"); }, 
                    ::testing::ExitedWithCode(static_cast<uint32_t>
                        (ExitCode::EXIT_unknown_resource)),
                    "Qubit id not found");

        ASSERT_TRUE(data.getQUId("a", idGate) == 0);
        ASSERT_TRUE(data.getQUId("a", cnotGate) == 0);
        ASSERT_TRUE(data.getQUId("b", cnotGate) == 1);
        ASSERT_TRUE(data.getQUId("q[0]") == 0);
        ASSERT_TRUE(data.getQUId("q[1]") == 1);
        ASSERT_TRUE(data.getQUId("q[2]") == 2);
        ASSERT_TRUE(data.getQUId("q[3]") == 3);
        ASSERT_TRUE(data.getQUId("q[4]") == 4);

        PassCache::Clear();
    }
}
