# OBS Auto Crop

English | [한국어](README.ko.md)

Choose **Transform → Auto Crop** for an OBS source to detect black borders in its current frame and apply the crop to that scene item. This is a one-time command, and you can undo it with OBS's Undo command.

The plugin provides localized menu and Undo labels for the 77 language codes supported by OBS 32.2.2. Error and guidance messages are available in Korean and English; other languages fall back to English.

## Automated releases

Pushing a three-part version tag beginning with `v`, such as `v1.0.0`, triggers `.github/workflows/release.yml` to build the plugin for Windows x64, macOS universal (Apple Silicon and Intel), and Ubuntu 26.04 x86_64. If all three builds and their tests pass, the workflow creates a GitHub Release with an installer (`.exe`, `.pkg`, or `.deb`) and a manual-install archive (`.zip`, `.zip`, or `.tar.gz`) for each OS. The macOS package and plugin are neither signed nor notarized.

```bash
git tag v1.0.0
git push origin v1.0.0
```

Push the workflow file to the default branch before creating a tag. The repository's Actions settings must allow `GITHUB_TOKEN` to create releases (`contents: write`). You can also run the workflow manually from the Actions page to check the builds; a manual run does not publish a release.

Use either the installer or the archive for your OS, not both.

| OS | Installer | Manual installation from archive |
| --- | --- | --- |
| Windows | Run `obs-auto-crop-VERSION-windows-x64-setup.exe`. In the wizard, select the OBS installation folder containing `bin/64bit/obs64.exe`. | Extract the ZIP's `plugins/` folder into `C:/ProgramData/obs-studio/`, creating `plugins/obs-auto-crop/`. |
| macOS | Open the `.pkg` and install it for the current user. The OBS app's location does not affect the plugin location. | Copy `obs-auto-crop.plugin` from the ZIP to `~/Library/Application Support/obs-studio/plugins/`. |
| Ubuntu 26.04 | Open the `.deb` in a package installer, or run `sudo apt install ./obs-auto-crop-v1.0.0-ubuntu-26.04-amd64.deb`. | Extract the tarball's `lib/` and `share/` directories under `/usr/`. For example: `sudo tar -C /usr -xzf obs-auto-crop-v1.0.0-ubuntu-26.04-x86_64.tar.gz`. |

The Windows installer places the plugin inside the selected OBS folder; the ZIP installs it under `C:/ProgramData/obs-studio/`. If you switch from the Windows ZIP to the installer, first remove the old `C:/ProgramData/obs-studio/plugins/obs-auto-crop/` copy. Restart OBS after installation.

The archives have different layouts to match each OS's OBS plugin paths: the Windows ZIP contains a `plugins/` tree, the macOS ZIP contains a `.plugin` bundle, and the Ubuntu tarball contains `lib/` and `share/` trees. Windows and macOS builds use the OBS 32.2.2 development SDK; the Ubuntu build uses the distribution's `libobs-dev` package. The Ubuntu `.deb` installs to system paths and has no option to change the destination. Its binary may not be compatible with OBS or Qt versions on other Linux distributions.

## Scope and limitations

- The command works when a single video source is selected. Locked items and groups are excluded.
- It downsizes the current frame to at most 1280 pixels for analysis, so very thin borders may go undetected.
- Avoid running it on a black frame or during a fade. If there is no content to detect, it does not apply a crop.
- It uses the OBS frontend's `transformMenu` Qt object. If an OBS update changes the menu structure, the integration may need an update.
