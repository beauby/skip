ARG COMMIT_HASH=deadbeef
FROM skiplabs/skip:${COMMIT_HASH} AS build

COPY ./skiplang /skiplang
WORKDIR /skiplang/compiler
# FIXME: This is redundant (it rebuilds the stage0).
RUN make
COPY . /work
WORKDIR /work/skipruntime-ts
RUN skargo build -r --lib --manifest-path=skiplang/ffi/Skargo.toml --out-dir=/usr/lib/
ENV SKIPRUNTIME=/usr/lib
RUN make install build
WORKDIR /work
RUN mkdir /packages && npm pack --pack-destination=/packages \
    -w eslint-config \
    -w tsconfig \
    -w skiplang/prelude/ts/binding \
    -w skiplang/prelude/ts/wasm \
    -w skiplang/prelude/ts/worker \
    -w skiplang/skdate/ts \
    -w skiplang/skjson/ts/binding \
    -w skiplang/skjson/ts/wasm \
    -w skipruntime-ts/core \
    -w skipruntime-ts/helpers \
    -w skipruntime-ts/wasm \
    -w skipruntime-ts/addon \
    -w skipruntime-ts/adapters/kafka \
    -w skipruntime-ts/adapters/postgres \
    -w skipruntime-ts/server \
    -w skipruntime-ts/metapackage

FROM skiplabs/skip:${COMMIT_HASH}

COPY --from=build /skiplang/compiler/stage1/bin/skc /usr/bin/
COPY --from=build /skiplang/compiler/stage1/bin/skargo /usr/bin/
COPY --from=build /skiplang/compiler/stage1/lib/libstd.sklib /usr/lib/
COPY --from=build /usr/lib/libskip-runtime-ts.so /usr/lib/libskip-runtime-ts.so
COPY --from=build /packages /packages
