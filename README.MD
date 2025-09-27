# Reborn 2026队内赛图传
## 自启动配置
- 将本文件夹复制到桌面上
- 检查rc.service与auto_start.sh配置与本机环境是否一致（一般情况下一致）
- 配置rc.service：<br>
1. 复制[rc.service](rc.service)中的内容
2. 运行以下命令
``` bash
sudo rm -rf /etc/systemd/system/rc.service
sudo nano /etc/systemd/system/rc.service
```
3. 将复制的内容粘贴进去
4. 运行以下命令
``` bash
sudo systemctl daemon-reload
sudo systemctl enable rc.service
```
5. 检验配置是否成功
``` bash
sudo systemctl status rc.service
```

## 编译
``` bash
mkdir build && cd build
cmake ..
make
```

## 运行
- 开机自启动（配置如上文所示）
- 运行可执行文件
``` bash
./build/hik_web_stream
```
- 运行启动脚本
```bash
bash auto_start.sh
```

## 使用
**请使用Firefox浏览器，Chromium内核浏览器会因为某些问题导致图像无法正常显示**<br>
访问http://<nuc的ip地址>:8080即可使用