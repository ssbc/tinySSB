# Makefile

all:
	source ~/.cargo/env
	rustup target add aarch64-linux-android
	rustup target add armv7-linux-androideabi
	cargo install cargo-ndk
	cargo ndk -t arm64-v8a -t armeabi-v7a -o ./jniLibs build --release
	mkdir -p ../android/tinySSB/app/src/main/jniLibs/arm64-v8a   
	mkdir -p ../android/tinySSB/app/src/main/jniLibs/armeabi-v7a 
	cp jniLibs/arm64-v8a/libiroh_kotlin_bridge.so \
		../android/tinySSB/app/src/main/jniLibs/arm64-v8a/
	cp jniLibs/armeabi-v7a/libiroh_kotlin_bridge.so \
		../android/tinySSB/app/src/main/jniLibs/armeabi-v7a/

# eof
