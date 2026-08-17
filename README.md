# HarshNoise

Bit-crushed decimation feedback plug-in for VST3, AU, and Standalone hosts.

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
cmake --build build/release --target ehl_stage_products HarshNoiseSmokeTests --parallel 2
ctest --test-dir build/release --output-on-failure
```

Release products are staged by `ehl_stage_products` under:

```text
artifacts/plugin-release/macos-arm64/standalone/harshnoise_standalone_plugin.app
artifacts/plugin-release/macos-arm64/vst3/harshnoise_vst3_plugin.vst3
artifacts/plugin-release/macos-arm64/au/harshnoise_au_plugin.component
artifacts/plugin-release/macos-arm64/ARTIFACTS.txt
artifacts/plugin-release/windows-x64/standalone/harshnoise_standalone_plugin.exe
artifacts/plugin-release/windows-x64/vst3/harshnoise_vst3_plugin.vst3
artifacts/plugin-release/windows-x64/ARTIFACTS.txt
```

On local macOS builds outside CI, VST3 and AU formats are also copied to the
current user's standard plug-in folders:

- `~/Library/Audio/Plug-Ins/VST3/HarshNoise.vst3`
- `~/Library/Audio/Plug-Ins/Components/HarshNoise.component`

Standalone remains in the artifact tree. CI and non-macOS builds do not copy by
default. Override with `-DEHL_COPY_PLUGIN_AFTER_BUILD=ON` or `OFF`.

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
