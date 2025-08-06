# Building the Rust library for Android


In a UNIX environment, you can just invoke the ```make``` command
which will execute the actions listed below:

## Getting ready

### Import Rust environment
```source ~/.cargo/env```

### Setting clang as linker (?needed for Windows?)
```export RUSTFLAGS="-C linker=clang"```

### Installing Android NDK Targets for Rust
```rustup target add aarch64-linux-android ```
```rustup target add armv7-linux-androideabi```

### Installing cargo-ndk helper
```cargo install cargo-ndk```


## Compile Library

### Build Library for different architectures
- Build for 64-bit (arm64-v8a)
```cargo ndk -t arm64-v8a -t armeabi-v7a -o ./jniLibs build --release```


### Move Libraries to the tinySSB Code Tree
- Create destination directories
```
mkdir -p ../android/tinySSB/app/src/main/jniLibs/arm64-v8a   
mkdir -p ../android/tinySSB/app/src/main/jniLibs/armeabi-v7a 
```
- Copy the 64-bit version
```
cp jniLibs/arm64-v8a/libiroh_kotlin_bridge.so \
/PATH_TO_tinySSB_REPO/android/tinySSB/app/src/main/jniLibs/arm64-v8a/
```
- Copy the 32-bit version
```
cp jniLibs/armeabi-v7a/libiroh_kotlin_bridge.so \
/PATH_TO_tinySSB_REPO/android/tinySSB/app/src/main/jniLibs/armeabi-v7a/ 
```

---
