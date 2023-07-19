#ifndef _ECAP_STREAM_XACTION_H
#define _ECAP_STREAM_XACTION_H

#include <libecap/common/names.h>
#include <libecap/adapter/xaction.h>
#include "service.h"

namespace EcapStream {
    using libecap::size_type;

    class Xaction: public libecap::adapter::Xaction {
        public:
            Xaction(libecap::shared_ptr<Service> s, libecap::host::Xaction *x);
            virtual ~Xaction();

            // meta-information for the host transaction
            virtual const libecap::Area option(const libecap::Name &name) const;
            virtual void visitEachOption(libecap::NamedValueVisitor &visitor) const;

            // lifecycle
            virtual void start();
            virtual void stop();

            // adapted body transmission control
            virtual void abDiscard();
            virtual void abMake();
            virtual void abMakeMore();
            virtual void abStopMaking();

            // adapted body content extraction and consumption
            virtual libecap::Area abContent(size_type offset, size_type size);
            virtual void abContentShift(size_type size);

            // virgin body state notification
            virtual void noteVbContentDone(bool atEnd);
            virtual void noteVbContentAvailable();

        protected:
            void stopVb();
            libecap::host::Xaction *lastHostCall();

        private:
            void _processBuffers();
            void _cleanup();

            libecap::shared_ptr<const Service> service;
            libecap::shared_ptr<libecap::Message>  adapted;
            libecap::host::Xaction *hostx;

            int _id = 0;
            char* _uri = 0;

            static int _counter;
    };


}

#endif
