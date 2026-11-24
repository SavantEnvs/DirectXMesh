# syntax=docker/dockerfile:1.7
# Commit image for DirectXMesh (savantenvs integration). Build from the repo root:
#   DOCKER_BUILDKIT=1 docker build -f mayhem/Dockerfile --build-arg COMMIT=$(git rev-parse HEAD) -t dxmesh .
FROM ghcr.io/savantenvs/base:20261023

ARG COMMIT
ARG REPO_URL
ARG SOURCE_DATE_EPOCH
ARG SANITIZER_FLAGS
ARG COVERAGE_FLAGS=
ARG MAYHEM_JOBS=

# On Linux upstream's CMake requires the DirectXMath and DirectX-Headers CMake packages (upstream CI
# gets them from vcpkg, whose directxmath port also installs dotnet's portable sal.h next to the
# DirectXMath headers). Neither is in Debian, so both are built once from pinned commits into static
# prefixes under /opt/toolchains/deps (SPEC §6.2 item 19). Both are header-only for this build.
USER root
ARG DXMATH_COMMIT=6ac6a3c51d4d99237656f22486ae5177c14c565d
ARG DXMATH_SHA256=0e122170d259c73a101c807fce56a1c6b2f91c9a073430b27c15aaffc043f8d4
ARG DXHEADERS_COMMIT=61c7718954ec73dbdd43c723240fe1a0452a3724
ARG DXHEADERS_SHA256=ae52d65bbc2e1c42f5b85144818ad765d40d0d37421a582e06ac2b29388c8161
ARG SAL_COMMIT=cdb6e1d5f9075214c8a58ca75d5314b5dc64daed
ARG SAL_SHA256=7dae281adc3a09a691291fb90526f05e4f9ef8b16d7f33d716ba690f7241a492
RUN set -eux; \
    d=/opt/toolchains/deps/directxmath-${DXMATH_COMMIT}; mkdir -p "$d/src" /tmp/b1; \
    curl -fsSL -o /tmp/m.tgz https://github.com/microsoft/DirectXMath/archive/${DXMATH_COMMIT}.tar.gz; \
    echo "${DXMATH_SHA256}  /tmp/m.tgz" | sha256sum -c -; \
    tar -xzf /tmp/m.tgz --strip-components=1 -C "$d/src"; \
    cmake -S "$d/src" -B /tmp/b1 -DCMAKE_INSTALL_PREFIX="$d" -DBUILD_TESTING=OFF; cmake --install /tmp/b1; \
    curl -fsSL -o "$d/include/sal.h" \
      https://raw.githubusercontent.com/dotnet/runtime/${SAL_COMMIT}/src/coreclr/pal/inc/rt/sal.h; \
    echo "${SAL_SHA256}  $d/include/sal.h" | sha256sum -c -; \
    h=/opt/toolchains/deps/directx-headers-${DXHEADERS_COMMIT}; mkdir -p "$h/src" /tmp/b2; \
    curl -fsSL -o /tmp/h.tgz https://github.com/microsoft/DirectX-Headers/archive/${DXHEADERS_COMMIT}.tar.gz; \
    echo "${DXHEADERS_SHA256}  /tmp/h.tgz" | sha256sum -c -; \
    tar -xzf /tmp/h.tgz --strip-components=1 -C "$h/src"; \
    cmake -S "$h/src" -B /tmp/b2 -DCMAKE_INSTALL_PREFIX="$h" -DDXHEADERS_BUILD_TEST=OFF \
      -DDXHEADERS_BUILD_GOOGLE_TEST=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="-g -gdwarf-3"; \
    cmake --build /tmp/b2 -j"$(nproc)"; cmake --install /tmp/b2; \
    find "$h" -name '*.so*' -delete; \
    rm -rf /tmp/b1 /tmp/b2 /tmp/m.tgz /tmp/h.tgz; chmod -R a+rX /opt/toolchains/deps
ENV DXMESH_DEPS_PREFIX=/opt/toolchains/deps/directxmath-6ac6a3c51d4d99237656f22486ae5177c14c565d;/opt/toolchains/deps/directx-headers-61c7718954ec73dbdd43c723240fe1a0452a3724

USER mayhem
WORKDIR /mayhem
COPY --chown=2000:2000 . /mayhem

RUN if [ -x /opt/mayhem/ccache/prefill ]; then /opt/mayhem/ccache/prefill bash mayhem/build.sh; \
    else bash mayhem/build.sh; fi

RUN if [ -f mayhem/test.sh ]; then bash mayhem/test.sh; else echo "no mayhem/test.sh"; fi

LABEL org.opencontainers.image.source="${REPO_URL}" \
      org.opencontainers.image.revision="${COMMIT}" \
      rlenv.tier="commit" \
      rlenv.source_date_epoch="${SOURCE_DATE_EPOCH}" \
      rlenv.user="mayhem"

USER mayhem
WORKDIR /mayhem
