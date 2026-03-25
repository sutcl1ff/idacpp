 Build the idacpp C++ scripting plugin for IDA Pro from source. Execute each step in sequence.

  First, detect which operating system you are running on and adapt all commands accordingly — Windows uses cmd/PowerShell and cmake -A x64 with MSVC, Linux/macOS
  uses bash and cmake -G Ninja.

  Step 1 — IDA SDK. Check if the IDASDK environment variable (%IDASDK% on Windows) is already set and points to a valid SDK directory (containing include/pro.h or
  src/include/pro.h). If so, skip this step. Otherwise, clone the IDA SDK and set the variable:
  git clone https://github.com/hexrayssa/ida-sdk.git
  export IDASDK=/absolute/path/to/ida-sdk

  Step 2 — clinglite + LLVM/Cling. Clone clinglite with submodules (--shallow-submodules saves ~3 GB), then build the Cling/LLVM tree using clinglite's build
  script. This is a one-time ~30-90 min build. On Windows, LLVM must use static CRT (-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded). If the build script doesn't handle
   this, build LLVM manually with Ninja+MSVC, targeting only the cling libraries — not cling.exe, which cannot link with /MT.
  git clone --recurse-submodules --shallow-submodules https://github.com/0xeb/clinglite.git
  cd clinglite
  python scripts/build_cling.py -j <cores>

  Step 3 — idacpp. Clone idacpp and build it against clinglite and the IDA SDK. The CMakeLists.txt handles CRT matching, path normalization, and IDA SDK bootstrap
  automatically. Use -G Ninja on Linux/macOS and -A x64 on Windows (MSVC).
  git clone https://github.com/allthingsida/idacpp.git
  cd idacpp && mkdir build && cd build
  # Linux / macOS:
  cmake -G Ninja \
      -DCLINGLITE_SOURCE_DIR=<path/to/clinglite> \
      -DCLING_BUILD_DIR=<path/to/clinglite>/build-cling \
      -DIDASDK=$IDASDK \
      ..
  # Windows (MSVC):
  cmake -A x64 \
      -DCLINGLITE_SOURCE_DIR=<path/to/clinglite> \
      -DCLING_BUILD_DIR=<path/to/clinglite>/build-cling \
      -DIDASDK=%IDASDK% \
      ..
  cmake --build . --config Release

  Plugins: idacpp has a modular plugin system (see plugins/README.md). Platform plugins (winsdk on Windows, linux on Linux) auto-enable and require no extra flags.

  Optional plugins must be cloned separately and pointed to via CMake flags:
  - idax (C++23 IDA SDK wrapper by Kenan Sulayman):
    git clone https://github.com/19h/idax.git /path/to/idax
    Then add to the cmake configure step above: -DPLUGIN_IDAX_SRC_DIR=/path/to/idax

  To disable an auto-enabled platform plugin: add e.g. -DPLUGIN_WINSDK=OFF to the cmake configure step.
  Enabled plugins appear in the startup banner.

  Step 4 — Verify. Confirm the plugin binary (idacpp.dll, idacpp.so, or idacpp.dylib) was placed in $IDASDK/src/bin/plugins/. Launch IDA and check that the C++ tab appears in the output window.
