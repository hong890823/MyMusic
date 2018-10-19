//
// Created by Hong on 2018/10/16.
//

#ifndef MYMUSIC_HPLAYSTATUS_H
#define MYMUSIC_HPLAYSTATUS_H


class HPlayStatus {
public:
    bool exit;
    bool load = true;
    bool seek = false;
public:
    HPlayStatus();
    ~HPlayStatus();
};


#endif //MYMUSIC_HPLAYSTATUS_H
