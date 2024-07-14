

using sigHandleType = void(*)(int);


// signal handler
void sigHandler(int sig);

// add signal to observe
void addSignal(int sig, sigHandleType handler, bool restart = true);

