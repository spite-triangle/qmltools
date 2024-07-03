#include "registerTask.h"


#include "handler/hoverTask.h"
#include "handler/documentTask.h"
#include "handler/initializeTask.h"
#include "handler/completionTask.h"
#include "handler/formatTask.h"
#include "handler/colorTask.h"

void RegisterTaskToServer(LspServer::Ptr &server)
{
    server->registoryTaskFactory("initialize", TaskFactoryTemplate<InitializeHandler>::makeFactory());
    server->registoryTaskFactory("initialized", TaskFactoryTemplate<InitializedHandler>::makeFactory());
    server->registoryTaskFactory("textDocument/hover", TaskFactoryTemplate<HoverTask>::makeFactory());
    server->registoryTaskFactory("textDocument/didSave", TaskFactoryTemplate<DocumentSavedTask>::makeFactory());
    server->registoryTaskFactory("textDocument/didOpen", TaskFactoryTemplate<DocumentOpenedTask>::makeFactory());
    server->registoryTaskFactory("textDocument/didClose", TaskFactoryTemplate<DocumentClosedTask>::makeFactory());
    server->registoryTaskFactory("textDocument/didChange", TaskFactoryTemplate<DocumentChangedTask>::makeFactory());
    server->registoryTaskFactory("textDocument/completion", TaskFactoryTemplate<CompletionTask>::makeFactory());
    server->registoryTaskFactory("textDocument/formatting", TaskFactoryTemplate<FormatTask>::makeFactory());
    server->registoryTaskFactory("textDocument/documentColor", TaskFactoryTemplate<DocumentColorTask>::makeFactory());
    server->registoryTaskFactory("textDocument/colorPresentation", TaskFactoryTemplate<ColorPresentationTask>::makeFactory());
    server->registoryTaskFactory("workspace/didCreateFiles", TaskFactoryTemplate<DocumentCreateTask>::makeFactory());
    server->registoryTaskFactory("workspace/didDeleteFiles", TaskFactoryTemplate<DocumentRemoveTask>::makeFactory());
    server->registoryTaskFactory("workspace/didRenameFiles", TaskFactoryTemplate<DocumentRenameTask>::makeFactory());
}


