# m5stickc-playground

Test project for the M5StickC Plus with PlatformIO.

## Commands

```sh
.venv/bin/pio run -e m5stickcplus
.venv/bin/pio run -e m5stickcplus -t upload
```

Uploads use `/dev/ttyUSB0`, configured in `platformio.ini`.

If uploading only works with `sudo`, add your user to the `dialout` group and sign in again:

```sh
sudo usermod -aG dialout $USER
```

If you already ran PlatformIO with `sudo`, some build artifacts may be owned by `root`. You can clean the local build with:

```sh
sudo rm -rf .pio
```
