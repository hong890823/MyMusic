//
// Created by yangw on 2018-4-1.
//

#ifndef HMUSIC_PCMBEAN_H
#define HMUSIC_PCMBEAN_H

#include <SoundTouch.h>

using namespace soundtouch;

class HPcmBean {

public:
    char *buffer;
    int buffsize;

public:
    HPcmBean(SAMPLETYPE *buffer, int size);
    ~HPcmBean();


};


#endif //HMUSIC_PCMBEAN_H
