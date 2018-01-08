# REWAVI

[![Build status](https://ci.appveyor.com/api/projects/status/ffcocigoiclo2ekr/branch/master?svg=true)](https://ci.appveyor.com/project/wieslawsoltes/rewavi/branch/master)

[![Github All Releases](https://img.shields.io/github/downloads/wieslawsoltes/rewavi/total.svg)](https://github.com/wieslawsoltes/rewavi/releases)
[![GitHub Release](https://img.shields.io/github/release/wieslawsoltes/rewavi.svg)](https://github.com/wieslawsoltes/rewavi/releases/latest)
[![Github Releases](https://img.shields.io/github/downloads/wieslawsoltes/rewavi/latest/total.svg)](https://github.com/wieslawsoltes/rewavi/releases)

rewritten/modified WAVI

## Download REWAVI

| Platform           | Character Set  | Version   | Download                                                                                                                |
|--------------------|----------------|-----------|-------------------------------------------------------------------------------------------------------------------------|
| Windows 32-bit     | MBCS           | 0.02      | [rewavi-0.02-Win32.zip](https://github.com/wieslawsoltes/rewavi/releases/download/0.02/rewavi-0.02-Win32.zip)           |
| Windows 64-bit     | MBCS           | 0.02      | [rewavi-0.02-x64.zip](https://github.com/wieslawsoltes/rewavi/releases/download/0.02/rewavi-0.02-x64.zip)               |

## System requirements

Minimum supported Windows version is Windows 7 SP1 or above, recommended is Windows 10 Anniversary Update.

Provided binaries should work under Windows XP Service Pack 3 (SP3) for x86, Windows XP Service Pack 2 (SP2) for x64.

Minimum supported Linux version is Ubutnu 16.10 (using Wine 2.0).

## CI Builds

[Download](https://ci.appveyor.com/project/wieslawsoltes/rewavi/build/artifacts) bleeding edge builds from the CI server.

## About

REWAVI - tool of extract PCM audio stream from avi file.

RESILENCE - tool of generate silence PCM WAV file.

## Changelog

* v0.02 (2018-01-06)
  - Visual Studio 2017 support.
  - Bug fixes.
  - x64 builds.
  - Added Cake build script.
  - Add appveyor configuration for using Cake build system.

* v0.01 (2011-11-19)
  - Initial release.

## Usage

* [Usage](https://github.com/wieslawsoltes/rewavi/blob/master/docs/usage.txt)
* [Usage2](https://github.com/wieslawsoltes/rewavi/blob/master/docs/usage2.txt)

## Troubleshooting

To report issues please use [issues tracker](https://github.com/wieslawsoltes/rewavi/issues).

For more informations and help please visit [this forum thread](http://forum.doom9.org/showthread.php?t=175174).

## Sources

Sources are available in the [git source code repository](https://github.com/wieslawsoltes/rewavi/).

## Build

### Prerequisites

```
git clone https://github.com/wieslawsoltes/rewavi.git
cd rewavi
git submodule update --init --recursive
```

### Install Visual Studio 2017

* [VS 2017](https://www.visualstudio.com/pl/downloads/)

### Windows 7 SDK

For Windows XP compatibility program is compiled using `Platform Toolset` for `Visual Studio 2017 - Windows XP (v141_xp)`.

For more details please read [Configuring Programs for Windows XP](https://msdn.microsoft.com/en-us/library/jj851139.aspx).

### Build Solution
```
Open rewavi.sln in Visual Studios 2017 or above.
```

## Copyrights

This is fork of [rewavi](https://github.com/chikuzen/rewavi) which is rewritten/modified [WAVI](http://sourceforge.net/projects/wavi-avi2wav/).

## License

rewavi is licensed under the [GNU Genenral Public License version 2 or later](LICENSE.TXT).
