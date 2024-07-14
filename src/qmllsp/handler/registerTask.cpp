#include "registerTask.h"


#include "handler/exitTask.h"
#include "handler/colorTask.h"
#include "handler/hoverTask.h"
#include "handler/formatTask.h"
#include "handler/renameTask.h"
#include "handler/documentTask.h"
#include "handler/tokenFindTask.h"
#include "handler/initializeTask.h"
#include "handler/completionTask.h"
#include "handler/configTask.h"

void RegisterTaskToServer(LspServer::Ptr &server)
{
    server->registoryTaskFactory("initialize", TaskFactoryTemplate<InitializeHandler>::makeFactory());
    server->registoryTaskFactory("initialized", TaskFactoryTemplate<InitializedHandler>::makeFactory());
    server->registoryTaskFactory("shutdown", TaskFactoryTemplate<ShutdownTask>::makeFactory());
    server->registoryTaskFactory("exit", TaskFactoryTemplate<ExitTask>::makeFactory());
    server->registoryTaskFactory("workspace/didChangeConfiguration", TaskFactoryTemplate<ConfigTask>::makeFactory());
    server->registoryTaskFactory("textDocument/hover", TaskFactoryTemplate<HoverTask>::makeFactory());
    server->registoryTaskFactory("textDocument/didSave", TaskFactoryTemplate<DocumentSavedTask>::makeFactory());
    server->registoryTaskFactory("textDocument/didOpen", TaskFactoryTemplate<DocumentOpenedTask>::makeFactory());
    server->registoryTaskFactory("textDocument/didClose", TaskFactoryTemplate<DocumentClosedTask>::makeFactory());
    server->registoryTaskFactory("textDocument/didChange", TaskFactoryTemplate<DocumentChangedTask>::makeFactory());
    server->registoryTaskFactory("textDocument/completion", TaskFactoryTemplate<CompletionTask>::makeFactory());
    server->registoryTaskFactory("textDocument/formatting", TaskFactoryTemplate<FormatTask>::makeFactory());
    server->registoryTaskFactory("textDocument/documentColor", TaskFactoryTemplate<DocumentColorTask>::makeFactory());
    server->registoryTaskFactory("textDocument/colorPresentation", TaskFactoryTemplate<ColorPresentationTask>::makeFactory());
    server->registoryTaskFactory("textDocument/rename", TaskFactoryTemplate<RenameTask>::makeFactory());
    server->registoryTaskFactory("textDocument/references", TaskFactoryTemplate<ReferencesFindTask>::makeFactory());
    server->registoryTaskFactory("textDocument/definition", TaskFactoryTemplate<DefineTask>::makeFactory());
    server->registoryTaskFactory("workspace/didCreateFiles", TaskFactoryTemplate<DocumentCreateTask>::makeFactory());
    server->registoryTaskFactory("workspace/didDeleteFiles", TaskFactoryTemplate<DocumentRemoveTask>::makeFactory());
    server->registoryTaskFactory("workspace/didRenameFiles", TaskFactoryTemplate<DocumentRenameTask>::makeFactory());
}


