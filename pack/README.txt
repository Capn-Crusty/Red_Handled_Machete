Red Handle Machete - DayZ mod by Capn_Crusty
https://steamcommunity.com/sharedfiles/filedetails/?id=3821886435

A one-of-a-kind machete: the vanilla machete with its handle wrapped in grimy
red athletic tape and a saw-back along the spine. Same stats as the vanilla
machete.

Only one exists on a server at a time. Any other copy - from loot, an admin
spawn or a duplicate - disappears as soon as it appears. A player who logs out
with it keeps it; if they stay away longer than 30 days (configurable), it is
released and a new one can spawn. When it is really lost (destroyed and gone,
deleted, despawned, or on a body that despawned), a new one spawns as loot.

Class name: MacheteRed

Servers: load it with -mod= (clients need it too). To make it spawn as loot,
merge types.xml into your mission's types (or register it in
cfgeconomycore.xml as a separate file).

Settings: <server profile folder>\RedHandleMachete\settings.json, created on
first start:
  AbsentCarrierDays  days a logged-out holder keeps it (default 30; 0 = forever)
The same folder holds log.txt (what the mod did) and state.json (its record of
the machete; leave it alone, delete it only together with a storage wipe).

More mods and servers: https://capncrusty.com
