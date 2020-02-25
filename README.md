# PathTracer

## Description
Creating a small path tracer piece by piece with the UMN Applied Motion Lab.

## Configuring
Tested on GCC9 and MSVC 2019
```
git clone https://github.com/LiamTyler/PathTracer.git
cd PathTracer
mkdir build
cd build
(linux) cmake -DCMAKE_BUILD_TYPE=[Debug/Release] ..
(windows) cmake -G "Visual Studio 16 2016" -A x64 ..
```

## Building
Tested on GCC9 and MSVC 2019

Any platform:
```
cmake --build . --config [Debug/Release]
```

Linux specifc:
```
make -j6
```
Note: With GCC9 a full parallel build (make -j) freezes during the assimp build on my machine. Just lower the number if it does freeze

Windows:
Just open build/PathTracer.sln and build there

## Running
```
./bin/pathTracer[_debug] <path to scene file> (ex: ../resources/scenes/simple.json)
```

## Example Output:
resources/scenes/simple.json rendered in 0.40 seconds @ 1000x1000 with 4x anti-aliasing on my Intel i7 8700k CPU
![Alt text](docs_and_images/exampleOutput.png)
