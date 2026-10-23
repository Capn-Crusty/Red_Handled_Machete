# Red Handle Machete

A one-of-a-kind DayZ machete: the vanilla machete with its handle wrapped in grimy red athletic tape and a saw-back
along the spine. Same stats as the vanilla machete. Works on vanilla DayZ, no other mods required.

Steam Workshop: https://steamcommunity.com/sharedfiles/filedetails/?id=3821886435

## Only one per server
Only one Red Handle Machete exists on a server at a time. Any other copy, from loot, an admin spawn or a duplicate,
is deleted as soon as it appears. A player who logs out with it keeps it; if they stay away longer than
`AbsentCarrierDays` (default 30), it is released and a new one can spawn. When it's really lost (deleted, despawned,
or on a body that despawned), the next one to spawn as loot becomes the real one.

How: `src/RedHandleMachete/Scripts/4_World/RedHandleMachete/RHM_Unique.c` (server side) records the real machete's
persistent ID and where it is (in the world, or with an offline player since their logout) in
`<server profile>/RedHandleMachete/state.json`, and checks every `MacheteRed` that appears against it.
`MacheteRed.c` and `5_Mission/RedHandleMachete/MissionServer.c` hook it into item creation and deletion, logouts and
shutdowns. Settings: `<server profile>/RedHandleMachete/settings.json`; what it did: `log.txt` there.

## For server owners
Load it with `-mod=` (clients need it too). To make it spawn as loot, merge `pack/types.xml` into your mission's types
(or register it in `cfgeconomycore.xml` as a separate file). See `pack/README.txt`.

## Building it yourself
The textures and materials come from DayZ's own machete files, so they aren't in this repository. To recreate them
you need DayZ Tools and the vanilla files from `dz\weapons\melee\blade\data` (for example on a P: drive):
1. Convert `machete_gr_co.paa`, `machete_smdi.paa` and `machete_nohq.paa` to `machete_co.png`, `smdi.png` and
   `nohq.png` in one folder (DayZ Tools ImageToPAA).
2. In PowerShell:
   ```
   Add-Type -ReferencedAssemblies System.Drawing -Path tools\Tape.cs
   [Tape]::Run('<folder>', 12, 0.22)                    # tape handle: tape_co/_smdi/_nohq.png
   [Tape]::Serrate('<folder>', 330, 1580, 40, 22)       # saw-back teeth (see-through)
   [Tape]::OpenSpine('<folder>', 1024, 951, 469, 1585)  # open the spine's top along the teeth
   [Tape]::Materials('<vanilla data folder>', 'src\RedHandleMachete\data')
   ```
3. Convert `tape_co.png`, `tape_smdi.png`, `tape_nohq.png` to `src/RedHandleMachete/data/machete_red_co.paa`,
   `_smdi.paa` and `_nohq.paa` with ImageToPAA.
4. Pack `src/RedHandleMachete` as `RedHandleMachete.pbo` with prefix `RedHandleMachete` (see `src/RedHandleMachete.txt`),
   binarizing `config.cpp`, and sign it with your own key.

## License
See [LICENSE](LICENSE). Requires DayZ; not affiliated with or endorsed by Bohemia Interactive.
