#include "registerTask.h"


#include "handler/initializeTask.h"

void RegisterTaskToServer(LspServere &server)
{
    server.registoryTaskFactory("initialize", TaskFactoryTemplate<InitializeHandler>::makeFactory());
}