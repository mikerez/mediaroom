#include "SharedIO.h"

// basic methods for IO allocation when control socket is connected

enum ss_proto
{
    SS_INIT,
    SS_ACK,
    SS_NACK,
    SS_NEWRCHAN,
    SS_NEWWCHAN,
    SS_CLOSECHAN,
};

struct ss_cmd
{
    uint64_t cmd;
    uint64_t id;
    char text[32];
};

class SharedSocket
{
public:
    SharedSocket()
    {
    }
    
    virtual ~SharedSocket()
    {
    }


    SharedIO* newRChan(char *name_prefix)
    {
        
    }

    SharedIO* newWChan()
    {
    }

    void closeChan

private:
    int sock;

};


