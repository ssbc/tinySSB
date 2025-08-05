# Building the Rust library for Android

## Getting ready

### Installing Android NDK Targets for Rust
```rustup target add aarch64-linux-android ```
```rustup target add armv7-linux-androideabi```

### Installing cargo-ndk helper
```cargo install cargo-ndk```

## Compile Library

### Build Library for different architecture
- Build for 64-bit (arm64-v8a)
```cargo ndk -t arm64-v8a -o ./jniLibs build --release```
- Build for 32-bit (armeabi-v7a)
```cargo ndk -t armeabi-v7a -o ./jniLibs build --release```

### Move Library to TinySSB Code
- Copy the 64-bit version
```
cp jniLibs/arm64-v8a/libiroh_kotlin_bridge.so \
/path/to/tinySSB/android/tinySSB/app/src/main/jniLibs/arm64-v8a/
```

- Copy the 32-bit version
```
cp jniLibs/armeabi-v7a/libiroh_kotlin_bridge.so \
/path/to/tinySSB/android/tinySSB/app/src/main/jniLibs/armeabi-v7a/ 
```