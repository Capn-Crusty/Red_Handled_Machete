class CfgPatches
{
	class RedHandleMachete
	{
		units[] = {"MacheteRed"};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data","DZ_Scripts","DZ_Weapons_Melee_Blade"};
	};
};

class CfgVehicles
{
	class Machete;
	// Vanilla machete with the handle wrapped in red cloth tape; stats and behaviour are inherited unchanged.
	// The vanilla model has no camo selection, so the retexture goes on its default "zbytek" section.
	// The materials are vanilla's with our normal/specular maps (matte tape); damage states use them too.
	class MacheteRed: Machete
	{
		displayName = "Red Handle Machete";
		descriptionShort = "A sharp machete with its handle wrapped in red tape.";
		hiddenSelections[] = {"zbytek"};
		hiddenSelectionsTextures[] = {"RedHandleMachete\data\machete_red_co.paa"};
		hiddenSelectionsMaterials[] = {"RedHandleMachete\data\machete_red.rvmat"};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 200;
					healthLevels[] =
					{
						{1.0, {"RedHandleMachete\data\machete_red.rvmat"}},
						{0.7, {"RedHandleMachete\data\machete_red.rvmat"}},
						{0.5, {"RedHandleMachete\data\machete_red_damage.rvmat"}},
						{0.3, {"RedHandleMachete\data\machete_red_damage.rvmat"}},
						{0.0, {"RedHandleMachete\data\machete_red_destruct.rvmat"}}
					};
				};
			};
		};
	};
};

class CfgMods
{
	class RedHandleMachete
	{
		dir = "RedHandleMachete";
		name = "Red Handle Machete";
		credits = "Capn_Crusty";
		author = "Capn_Crusty";
		action = "https://capncrusty.com";
		version = "1.2";
		type = "mod";
		dependencies[] = {"World","Mission"};
		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"RedHandleMachete/Scripts/4_World"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"RedHandleMachete/Scripts/5_Mission"};
			};
		};
	};
};
