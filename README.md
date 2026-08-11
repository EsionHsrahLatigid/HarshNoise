# HarshNoise

Brutal digital harsh-noise generator plug-in for VST3, AU, and Standalone hosts.

HarshNoise combines bit crushing, sample-rate decimation, aggressive feedback,
chaotic modulation, and stutter-style glitching for destructive digital noise
textures.

## Identity

- Owner: EsionHsrahLatigid
- Company: EsionHsrahLatigid
- Manufacturer code: EHL_
- Plug-in code: Hrsh
- Bundle ID: jp.ehl.harshnoise

## Build

Requirements:

- CMake 3.22 or newer
- A C++17 compiler
- Xcode on macOS for AU and Standalone builds

JUCE 8.0.15 is fetched automatically when a local `JUCE/` checkout is not
present.

```bash
cmake -S . -B build/release -DCMAKE_BUILD_TYPE=Release \
  -DHARSHNOISE_BUILD_PLUGIN=ON \
  -DHARSHNOISE_BUILD_TESTS=ON
cmake --build build/release --target HarshNoise_Artifacts HarshNoiseSmokeTests --parallel 2
ctest --test-dir build/release --output-on-failure
```

Staged products are written to:

- `artifacts/Release/VST3/HarshNoise.vst3`
- `artifacts/Release/AU/HarshNoise.component` on macOS
- `artifacts/Release/Standalone/HarshNoise.app` on macOS

## Parameters

| Parameter | Range | Description |
| --- | --- | --- |
| CRUSH | 1-16 bits | Bit-depth reduction and quantization noise |
| DECIMATE | 1-64x | Sample-and-hold decimation |
| FEEDBACK | 0-150% | Short unstable feedback delay |
| CHAOS | 0-100% | Chaotic modulation and stutter probability |
| MIX | 0-100% | Dry/wet balance |
| OUTPUT | -24 to +12 dB | Output gain |

## Warning

This plug-in is intentionally capable of abrupt, loud output. Start with low
monitor levels, especially when feedback or chaos is high.

## License

MIT. See `LICENSE`.
