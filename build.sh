#!/bin/bash

PRESET_NAME=$1
HOST_OS=$OS$OSTYPE
HOST_TYPE=$HOSTTYPE
HOST_ID="${HOST_OS}-${HOST_TYPE}"

# Convert HOST_ID to lowercase
if ((BASH_VERSINFO[0] < 3))
then
  HOST_ID=$(tr '[:lower:]' '[:upper:]' <<< $HOST_ID)
else
  HOST_ID=${HOST_ID,,}
fi

echo "Host OS: $HOST_OS"
echo "Host Type: $HOST_TYPE"
echo "Host Id: $HOST_ID"

if [[ $1 == "" ]]; then
  case "$HOST_ID" in
    "windows_"*)
      # Windows
      PRESET_NAME="msvc-win64-release"
      ;;
    "darwin"*)
      # Mac OSX
      PRESET_NAME="clang-macos-release"
      ;;
    "linux-"*)
      # Linux
      PRESET_NAME="gcc-linux-release"
      ;;
    *)
      # Default case
      echo "Unsupported OS or architecture"
      exit 1
      ;;
  esac
fi

echo "Building ${PRESET_NAME}"

# Detect if CC and CXX are set in the environment
CMAKE_COMPILER_ARGS=""
if [[ -n "$CC" && -n "$CXX" ]]; then
  echo "Using custom compiler: CC=$CC, CXX=$CXX"
  CMAKE_COMPILER_ARGS="-DCMAKE_C_COMPILER=$CC -DCMAKE_CXX_COMPILER=$CXX"
fi

cmake --preset "${PRESET_NAME}" ${CMAKE_COMPILER_ARGS} && cmake --build --preset "${PRESET_NAME}" -j
