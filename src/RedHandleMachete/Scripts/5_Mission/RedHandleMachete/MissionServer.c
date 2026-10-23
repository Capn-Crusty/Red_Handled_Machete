modded class MissionServer
{
	override void OnMissionStart()
	{
		super.OnMissionStart();
		RHM_Unique.EnsureLoaded();
	}

	override void OnMissionFinish()
	{
		RHM_Unique.OnMissionFinish();
		super.OnMissionFinish();
	}

	override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
	{
		RHM_Unique.BeforeDisconnect(player, uid);
		super.PlayerDisconnected(player, identity, uid);
		RHM_Unique.AfterDisconnect(player);
	}
}
