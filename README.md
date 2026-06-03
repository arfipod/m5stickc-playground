# m5stickc-playground

Test project for the M5StickC Plus with PlatformIO.

[![CI](https://github.com/arfipod/m5stickc-playground/actions/workflows/ci.yml/badge.svg)](https://github.com/arfipod/m5stickc-playground/actions/workflows/ci.yml)

## Commands

```sh
.venv/bin/pio run -e m5stickcplus
.venv/bin/pio run -e m5stickcplus -t upload
```

## CI/CD

GitHub Actions builds the firmware with PlatformIO for pushes and pull requests.
The CI workflow uploads `firmware.bin`, `firmware.elf`, `firmware.map`, and
`SHA256SUMS` as a build artifact.

Tagged pushes that match `v*`, for example `v1.0.0`, run the release workflow
and attach the compiled firmware files to a GitHub Release.

Uploads use `/dev/ttyUSB0`, configured in `platformio.ini`.

If uploading only works with `sudo`, add your user to the `dialout` group and sign in again:

```sh
sudo usermod -aG dialout $USER
```

If you already ran PlatformIO with `sudo`, some build artifacts may be owned by `root`. You can clean the local build with:

```sh
sudo rm -rf .pio
```
