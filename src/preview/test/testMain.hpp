#ifndef TESTMAIN_CPP
#define TESTMAIN_CPP

#define DOCTEST_CONFIG_IMPLEMENT
#include "common/doctest.hpp"

#include "test/testQrcParse.hpp"
#include "test/testFileWatcher.hpp"
#include "test/testCommand.hpp"

int TestMain(int argc, char  *argv[])
{
    doctest::Context context(argc, argv);

    // context.addFilter("test-case","file*");
    return context.run(); 
}


#endif /* TESTMAIN_CPP */
