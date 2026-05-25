#!/bin/sh


usage() {
  echo "  Usage:"
  echo "    $0 [proj | fw | clean]"
  echo "    $0 fw <driver>"
  exit 1
}

build_fw() {
  cd framework/
  echo "  Building framework:"
  make
}

build_proj() {
  cd proj/code/
  echo "  Building project:"
  make
}

clean_all() {
  cd framework/
  make clean
  cd ../proj/code/
  make clean
}

if [ "$1" = "proj" ]; then
  build_fw
  cd ../
  build_proj
  lcom_run proj

elif [ "$1" = "fw" ]; then
  build_fw
  lcom_run proj "$2"

elif [ "$1" = "clean" ]; then
  clean_all

else 
  usage $0
  exit 1

fi
  