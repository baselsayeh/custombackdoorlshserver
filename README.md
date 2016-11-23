# custombackdoorlshserver
Backdoor lsh server for use with CVE-2016-5195.

change u:r:system_server:s0 if necessary

note:if you cant compile then copy libselinux from your device to lib folder in your platform (and arch) folder

to compile with ndk:

replace [put ndk path here] to your ndk path and replace arm64-v8a and/or android-23 if necessary

export ndkpath=[put ndk path here]

export PATH=$ndkpath:$PATH

ndk-build APP_BUILD_SCRIPT=./Android.mk NDK_PROJECT_PATH=$ndkpath NDK_LIBS_OUT=./bin NDK_OUT=. TARGET_ARCH_ABI=arm64-v8a TARGET_PLATFORM=android-23
