#include "common/previewProject.h"
#include "previewProject.h"

#include "common/argparse.hpp"

void ProjectExplorer::Project::appendQrcFile(const QString &strPath)
{
    m_setting.setQrcFile.insert(strPath);
}
void ProjectExplorer::Project::appendExtendSearchFolder(const QString &strPath)
{
    m_setting.setExtendSearchFolder.insert(strPath);
}

bool ProjectExplorer::Project::parserCommand(int argc, char *argv[])
{
    // auto args = util::argparser("A quantum physics calculation program.");
    // args.set_program_name("test")
    //     .add_help_option()
    //     .add_sc_option("-v", "--version", "show version info", []() {
    //         std::cout << "version " << VERSION << std::endl;
    //     })
    //     .add_option("-o", "--openmp", "use openmp or not")
    //     .add_option("-m", "--mpi", "use mpi or not")
    //     .add_option<int>("-t", "--threads", "if openmp it set,\nuse how many threads,\ndefault is 4", 4)
    //     .add_option<util::StepRange>("-r", "--range", "range", util::range(0, 10, 2))
    //     .add_named_argument<std::string>("input", "initialize file")
    //     .add_named_argument<std::string>("output", "output file")
    //     .parse(argc, argv);
    return false;
}
