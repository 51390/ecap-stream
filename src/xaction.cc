#include <iostream>
#include <libecap/common/message.h>
#include <libecap/common/header.h>
#include <libecap/common/names.h>
#include <libecap/common/named_values.h>
#include <libecap/host/host.h>
#include <libecap/host/xaction.h>

#include "xaction.h"
#include "chunk.h"

int EcapStream::Xaction::_counter = 0;

class HeaderVisitor: public libecap::NamedValueVisitor {
    public:
        HeaderVisitor(libecap::shared_ptr<const EcapStream::Service> aSvc, int anId): svc(aSvc), _id(anId) {}
        virtual void visit(const libecap::Name &name, const libecap::Area &value) {
            svc->header(_id, name.image().c_str(), value.start);
        }

    private:
        libecap::shared_ptr<const EcapStream::Service> svc;
        int _id;
        const char* _uri;
};



EcapStream::Xaction::Xaction(libecap::shared_ptr<Service> aService,
    libecap::host::Xaction *x):
    service(aService),
    hostx(x),
    _uri(0) {
    _id = _counter++;
}

EcapStream::Xaction::~Xaction() {
    if (libecap::host::Xaction *x = hostx) {
        hostx = 0;
        x->adaptationAborted();
    }

    _cleanup();
}

const libecap::Area EcapStream::Xaction::option(const libecap::Name &) const {
    return libecap::Area(); // this transaction has no meta-information
}

void EcapStream::Xaction::visitEachOption(libecap::NamedValueVisitor &) const {
    // this transaction has no meta-information to pass to the visitor
}


void EcapStream::Xaction::start() {
    if(!hostx) {
        std::clog << "No hostx, aborting transaction no. " << _id << std::endl;
        return;
    }

    if (hostx->virgin().body()) {
        hostx->vbMake();
    }

    if(!_uri) {
        try {
            const libecap::Message& cause = \
                service->mode() == EcapStream::Service::RESPMOD ? \
                hostx->cause() : \
                hostx->virgin();
            const libecap::RequestLine& rl = dynamic_cast<const libecap::RequestLine&>(cause.firstLine());
            const libecap::Area uri = rl.uri();
            _uri = (char*)malloc(uri.size + 1);
            memset(_uri, 0, uri.size + 1);
            memcpy(_uri, uri.start, uri.size);
            service->send_uri(_id, _uri, service->mode());
        } catch(const std::exception& e) {
            std::clog << "Transaction failed to start while initializing: " << e.what() << std::endl;
        }
    }

    adapted = hostx->virgin().clone();

    if(!adapted) {
        return;
    }

    if(adapted->header().hasAny(libecap::headerContentLength)) {
        adapted->header().removeAny(libecap::headerContentLength);
    }

    HeaderVisitor hv(service, _id);
    adapted->header().visitEach(hv);

    if (!adapted->body()) {
        lastHostCall()->useAdapted(adapted);
    } else {
        hostx->useAdapted(adapted);
    }
}

void EcapStream::Xaction::stop() {
    hostx = 0;
    // the caller will delete
    _cleanup();
}

void EcapStream::Xaction::_cleanup() {
    if(_uri) {
        free(_uri);
        _uri = 0;
    }
    service->cleanup(_id);
}

void EcapStream::Xaction::abDiscard()
{
    stopVb();
}

void EcapStream::Xaction::abMake()
{
    hostx->noteAbContentAvailable();
}

void EcapStream::Xaction::abMakeMore()
{
    hostx->vbMakeMore();
}

void EcapStream::Xaction::abStopMaking()
{
    stopVb();
}

libecap::Area EcapStream::Xaction::abContent(size_type offset, size_type size) {
    Chunk c = service->receive(_id, offset, size);

    if(c.size) {
        return libecap::Area::FromTempBuffer((const char*)c.bytes, c.size);
    } else {
        return libecap::Area();
    }
}

void EcapStream::Xaction::abContentShift(size_type size) {
}

void EcapStream::Xaction::noteVbContentDone(bool atEnd)
{
    stopVb();
    service->done(_id);
    hostx->noteAbContentDone(atEnd);
}

void EcapStream::Xaction::noteVbContentAvailable()
{
    if(!_uri) {
        // send uri and headers
        HeaderVisitor hv(service, _id);
        const libecap::Message& cause = hostx->cause();
        const libecap::RequestLine& rl = dynamic_cast<const libecap::RequestLine&>(cause.firstLine());
        const libecap::Area uri = rl.uri();
        _uri = (char*)malloc(uri.size + 1);
        memset(_uri, 0, uri.size + 1);
        memcpy(_uri, uri.start, uri.size);
        service->send_uri(_id, _uri, service->mode());

        adapted->header().visitEach(hv);
    }

    const libecap::Area vb = hostx->vbContent(0, libecap::nsize); // get all vb

    hostx->vbContentShift(vb.size); // we have a copy; do not need vb any more

    if (vb.size && vb.start) {
        service->send(_id, vb.start, vb.size);
        hostx->noteAbContentAvailable();
    }
}

// tells the host that we are not interested in [more] vb
// if the host does not know that already
void EcapStream::Xaction::stopVb() {
    hostx->vbStopMaking(); // we will not call vbContent() any more
}

// this method is used to make the last call to hostx transaction
// last call may delete adapter transaction if the host no longer needs it
// TODO: replace with hostx-independent "done" method
libecap::host::Xaction *EcapStream::Xaction::lastHostCall() {
    libecap::host::Xaction *x = hostx;
    hostx = 0;
    return x;
}


