# PathTracer

## Description
Creating a small path tracer piece by piece with the UMN Applied Motion Lab

## Building
Tested on GCC9 and MSVC 2019
```
git clone https://github.com/LiamTyler/PathTracer.git
cd PathTracer
mkdir build
cd build
(linux) cmake -DCMAKE_BUILD_TYPE=[Debug/Release] ..
(windows) cmake -G "Visual Studio 16 2016" -A x64 ..
```

## Running
```
./bin/pathTracer[_debug] <path to scene file>
```
