#include "gtest/gtest.h"
#include "dos/output.h"
#include "debug.h"

#include <string>

using namespace std;
DebugStream debug_stream;

int main(int argc, char* argv[]) {
    TRACE_ENABLE(false);
    setOutputLevel(LOG_ERROR);    
    for (int i = 0; i < argc; ++i) {
        const string arg{argv[i]};
        if (arg == "--debug") {
            TRACE_ENABLE(true);
            setOutputLevel(LOG_DEBUG);
        }
    }
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
