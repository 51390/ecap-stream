# eCap Stream

![Tests](https://github.com/51390/ecap-stream/actions/workflows/test.yml/badge.svg)

This is an [eCap adapter](https://www.e-cap.org/) that receives body data from the
host and forwards it to an external library, to be then recorded, adapted or
filtered at will. The processed -- or unprocessed -- data is then sent back to the host.

The intent of this library is to provide a simpler interface than fully implementing
the eCap protocol, and by that also supporting FFI implementations.

## Building

```
$ docker build -t ecap-stream .
```

## Testing

The default entrypoint runs a simple test case, so all that is needed is to execute the
docker image.

```
$ docker run -it ecap-stream
```

## Using

Ecap-stream works by loading a configurable shared object library
that conforms to its api, and delegates calls to it as some particular
events take place in the host and are sent down to ecap-stream via the
ecap interface.

These events are:
 - _new transaction is initialized_: in the context of a web or proxy server,
 these could be requests and responses.
 - _new data is available_: again in the web/proxy context, these will translate
 into request or response body data.
 - _host requests data_: the host is requesting a block of data.
 - _no more data is available from the host_: this would mean that all of the
 request/response data has been transferred in a web/proxy context.
 - _transaction is finished_.

The docker image built can be used as the base image for a Dockerfile used in another project.

```
FROM ecap-stream

...
```

This will make both the libecap and libecap-stream dynamic shard objects available for linking.

The next steps are:
  1. implement the external client module that will be loaded and receive the API calls
with the host data to be then analyzed, adapted, or filtered, depending on the module's 
implementation requirements. 
  2. this module must be packaged in another shared object and also made available for linking.
  3. configure the host to load both the ecap-stream shared object, and pass along this configuration the
  required path for the module that will in turn be loaded by eCap stream to fully implement your
  adapter/filter.

The following section details which endpoints need to be implemented, and their purpose, and
the final one will provide an example configuration for loading ecap-stream in the squid proxy.

## API

The following functions need to be implemented in the client module that will be loaded by
libecap-stream, to complete an end-to-end implementation of the client module.

### _void_ init()

This function should perform any module wide initialization required, such as specific logging
or runtime setups that may be needed. It is called once per eCap-service initialization, which
usually translates into a single call per host lifecycle.

### _void_ uri(_int_ id, _const char*_ uri)

This function is the first called for each transaction. In the context of web requests or responses,
note that each flow of information is separately handled by different transactions. So, there could be
an initial transaction for handling the request and a matching subsequent transaction for handling the
respective response.

_Parameters_:
 - **id** _(int)_: this is an integer that represents the transaction identifier. It will be passed down
 throughout all other interactions between eCap stream and the client module.
 - **uri** _(const char*)_: a C null-terminated string, containing the full uri of the request (inluding
 query string parameters).

### _void_ header(_int_ id, _const char*_ name, _const char*_ value)

This function will notify about every request or response header that is seen in the transaction.
There will be one call per header.

_Parameters_:
  - **id** _(int)_: the transaction id to which the header belongs to.
  - **name** _(const char*)_: a null-terminated string representing the header name.
  - **value** _(const char*)_: a null-terminated string representing the header value.

### _void_ receive(_int_ id, _const void*_ data, _libecap::size_type_ size)

This function notifies the client module of raw data available from the host, which usually is a part
of a request or response body. None, a single, or multiple chunks may be sent, and the only guarantees made
are on the order and completeness, meaning that chunks are sent ordered and all data in the body will
be eventually sent down as chunks, if any data exists -- empty bodies may result in no calls being made.

_Parameters_:
  - **id** _(int)_: the transaction id to which the chunk of data belongs to.
  - **data** _(const void*)_: a pointer to an array of bytes that contains the available data from the host.
  - **size** _(libecap::size_type)_: the length of data available.

### _[Chunk](https://github.com/51390/ecap-stream/blob/main/src/chunk.h)_ send(_int_ id, _libecap::size_type_ offset, _libecap::size_type_ size)

This function is called when the host expects to receive data. The auxiliar parameters
`offset` and `size` can be used to determine where the data transmission should start or
resume, and how many bytes are expected to be sent back to the host.

_Parameters:_
  - **id** _(int)_: the transaction id to which the chunk of data belongs to.
  - **offset** _(libecap::size_type)_: the data offset of the data chunk to send back to the host.
  - **size** _(libecap::size_type)_: the number of bytes of data to send back to the host.

_Return value:_
  - _Chunk_: a struct containing a `const void*` named `bytes`, pointing to the data to be sent back to the host,
  and a `size_t` `size` attribute representing the amount of bytes available in the data.
  See ["chunk.h"](https://github.com/51390/ecap-stream/blob/main/src/chunk.h) for details.
  It should usually be acceptable to return a `null` `bytes` value along with a `0` size -- but this ultimately
  depends on what the host expects, which falls outside the scope of eCap Stream.

### _void_ done(_int_ id)

This function signifies that the host has no additional raw data to be sent to the client module.
There will be usually one last call to `send` so that any trailing data (such as stream terminators) can
be properly sent back to the host.

_Parameters_:
  - **id** _(int)_: the transaction id for which data has been fully transmitted.

### _void_ cleanup(_int_ id)

This function signifies transaction termination, and serves to cleanup any transaction
related resources in the client module. No other calls are to be expected after this,
and it should be called only once.

_Parameters_:
  - **id** _(int)_: the transaction id to be terminated.

## Configuration

The `modulePath` configuration parameter must be passed to eCap-stream, so it can
find the client module implementation. This varies depending on the eCap host eCap-stream
is to run on, so we will provide an example for configuring the [Squid](http://www.squid-cache.org/)
proxy server. Always refer to the documentation for the host you want to use eCap-stream with.

Example Squid configuration (squid.conf):

```
...
loadable_modules /usr/local/lib/libecap-stream.so

ecap_enable on
ecap_service lens_respmod respmod_precache \
                 uri=ecap://github.com/51390/ecap-stream/respmod \
                 modulePath=/usr/local/lib/libclient-module.so
ecap_service lens_reqmod reqmod_precache \
                 uri=ecap://github.com/51390/ecap-stream/reqmod \
                 modulePath=/usr/local/lib/libclient-module.so

adaptation_access lens_respmod allow all
adaptation_access lens_reqmod allow all

...
```

This example would load a client module in the path `/usr/local/lib/libclient-module.so`,
that should implement the [API](#API) described above. After the host initializes the eCap
interface and service, transactions should induce the data exchange between host, eCap-stream
and finally, the client module.

By configuring both REQMOD and RESPMOD modes, the client module should then start seeing transactions
both in the _request_ as well as _response_ sides of the communication. Squid -- and thus eCap stream --
will usually send 2 transactions related to the _request_ leg, and 1 transaction related to the _response_
leg of the communication, but this may vary depending on the flow between `client -> proxy -> server`.

## References

 - [eCap documentation](https://www.e-cap.org/docs/)
 - [Squid eCap support](https://wiki.squid-cache.org/Features/eCAP)
