#include "MixerApplication.h"
#include <iostream>
#include <csignal>

void signalHandler(int signal)
{
    exit(signal);
}

int runApplication()
{
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    MixerApplication app;

    if (!app.initialize())
    {
        std::cerr << "[MixerPanel] Failed to initialize application" << std::endl;
        return -1;
    }

    int exitCode = app.run();
    
    return exitCode;
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return runApplication();
}
#else
int main(int, char**)
{
    return runApplication();
}
#endif
