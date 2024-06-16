#include "registerTask.h"


#include "handler/hoverTask.h"
#include "handler/documentTask.h"
#include "handler/initializeTask.h"

void RegisterTaskToServer(LspServer::Ptr &server)
{
    server->registoryTaskFactory("initialize", TaskFactoryTemplate<InitializeHandler>::makeFactory());
    server->registoryTaskFactory("initialized", TaskFactoryTemplate<InitializedHandler>::makeFactory());
    server->registoryTaskFactory("textDocument/hover", TaskFactoryTemplate<HoverTask>::makeFactory());
    server->registoryTaskFactory("textDocument/didSave", TaskFactoryTemplate<DocumentSavedTask>::makeFactory());
}

