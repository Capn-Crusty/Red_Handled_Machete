/*
 * Keeps at most one Red Handle Machete on a server, including one carried by a player who is offline.
 *
 * The loot economy (types.xml) still decides where machetes spawn. This keeps a record of the one real
 * machete (its persistent ID) and deletes any other copy as soon as it appears, whatever made it:
 * a loot spawn, an admin spawn, a duplicate after a crash.
 *
 * Record:   $profile:RedHandleMachete\state.json   (written by this script)
 * Settings: $profile:RedHandleMachete\settings.json (created with defaults on first start)
 *   AbsentCarrierDays: when the real machete is carried by a player who logs out, it stays theirs for this
 *   many days; after that the next copy to spawn becomes the real one (theirs is deleted if they return).
 *   0 = never release it. Fractions work (0.01 = about 15 minutes).
 * Log:      $profile:RedHandleMachete\log.txt
 *
 * The real machete is lost (and a new one may spawn) only when it is really deleted: destroyed and cleaned
 * up, deleted by an admin, despawned, or gone with a despawned body. Logging out, restarts and crashes keep it.
 */

class RHM_Settings
{
	float AbsentCarrierDays = 30;
}

class RHM_State
{
	string Id = "";     // persistent ID of the real machete, "" = none
	int Status = 0;     // 0 = none, 1 = in the world (on the ground, in a container or with an online player), 2 = with an offline player
	string Holder = ""; // ID of the online player holding it (Status 1) or the offline player who has it (Status 2)
	int Since = 0;      // Status 2: when that player logged out (UTC seconds)
}

class RHM_Unique
{
	static const string DIR = "$profile:RedHandleMachete\\";
	static const string STATE_FILE = "$profile:RedHandleMachete\\state.json";
	static const string SETTINGS_FILE = "$profile:RedHandleMachete\\settings.json";
	static const string LOG_FILE = "$profile:RedHandleMachete\\log.txt";
	static const int STARTUP_GRACE_MS = 120000; // after a start, wait this long for the recorded machete to load

	static ref RHM_State s_State;
	static ref RHM_Settings s_Settings;
	static MacheteRed s_Real;
	static bool s_ShuttingDown;
	static bool s_TickStarted;

	static void EnsureLoaded()
	{
		if (s_State)
			return;

		MakeDirectory(DIR);
		string err;

		s_Settings = new RHM_Settings();
		if (FileExist(SETTINGS_FILE))
		{
			if (!JsonFileLoader<RHM_Settings>.LoadFile(SETTINGS_FILE, s_Settings, err))
			{
				Log("settings.json unreadable, using defaults: " + err);
				s_Settings = new RHM_Settings();
			}
		}
		else
		{
			JsonFileLoader<RHM_Settings>.SaveFile(SETTINGS_FILE, s_Settings, err);
		}

		s_State = new RHM_State();
		if (FileExist(STATE_FILE) && !JsonFileLoader<RHM_State>.LoadFile(STATE_FILE, s_State, err))
		{
			Log("state.json unreadable, starting with no machete recorded: " + err);
			s_State = new RHM_State();
		}

		// A stop or crash while someone held it: the machete is in that player's saved character now.
		if (s_State.Status == 1 && s_State.Holder != "")
		{
			s_State.Status = 2;
			s_State.Since = Now();
			Save();
		}

		Log(string.Format("started: record id=%1 status=%2 holder=%3, AbsentCarrierDays=%4", s_State.Id, StatusName(), s_State.Holder, s_Settings.AbsentCarrierDays));

		if (!s_TickStarted)
		{
			s_TickStarted = true;
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(RHM_Unique.Tick, 30000, true);
		}
	}

	// ---- events ----

	static void OnInit(MacheteRed m)
	{
		EnsureLoaded();
		// Give a loaded item time to get its persistent ID and its place in the inventory hierarchy.
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(RHM_Unique.Check, 2000, false, m, 0);
	}

	static void Check(MacheteRed m, int tries)
	{
		if (!m || m.m_RHM_Removing || s_ShuttingDown)
			return;

		string id = IdOf(m);
		if (id == "")
		{
			if (tries < 10)
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(RHM_Unique.Check, 3000, false, m, tries + 1);
			else
				Log("a machete has no persistent ID after 30 s; left alone at " + PosOf(m));
			return;
		}

		if (s_State.Status == 0 || s_State.Id == "")
		{
			Adopt(m, id, "none was recorded");
			return;
		}

		if (id == s_State.Id)
		{
			if (s_Real && s_Real != m)
			{
				Remove(m, "second copy of the real machete (same ID)");
				return;
			}
			if (s_Real != m)
			{
				s_Real = m;
				if (s_State.Status == 2)
					Log("the offline holder is back: " + s_State.Holder);
				s_State.Status = 1;
				s_State.Holder = HolderOf(m);
				s_State.Since = 0;
				Save();
			}
			return;
		}

		// A different machete.
		if (s_State.Status == 1)
		{
			if (s_Real)
			{
				Remove(m, "the real machete exists");
				return;
			}
			if (GetGame().GetTime() < STARTUP_GRACE_MS)
			{
				// The recorded one may still be loading.
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(RHM_Unique.Check, 15000, false, m, tries);
				return;
			}
			Log("the recorded machete never loaded (wiped or lost); treating it as gone");
			Adopt(m, id, "the recorded one is gone");
			return;
		}

		// Status 2: the real one is with an offline player.
		if (s_Settings.AbsentCarrierDays > 0 && Now() - s_State.Since >= s_Settings.AbsentCarrierDays * 86400)
		{
			Log(string.Format("%1 has been offline with it for more than %2 days; releasing it", s_State.Holder, s_Settings.AbsentCarrierDays));
			Adopt(m, id, "the offline holder's time ran out");
			return;
		}
		Remove(m, "the real machete is with offline player " + s_State.Holder);
	}

