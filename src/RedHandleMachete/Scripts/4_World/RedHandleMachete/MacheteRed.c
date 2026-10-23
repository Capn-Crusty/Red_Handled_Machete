class MacheteRed extends Machete
{
	string m_RHM_LeavingUid; // set while its holder logs out (the character is about to be removed)
	bool m_RHM_Removing;     // set when RHM_Unique deletes this copy as an extra

	override void EEInit()
	{
		super.EEInit();
		if (GetGame().IsServer())
			RHM_Unique.OnInit(this);
	}

	override void EEDelete(EntityAI parent)
	{
		if (GetGame().IsServer())
			RHM_Unique.OnDelete(this);
		super.EEDelete(parent);
	}
}
