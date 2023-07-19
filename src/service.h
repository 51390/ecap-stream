#ifndef _ECAP_STREAM_SERVICE_H
#define _ECAP_STREAM_SERVICE_H

#include <cstring>
#include <libecap/common/area.h>
#include <libecap/adapter/service.h>

#include "chunk.h"

namespace EcapStream {
    using libecap::size_type;

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

            void (*send_uri)(int, const char*);
            void (*header)(int, const char*, const char*);
            void (*send)(int, const void*, size_type);
            Chunk (*receive)(int, size_type, size_type);
            void (*done)(int);
            void (*cleanup)(int);
        private:
            void* _module;
            std::string _modulePath;
    };
}

#endif
