#!/bin/bash
SRC_DIR=/home/wuyuncheng/Documents/projects/CollaborativeML/src/include/meta
DST_DIR=/home/wuyuncheng/Documents/projects/CollaborativeML/src/include/protobuf/
protoc -I=$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/*.proto