	static void OnDelete(MacheteRed m)
	{
		if (!s_State || m.m_RHM_Removing || m != s_Real || s_ShuttingDown)
			return;

		s_Real = null;
		if (m.m_RHM_LeavingUid != "")
		{
			s_State.Status = 2;
			s_State.Holder = m.m_RHM_LeavingUid;
			s_State.Since = Now();
			Save();
			Log("logged out with the real machete: " + s_State.Holder);
			return;
		}

		Log("the real machete is gone (deleted at " + PosOf(m) + "); the next one to spawn becomes the real one");
		s_State.Id = "";
		s_State.Status = 0;
		s_State.Holder = "";
		s_State.Since = 0;
		Save();
	}

	// Called by MissionServer before a player's character is saved and removed.
	static void BeforeDisconnect(PlayerBase player, string uid)
	{
		if (!player || !GetGame().IsServer())
			return;
		array<EntityAI> items = {};
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		EntityAI inHands = player.GetHumanInventory().GetEntityInHands();
		if (inHands)
			items.Insert(inHands);
		foreach (EntityAI e : items)
		{
			MacheteRed m = MacheteRed.Cast(e);
			if (m)
				m.m_RHM_LeavingUid = uid;
		}
	}

	// After it: an unconscious or restrained player is killed instead of removed, so the machete stays on the body.
	static void AfterDisconnect(PlayerBase player)
	{
		if (!player || player.IsAlive())
			return;
		array<EntityAI> items = {};
		player.GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, items);
		foreach (EntityAI e : items)
		{
			MacheteRed m = MacheteRed.Cast(e);
			if (m)
				m.m_RHM_LeavingUid = "";
		}
	}

	static void OnMissionFinish()
	{
		if (!s_State)
			return;
		if (s_Real)
		{
			string holder = HolderOf(s_Real);
			if (holder != "")
			{
				s_State.Status = 2;
				s_State.Holder = holder;
				s_State.Since = Now();
				Save();
			}
		}
		s_ShuttingDown = true;
		Log("server stopping: record id=" + s_State.Id + " status=" + StatusName() + " holder=" + s_State.Holder);
	}

	// Keeps the holder current, so a crash while someone holds it is handled at the next start.
	static void Tick()
	{
		if (!s_State || !s_Real || s_ShuttingDown)
			return;
		string holder = HolderOf(s_Real);
		if (holder != s_State.Holder)
		{
			s_State.Holder = holder;
			Save();
		}
	}

	// ---- helpers ----

	static void Adopt(MacheteRed m, string id, string why)
	{
		s_Real = m;
		s_State.Id = id;
		s_State.Status = 1;
		s_State.Holder = HolderOf(m);
		s_State.Since = 0;
		Save();
		Log("real machete is now " + id + " at " + PosOf(m) + " (" + why + ")");
	}

	static void Remove(MacheteRed m, string why)
	{
		m.m_RHM_Removing = true;
		Log("deleted an extra machete at " + PosOf(m) + ": " + why);
		GetGame().ObjectDelete(m);
	}

	static string IdOf(EntityAI e)
	{
		int b1, b2, b3, b4;
		e.GetPersistentID(b1, b2, b3, b4);
		if (b1 == 0 && b2 == 0 && b3 == 0 && b4 == 0)
			return "";
		return string.Format("%1,%2,%3,%4", b1, b2, b3, b4);
	}

	static string HolderOf(EntityAI e)
	{
		PlayerBase p = PlayerBase.Cast(e.GetHierarchyRootPlayer());
		if (p && p.IsAlive() && p.GetIdentity())
			return p.GetIdentity().GetId();
		return "";
	}

	static string PosOf(EntityAI e)
	{
		EntityAI root = e.GetHierarchyRoot();
		if (!root)
			root = e;
		vector pos = root.GetPosition();
		string where = "ground";
		if (root != e)
			where = root.GetType();
		return string.Format("%1 %2 (%3)", Math.Round(pos[0]), Math.Round(pos[2]), where);
	}

	static string StatusName()
	{
		if (s_State.Status == 1)
			return "in world";
		if (s_State.Status == 2)
			return "with offline player";
		return "none";
	}

	// UTC seconds since 1970.
	static int Now()
	{
		int y, mo, d, h, mi, s;
		GetYearMonthDayUTC(y, mo, d);
		GetHourMinuteSecondUTC(h, mi, s);
		if (mo <= 2)
			y = y - 1;
		int era = y / 400;
		int yoe = y - era * 400;
		int mp = (mo + 9) % 12;
		int doy = (153 * mp + 2) / 5 + d - 1;
		int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
		int days = era * 146097 + doe - 719468;
		return days * 86400 + h * 3600 + mi * 60 + s;
	}

	static void Save()
	{
		string err;
		if (!JsonFileLoader<RHM_State>.SaveFile(STATE_FILE, s_State, err))
			Log("cannot save state.json: " + err);
	}

	static void Log(string msg)
	{
		Print("[RedHandleMachete] " + msg);
		int y, mo, d, h, mi, s;
		GetYearMonthDayUTC(y, mo, d);
		GetHourMinuteSecondUTC(h, mi, s);
		FileHandle f = OpenFile(LOG_FILE, FileMode.APPEND);
		if (f)
		{
			FPrintln(f, string.Format("%1-%2-%3 %4:%5:%6 UTC  %7", y, d2(mo), d2(d), d2(h), d2(mi), d2(s), msg));
			CloseFile(f);
		}
	}

	static string d2(int v)
	{
		if (v < 10)
			return "0" + v.ToString();
		return v.ToString();
	}
}
