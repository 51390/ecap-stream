# eCap Stream

![Tests](https://github.com/51390/ecap-stream/actions/workflows/test.yml/badge.svg)

This is an [eCap adapter](https://www.e-cap.org/) that receives body data from the
host and forwards it to an external library, to be then recorded, adapted or
filtered at will. The processed -- or unprocessed -- data is then sent back to the host.

The intent of _eCap Stream_ is to provide a simpler interface than fully implementing
the eCap protocol.

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

### _void init()_

This function should perform any module wide initialization required, such as specific logging
or runtime setups that may be needed. It is called once per eCap-service initialization, which
usually translates into a single call per host lifecycle.

### _void uri(int id, const char* uri)_

This function is the first called for each transaction. In the context of web requests or responses,
note that each flow of information is separately handled by different transactions. So, there could be
an initial transaction for handling the request and a matching subsequent transaction for handling the
respective response.

Parameters:
 - ** id (int) **: this is an integer that represents the transaction identifier. It will be passed down
 throughout all other interactions between eCap stream and the client module.
 - ** uri (const char*) **: a C null-terminated string, containing the full uri of the request (inluding
 query string parameters).

### _void header(int id, const char* name, const char* value)_

This function will notify about every request or response header that is seen in the transaction.
There will be one call per header.

Parameters:
  - ** id (int) **: the transaction id to which the header belongs to.
  - ** name (const char*) **: a null-terminated string representing the header name.
  - ** value (const char*) **: a null-terminated string representing the header value.

### _void receive(int id, const void* data, libecap::size_type size)_

This function notifies the client module of raw data available from the host, which usually is a part
of a request or response body. None, a single, or multiple chunks may be sent, and the only guarantees made
are on the order and completeness, meaning that chunks are sent ordered and all data in the body will
be eventually sent down as chunks, if any data exists -- empty bodies may result in no calls being made.

Parameters:
  - **id (int)**: the transaction id to which the chunk of data belongs to.
  - **data (const void*)**: a pointer to an array of bytes that contains the available data from the host.
  - **size (libecap::size_type)**: the length of data available.

### _[Chunk](https://github.com/51390/ecap-stream/blob/main/src/chunk.h) send(int id, libecap::size_type offset, libecap::size_type size)_

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

### _void done(int id)_

This function signifies that the host has no additional raw data to be sent to the client module.
There will be usually one last call to `send` so that any trailing data (such as stream terminators) can
be properly sent back to the host.

_Parameters_:
  - **id** _(int)_: the transaction id for which data has been fully transmitted.

### _void cleanup(int id)_

This function signifies transaction termination, and serves to cleanup any transaction
related resources in the client module. No other calls are to be expected after this,
and it should be called only once.

_Parameters_:
  - **id** _(int)_: the transaction id to be terminated.

## Configuration

The `modulePath` configuration parameter must be passed to eCap-stream, so it can
find the client module implementation. This varies depending on the eCap host eCap-stream
is to run on, so we will provide an example for configuring the [Squid](http://www.squid-cache.org/)
proxy server. Always refer to the host documentation you want to use eCap-stream with.

Example Squid configuration (squid.conf):

```
...
loadable_modules /usr/local/lib/libecap-stream.so

ecap_enable on
ecap_service lens_respmod respmod_precache \
                 uri=ecap://github.com/51390/ecap-stream \
                 modulePath=/usr/local/lib/lib-client-module.so

adaptation_access lens_respmod allow all

...
```

This example would load a client module in the path `/usr/local/lib/lib-client-module.so`,
and then perform the calls as detailed per the [API](#API) section above, for every response
transaction that passes through the Squid proxy.

For additional details about eCap, and Squid's implementation, refer to the [References](#References) below.

## References

 - [eCap documentation](https://www.e-cap.org/docs/)
 - [Squid eCap support](https://wiki.squid-cache.org/Features/eCAP)
