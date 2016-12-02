# WPF POC connection over fiber 

It's reworked **Prism** project.

## Differencies from Prism
- x64 binaries;
- available *Release Configuration*;
- videorendering is doing to D3DImage directly, bypassing Window (hWnd).

Repository contains Fiber as submodule. Submodule heads to *skinny3* branch. This repo contains fork of Fiber repo.
It has the same codebase as origin/skinny3 the difference is that here *features* are enbled. 

## Prerequisits

*Fiber requires:*
- minGW x64;
- CMake [link](https://cmake.org/files/v3.6/cmake-3.6.2-win64-x64.msi)
- YASM  [link](http://www.tortall.net/projects/yasm/releases/yasm-1.3.0-win64.exe) , file should be renamed to yasm.exe. Path to executable should be added to **PATH** environment variable
- Python, was checked with v 2.7
- Visual Studio 2013 Community Edition [link](https://www.visualstudio.com/en-us/news/releasenotes/vs2013-community-vs)

*WPF client requires:*
- Visual Studio 2015 Community Edition [link](https://www.visualstudio.com/downloads/)


## Check out

After checking out this repo should be checked out submodules as well

`git submodule update --init --recursive`


## Build
Before to build ensure that all prerequisites are installed correctly.

### Build fiber
Go to `fiberapp` folder and from command line run:

`scripts\buildfiber_x64.bat`

it should build a bunch of libraries.

### Build WPF client

In Visual Studio 2015 open solution `WpfFiber.sln` and build it. The result of build would be:
- winfbrapp.dll
- WpfFiber.exe
- WpfFiber.exe.conf

binaries will be located at `WpfFiber\bin\$(Configuration)` folder.