#ifndef _ECAP_STREAM_CHUNK_H
#define _ECAP_STREAM_CHUNK_H

#include <cstdlib>

namespace EcapStream {

    typedef struct {
        size_t size;
        const void* bytes;
    } Chunk;
}

#endif
