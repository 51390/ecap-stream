#ifndef _ECAP_STREAM_SERVICE_H
#define _ECAP_STREAM_SERVICE_H

#include <cstring>
#include <libecap/adapter/service.h>

#include "chunk.h"

namespace EcapStream {

    class Service: public libecap::adapter::Service {
        public:
            Service();

            // About
            virtual std::string uri() const; // unique across all vendors
            virtual std::string tag() const; // changes with version and config
            virtual void describe(std::ostream &os) const; // free-format info

            // Configuration
            virtual void configure(const libecap::Options &cfg);
            virtual void reconfigure(const libecap::Options &cfg);
            void setOne(const libecap::Name &name, const libecap::Area &valArea);

            // Lifecycle
            virtual void start(); // expect makeXaction() calls
            virtual void stop(); // no more makeXaction() calls until start()
            virtual void retire(); // no more makeXaction() calls

            // Scope (XXX: this may be changed to look at the whole header)
            virtual bool wantsUrl(const char *url) const;

            // Work
            virtual MadeXactionPointer makeXaction(libecap::host::Xaction *hostx);

            void (*init)();
            void (*transfer)(int, const void*, size_t, const char*);
            void (*commit)(int, const char*, const char*);
            void (*header)(int, const char*, const char*, const char*);
            void (*content_done)(int);
            Chunk (*get_content)(int);
        private:
            void * module_;
            std::string analyzerPath;
    };


}

#endif
