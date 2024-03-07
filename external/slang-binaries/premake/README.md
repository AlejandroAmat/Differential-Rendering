# premake

Slang and many of it's surrounding libraries use [premake](https://premake.github.io/). 

This directory holds binaries for premake. For windows, macos and linux it is possible to use the premake binaries that 
are downloadable directly from the [premake download page](https://premake.github.io/download/).

Linux Certificates Issue
========================

`slang-pack` uses the http module in premake to download packages. It is common for packages to be located at https based urls. For https to work premake needs
to verify certificates, by using a 'certificate bundle'. By default premake is built using `/etc/ssl/cert.pem` as the package bundle location. 

Unfortunately this doesn't appear to work some linux versions. For example CentOS 7, has it's certificates located at `/etc/ssl/certs/ca-bundle.crt`.

Normal `premake` builds do not allow for specifying where certificates are located. To work around this problem we build a special version of 
premake which requires the certificate bundle to be specified in the `CURL_CA_BUNDLE` environment parameter. This binary is in the centos-7-x64 directory. 
The binary is built for CentOS 7 because this linux distro initially had the problem, and also because CentOS 7 binaries have a high success of working 
across other distros.

Note that the `CURL_CA_BUNDLE` *must* be specified for CentOS 7 binary otherwise downloads via `https` will cause an error of the form:

```
Error: .../external/slang-binaries/lua-modules/slang-pack.lua:190: Unable to download 'https://github.com/shader-slang/slang-llvm/releases/download/v13.x-19/slang-llvm-v13.x-19-linux-x86_64-release.zip' 
SSL peer certificate or SSH remote key was not OK
Cert verify failed: BADCERT_NOT_TRUSTED
code:0.0
```

If the certificate bundle is located in the file `/etc/ssl/certs/ca-bundle.crt` premake can be invoked by first setting the `CURL_CA_BUNDLE` as shown below

```
export CURL_CA_BUNDLE=/etc/ssl/certs/ca-bundle.crt
premake5 gmake --deps=true 
```

Building CentOS 7 premake
=========================

First download the [premake source](https://premake.github.io/download/) this package will have the projects in a state ready to build. 

In `build/gmake2.unix` directory, first we want to add `-std=c99` in the file `mbedtls-lib.make`

```
ALL_CPPFLAGS += $(CPPFLAGS) -MMD -MP $(DEFINES) $(INCLUDES) -std=c99
```

Without this change building `mbedtls` fails. 

Now we want to `CURL_CA_BUNDLE` specify the certificate bundle. Edit `curl-lib.make` and replace all of (there are more than one)

```
-DCURL_CA_BUNDLE=\"/etc/ssl/cert.pem\"
```

with

```
-DCURL_WANTS_CA_BUNDLE_ENV
```

Now we can build [build premake](https://github.com/premake/premake-core/blob/master/BUILD.txt) with:

```
make config=release
```

You should now have binaries in the path from the root of `bin/release'. 

Certificates
============

Certificates that are known to work correctly on linux with premake are located in `slang-binaries` repo in the file `certificate/linux/ca-bundle.crt`. 
This certificate bundle is from CentOS 7, and may need to be updated if certificates change.

For example from the slang root, with slang-binaries submodule

```
export CURL_CA_BUNDLE=external/slang-binaries/certificate/linux/ca-bundle.crt
premake gmake --deps=true 
```
