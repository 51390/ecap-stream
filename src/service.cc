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
    } else if(name == "modulePath") {
        _modulePath = value;
    } else {
        throw libecap::TextException(CfgErrorPrefix +
                "unsupported configuration parameter: " + name);
    }
}

void EcapStream::Service::start() {
    libecap::adapter::Service::start();

    std::clog << "Ecap-Stream started." << std::endl;

    _module = dlopen(_modulePath.c_str(), RTLD_NOW | RTLD_GLOBAL);

    if(_module) {
        void (*init)();
        init = (void (*)())dlsym(_module, "init");
        init();

        send_uri = (void (*)(int, const char*))dlsym(_module, "uri");
        header = (void (*)(int, const char*, const char*))dlsym(_module, "header");
        // note on function and variable naming.
        // send & receive are "mirrored" to keep a more ergonomic
        // naming from the perspective of the calling / defining module:
        // ecap-stream will send data by calling "send" but this call will then
        // translate in the loaded module into a "receive" function, because,
        // from its perspective, it is the one receiving data.
        // And similarly or the receive/send exchange.
        send = (void (*)(int, const void*, size_type))dlsym(_module, "receive");
        receive = (Chunk (*)(int, size_type, size_type))dlsym(_module, "send");
        done = (void (*)(int))dlsym(_module, "done");
        std::clog << "Ecap-Stream started." << std::endl;
    } else {
        std::clog << "Ecap-Stream failed starting. Check the 'modulePath' configuration." << std::endl;
    }
}

void EcapStream::Service::stop() {
    libecap::adapter::Service::stop();
}

void EcapStream::Service::retire() {
    libecap::adapter::Service::stop();

    if(_module) {
        dlclose(_module);
    }
    header = 0;
    send = 0;
    receive = 0;
    done = 0;
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
