# NPCs in TF2
Fork of NPCs in CS:S for functionality in TF2. Linux only.
> [!WARNING]  
> The TF2 SDK Update broke this extension. I currently do not have the motivation to fix it. If you're crazy enough to help fix the gamedata, feel free to open up a pull request.
## How to Build
- Clone hl2sdk (TF2 branch), metamod and sourcemod from the alliedmodders GitHub
- Make sure CMake and Make is installed along with it's dependencies
- In the terminal, Type "cmake ./" and then "make" to build it.
### Issues
- Some variables need to be updated in the gamedata.
- Helicopters, Gunships and Dropships fly way too fast and outside of their intended path
- Something about pose parameters and attachments keep causing crashes
