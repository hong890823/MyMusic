//
// Created by yangw on 2018-4-1.
//

#include "HPcmBean.h"

HPcmBean::HPcmBean(SAMPLETYPE *buffer, int size) {

    this->buffer = (char *) malloc(static_cast<size_t>(size));
    this->buffsize = size;
    memcpy(this->buffer, buffer, static_cast<size_t>(size));

}

HPcmBean::~HPcmBean() {
    free(buffer);
    buffer = NULL;
}
