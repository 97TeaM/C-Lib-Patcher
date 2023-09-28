# C-Lib-Patcher
Fixed, reupload distribution of C-Lib-Patcher, inspried by FЯIDA


Reupload from main git repo, currently deleted

```./old``` directory contains raw concepts made by me near August-September 2023.

## Pre-requirements
Currently needed only official **LLVM** toolchain from Andoroid NDK, avilable *[here](https://developer.android.com/ndk/downloads)*

---

### Injector compilation: ```make```

---

### The patch library compilation: ```clang++ -fPIC -shared -o libpatch.so libpatch.cpp -target armv7a-linux-androideabi19 -static-libstdc++```

See examples in ```injector.cpp``` and edit patches for your own!
