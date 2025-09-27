#! /usr/bin/env bash
export LD_LIBRARY_PATH=/home/reborn/桌面/img_transport_demo/hik_sdk/lib:$LD_LIBRARY_PATH

cd /home/reborn/桌面/img_transport_demo/build
LD_LIBRARY_PATH=/opt/MVS/lib:$LD_LIBRARY_PATH ./hik_web_stream
