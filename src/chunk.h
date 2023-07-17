#ifndef _CHUNK_H
#define _CHUNK_H

#include <cstdlib>

namespace Adapter {

    typedef struct {
        size_t size;
        const void* bytes;
    } Chunk;

}

#endif
