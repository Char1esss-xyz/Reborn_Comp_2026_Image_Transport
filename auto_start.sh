#! /usr/bin/env bash
export LD_LIBRARY_PATH=/home/reborn/桌面/Reborn_Comp_Image_Transport/hik_sdk/lib:$LD_LIBRARY_PATH

cd /home/reborn/桌面/Reborn_Comp_2026_Image_Transport/build
LD_LIBRARY_PATH=/opt/MVS/lib:$LD_LIBRARY_PATH ./hik_web_stream
