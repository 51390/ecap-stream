#include <dlfcn.h>
#include <iostream>
#include <libecap/common/autoconf.h>
#include <libecap/common/area.h>
#include <libecap/common/errors.h>
#include <libecap/common/names.h>
#include <libecap/common/named_values.h>
#include <libecap/common/options.h>
#include <libecap/common/registry.h>

#include "service.h"
#include "xaction.h"

namespace EcapStream {
    static const std::string CfgErrorPrefix =
        "Modifying EcapStream: configuration error: ";
}

class Cfgtor: public libecap::NamedValueVisitor {
    public:
        Cfgtor(EcapStream::Service &aSvc): svc(aSvc) {}
        virtual void visit(const libecap::Name &name, const libecap::Area &value) {
            svc.setOne(name, value);
        }
        EcapStream::Service &svc;
};

EcapStream::Service::Service(): libecap::adapter::Service() {
}

std::string EcapStream::Service::uri() const {
    return "ecap://github.com/51390/ecap-stream";
}

std::string EcapStream::Service::tag() const {
    return PACKAGE_VERSION;
}

void EcapStream::Service::describe(std::ostream &os) const {
    os << "A modifying adapter from " << PACKAGE_NAME << " v" << PACKAGE_VERSION;
}

void EcapStream::Service::configure(const libecap::Options &cfg) {
    Cfgtor cfgtor(*this);
    cfg.visitEachOption(cfgtor);
}

void EcapStream::Service::reconfigure(const libecap::Options &cfg) {
    configure(cfg);
}

void EcapStream::Service::setOne(const libecap::Name &key, const libecap::Area &val) {
    const std::string value = val.toString();
    const std::string name = key.image();
    if (key.assignedHostId()) {
        // skip host-standard options we do not know or care about
    } else if(name == "analyzerPath") {
        analyzerPath = value;
    } else
        throw libecap::TextException(CfgErrorPrefix +
            "unsupported configuration parameter: " + name);
}

void EcapStream::Service::start() {
    libecap::adapter::Service::start();

    std::clog << "Prism starting" << std::endl;

    module_ = dlopen(analyzerPath.c_str(), RTLD_NOW | RTLD_GLOBAL);

    if(module_) {
        init = (void (*)())dlsym(module_, "init");
        init();

        transfer = (void (*)(int, const void*, size_t, const char*))dlsym(module_, "transfer");
        commit = (void (*)(int, const char*, const char*))dlsym(module_, "commit");
        header = (void (*)(int, const char*, const char*, const char*))dlsym(module_, "header");
        get_content = (Chunk (*)(int))dlsym(module_, "get_content");
        content_done = (void (*)(int))dlsym(module_, "content_done");
    }

    std::clog << "Prism init OK" << std::endl;
}

void EcapStream::Service::stop() {
    libecap::adapter::Service::stop();
}

void EcapStream::Service::retire() {
    libecap::adapter::Service::stop();
}

bool EcapStream::Service::wantsUrl(const char *url) const {
    return true; // no-op is applied to all messages
}

EcapStream::Service::MadeXactionPointer
EcapStream::Service::makeXaction(libecap::host::Xaction *hostx) {
    return EcapStream::Service::MadeXactionPointer(
        new EcapStream::Xaction(std::tr1::static_pointer_cast<Service>(self), hostx));
}

// create the adapter and register with libecap to reach the host application
static const bool Registered =
    libecap::RegisterVersionedService(new EcapStream::Service);
