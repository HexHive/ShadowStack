#!/bin/bash

all:
	rm -rf debug-install
	mkdir debug-install
	ninja -C ./debug-build install
	ninja -C ./libunwind-debug-build install
	
	#ninja -C ./libcxxabi-debug-build clean 
	#ninja -C ./libcxx-debug-build clean 
	
	#ninja -C ./libcxxabi-debug-build install
	#ninja -C ./libcxx-debug-build install

release:
	ninja -C ./release-build install
	ninja -C ./libunwind-release-build install
	ninja -C ./libcxxabi-release-build clean
	ninja -C ./libcxx-release-build clean 
	ninja -C ./libcxxabi-release-build install
	ninja -C ./libcxx-release-build install
