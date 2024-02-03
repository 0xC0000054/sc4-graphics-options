# sc4-graphics-options

A DLL Plugin for SimCity 4 that sets the game's rendering mode and resolution options.   

## Features

* Custom resolutions can be configured without command line arguments.
* Allows the game's intro to be disabled without a command line argument.
* Supports borderless full screen mode without requiring a launcher.
* Supports changing the game's rendering driver without a command line argument, see the Driver section of the setting overview.
* Allows full screen mode to use 32-bit color.

The plugin can be downloaded from the Releases tab: https://github.com/0xC0000054/sc4-graphics-options/releases

## System Requirements

* Windows 10 or later

The plugin may work on Windows 7 or later with the [Microsoft Visual C++ 2022 x86 Redistribute](https://aka.ms/vs/17/release/vc_redist.x86.exe) installed, but I do not have the ability to test that.

## Installation

1. Close SimCity 4.
2. Copy `SC4GraphicsOptions.dll` and `SC4GraphicsOptions.ini` into the Plugins folder in the SimCity 4 installation directory.
3. Configure the graphics options, see the `Configuring the plugin` section.

## Configuring the plugin

1. Open `SC4GraphicsOptions.ini` in a text editor (e.g. Notepad).    
Note that depending on the permissions of your SimCity 4 installation directory you may need to start the text editor 
with administrator permissions to be able to save the file.

2. Adjust the settings in the `[GraphicsOptions]` section to your preferences.

3. Save the file and start the game.

### Settings overview: 

`EnableIntroVideo` controls whether the game's intro video will be played on startup, defaults to true.
Setting this to false is equivalent to the -Intro:off command line argument. 

 `Driver` the driver that SC4 uses for rendering, the supported values are listed in the following table:

 | Driver | Notes |
 |--------|-------|
 | DirectX | SC4's default hardware renderer. Because this renderer targets DirectX 7, a DirectX wrapper (e.g. [dgVoodo 2](https://github.com/dege-diosg/dgVoodoo2) or [DxWrapper](https://github.com/elishacloud/dxwrapper)) is required for resolutions above 2048x2048. |
 | OpenGL | An unfinished hardware renderer. simmaster07's [SCGL](https://github.com/nsgomez/scgl) project aims to replace this renderer with a new one targeting OpenGL 3.0. |
 | SCGL | simmaster07's replacement for SC4's OpenGL renderer. This is an alias for the OpenGL entry above. |
 | Software | The renderer SC4 uses when no supported hardware rendering support is available. |

 `WindowWidth` the width of SC4's window when running in windowed mode. This is ignored for the full screen and borderless full screen modes.
The minimum value is 800, values above 2048 with the DirectX driver require the use of a DirectX wrapper.

`WindowHeight` the height of SC4's window when running in windowed mode. This is ignored for the full screen and borderless full screen modes.
The minimum value is 600, values above 2048 with the DirectX driver require the use of a DirectX wrapper.

`ColorDepth` the color depth that SC4 uses, in bits per pixel. The supported values are 16 and 32.

`WindowMode` the window mode that SC4 uses, the possible values listed in the following table:

| Window Mode | Notes |
|-------------|-------|
| Windowed | Runs the game in windowed mode, the window size is set by the `WindowWidth` and `WindowHeight` values above. Equivalent to the -w command line parameter.|
| FullScreen | Runs the game in full screen mode. Equivalent to the -f command line parameter. Screen resolutions larger that 2048x2048 in DirectX mode require the use of a DirectX wrapper. |
| BorderlessFullScreen | Runs the game a window that covers the entire screen. Screen resolutions larger that 2048x2048 in DirectX mode require the use of a DirectX wrapper. |
| Borderless | An alias for The `BorderlessFullScreen` option above. |

## Troubleshooting

The plugin should write a `SC4GraphicsOptions.log` file in the same folder as the plugin.    
The log contains status information for the most recent run of the plugin.

# License

This project is licensed under the terms of the GNU Lesser General Public License version 2.1.    
See [LICENSE.txt](LICENSE.txt) for more information.

## 3rd party code

[gzcom-dll](https://github.com/nsgomez/gzcom-dll) Located in the vendor folder, MIT License.    
[scgl](https://github.com/nsgomez/scgl) Located in the vendor folder, GNU Lesser General Public License version 2.1.    
[EABase](https://github.com/electronicarts/EABase) Located in the vendor folder, BSD 3-Clause License.    
[EASTL](https://github.com/electronicarts/EASTL) Located in the vendor folder, BSD 3-Clause License.    
[SC4Fix](https://github.com/nsgomez/sc4fix) - MIT License.     
[Windows Implementation Library](https://github.com/microsoft/wil) - MIT License    
[Boost.PropertyTree](https://www.boost.org/doc/libs/1_83_0/doc/html/property_tree.html) - Boost Software License, Version 1.0.

# Source Code

## Prerequisites

* Visual Studio 2022

## Building the plugin

* Open the solution in the `src` folder
* Update the post build events to copy the build output to you SimCity 4 application plugins folder.
* Build the solution

## Debugging the plugin

Visual Studio can be configured to launch SimCity 4 on the Debugging page of the project properties.
I configured the debugger to launch the game in a window with the following command line:    
`-intro:off -CPUcount:1`

You may need to adjust the resolution for your screen.
