#include "SharedSocket.h"

struct SharedCtx
{
    SharedIO* io;
};

class SharedServer: public SharedSocket
{
  public:
    SharedServer()
    {
    }

    ~SharedServer()
    {
    }

    Listen()
    {
    }

    ProcessCommands()
    {
        

    }

    CharedCtx* createChannel()
    {
        SharedCtx ctx;
        _ctx_map.insert(ctx);
    }

  private:
    std::map<uint64_t,SharedCtx> _ctx_map;
};


