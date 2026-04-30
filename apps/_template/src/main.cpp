#include "template_app.hpp"

// Keep main boring. The app class owns the app-specific state and hooks into
// EngineCore through AppCapabilities.
int main()
{
    TemplateApp app;
    return app.Run() ? 0 : 1;
}
