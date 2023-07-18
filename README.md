# E-Cap Stream

![Tests](https://github.com/51390/ecap-stream/actions/workflows/test.yml/badge.svg)

This is an [E-Cap adapter](https://www.e-cap.org/) that receives body data from the host and forwards it to an external library, to be recorded, adapted, aborted or ignored.
The main advantage of E-Cap Stream is to provide a simpler interface than directly implementing the e-Cap protocol.

## Building

```
$ docker build -t ecap-stream .
```

## Testing

The default entrypoint runs a simple test case, so all that is needed is to execute the docker image.

```
$ docker run -it ecap-stream
```

## Using

The image can be used as the base image for a Dockerfile used in another project.

```
FROM ecap-stream

...
```
