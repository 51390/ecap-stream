
#include <libecap/common/message.h>
#include <libecap/common/header.h>
#include <libecap/common/names.h>
#include <libecap/common/named_values.h>
#include <libecap/host/host.h>
#include <libecap/host/xaction.h>

#include "xaction.h"
#include "chunk.h"

libecap::Name EcapStream::Xaction::headerContentEncoding("Content-Encoding", libecap::Name::NextId());
int EcapStream::Xaction::counter = 0;

class HeaderVisitor: public libecap::NamedValueVisitor {
    public:
        HeaderVisitor(libecap::shared_ptr<const EcapStream::Service> aSvc, int anId, const char* anUri): svc(aSvc), id(anId), requestUri(anUri) {}
        virtual void visit(const libecap::Name &name, const libecap::Area &value) {
            svc->header(id, name.image().c_str(), value.start, requestUri);
        }

    private:
        libecap::shared_ptr<const EcapStream::Service> svc;
        int id;
        const char* requestUri;
};



EcapStream::Xaction::Xaction(libecap::shared_ptr<Service> aService,
    libecap::host::Xaction *x):
    service(aService),
    hostx(x) {
    id = counter++;
}

EcapStream::Xaction::~Xaction() {
    if (libecap::host::Xaction *x = hostx) {
        hostx = 0;
        x->adaptationAborted();
    }

    service->commit(id, "N/A", requestUri);
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
        return;
    }
    if (hostx->virgin().body()) {
        hostx->vbMake();
    }

    adapted = hostx->virgin().clone();

    if(!adapted) {
        return;
    }

    if(adapted->header().hasAny(libecap::headerContentLength)) {
        adapted->header().removeAny(libecap::headerContentLength);
    }

    if (!adapted->body()) {
        lastHostCall()->useAdapted(adapted);
    } else {
        hostx->useAdapted(adapted);
    }

}

void EcapStream::Xaction::stop() {
    hostx = 0;
    // the caller will delete
    service->commit(id, "N/A", requestUri);
    _cleanup();
}

void EcapStream::Xaction::_cleanup() {
    if(requestUri) {
        free(requestUri);
        requestUri = 0;
    }
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
    // FIXME: transfer only size bytes, and keep the rest in an intermediary buffer.
    Chunk c = service->get_content(id);

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
    service->content_done(id);
    hostx->noteAbContentDone(atEnd);
}

void EcapStream::Xaction::noteVbContentAvailable()
{

    if(!requestUri) {
        // grab the request uri
        const libecap::Message& cause = hostx->cause();
        const libecap::RequestLine& rl = dynamic_cast<const libecap::RequestLine&>(cause.firstLine());
        const libecap::Area uri = rl.uri();
        requestUri = (char*)malloc(uri.size + 1);
        memset(requestUri, 0, uri.size + 1);
        memcpy(requestUri, uri.start, uri.size);

        HeaderVisitor hv(service, id, requestUri);
        adapted->header().visitEach(hv);

    }

    const libecap::Area vb = hostx->vbContent(0, libecap::nsize); // get all vb

    hostx->vbContentShift(vb.size); // we have a copy; do not need vb any more

    if (vb.size && vb.start) {
        service->transfer(id, vb.start, vb.size, requestUri);
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


